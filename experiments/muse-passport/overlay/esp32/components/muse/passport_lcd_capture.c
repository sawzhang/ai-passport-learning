/* MIT. Bounded USB capture of actual RGB565 LCD bands, without a framebuffer. */
#include "passport_lcd_capture.h"
#include "lvgl.h"
#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "mbedtls/base64.h"
#include <stdio.h>
#include <string.h>

static bool s_active, s_registered;
static bool write_line(const char *line, size_t size)
{
    bool ok = usb_serial_jtag_write_bytes(line, size, pdMS_TO_TICKS(100)) == (int)size;
    if (!ok) s_active = false; /* A disconnected reader must not strand the UI. */
    return ok;
}
static void capture_flush(lv_event_t *event)
{
    if (!s_active) return;
    lv_display_t *display = lv_event_get_target(event);
    const lv_area_t *area = lv_event_get_param(event);
    lv_draw_buf_t *buf = lv_display_get_buf_active(display);
    if (!area || !buf || !buf->data || lv_display_get_color_format(display) != LV_COLOR_FORMAT_RGB565) {
        s_active = false;
        write_line("PXS ABORT\n", 10);
        return;
    }
    int width = lv_area_get_width(area);
    if (width <= 0 || buf->header.stride < (unsigned)width * 2) {
        s_active = false;
        write_line("PXS ABORT\n", 10);
        return;
    }
    char line[240];
    for (int y = area->y1; y <= area->y2; ++y) {
        const uint8_t *row = buf->data + (y - area->y1) * buf->header.stride;
        for (int x = 0; x < width; x += 72) {
            int count = width - x < 72 ? width - x : 72;
            int header = snprintf(line, sizeof(line), "PXS %d %d %d ", y, (int)area->x1 + x, count);
            size_t encoded = 0;
            if (header < 0 || (size_t)header >= sizeof(line) - 1 ||
                mbedtls_base64_encode((unsigned char *)line + header, sizeof(line) - header - 1,
                                     &encoded, row + x * 2, count * 2) != 0) {
                s_active = false;
                return;
            }
            line[header + encoded] = '\n';
            if (!write_line(line, header + encoded + 1)) return;
        }
    }
    if (lv_display_flush_is_last(display)) {
        write_line("PXS END\n", 8);
        s_active = false;
    }
}
void passport_lcd_capture(void)
{
    if (s_active || !usb_serial_jtag_is_connected()) return;
    lv_display_t *display = lv_display_get_default();
    if (!s_registered) {
        lv_display_add_event_cb(display, capture_flush, LV_EVENT_FLUSH_START, NULL);
        s_registered = true;
    }
    char line[48];
    int n = snprintf(line, sizeof(line), "PXS BEGIN %d %d\n",
                     (int)lv_display_get_horizontal_resolution(display), (int)lv_display_get_vertical_resolution(display));
    s_active = write_line(line, n);
    if (s_active) lv_obj_invalidate(lv_screen_active());
}
