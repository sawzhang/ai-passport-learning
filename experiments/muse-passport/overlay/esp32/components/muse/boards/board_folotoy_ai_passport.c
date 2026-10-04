/* FoloToy AI Passport adapter. Hardware source: FoloToy BSP snapshot
 * 0b9e4c81ee4421c0bac39ca3561d65a8285acd4a, bsp_pins.h.
 * ESP32-C3, 8MB Flash, no PSRAM; ST7789, ADC three-key ladder, ES8311, CW2017.
 * Original adapter code is MIT. Copied BSP retains its FoloToy MIT license. */
#include "muse_board.h"
#include "muse_menu.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "passport_keys.h"

_Static_assert(MUSE_BTN_TALK_PRESS == 1 && MUSE_BTN_TALK_RELEASE == 2 &&
    MUSE_BTN_AUX_PRESS == 4 && MUSE_BTN_AUX_RELEASE == 8 && MUSE_BTN_UP == 16 &&
    MUSE_BTN_DOWN == 32 && MUSE_BTN_ENTER == 256 && MUSE_BTN_ESCAPE == 512,
    "ADC adapter event bits must match the Muse contract");
static passport_keys_t s_keys;
static esp_err_t init(void) {
    esp_err_t e = bsp_i2c_init();
    if (e != ESP_OK) return e;
    e = bsp_button_init(NULL, NULL);
    if (e != ESP_OK) return e;
    if (bsp_battery_init() != ESP_OK)
        ESP_LOGW("passport", "battery meter unavailable");
    passport_keys_init(&s_keys);
    return ESP_OK;
}
static lv_display_t *display_start(lv_indev_t **touch) {
    *touch = NULL;
    if (bsp_display_init() != ESP_OK) return NULL;
    return bsp_lvgl_init();
}
static void brightness(int pct) { bsp_display_backlight(pct < 0 ? 0 : pct > 100 ? 100 : pct); }
static esp_err_t audio_init(esp_codec_dev_handle_t *spk, esp_codec_dev_handle_t *mic) {
    esp_err_t e = bsp_audio_init();
    if (e != ESP_OK) return e;
    *spk = *mic = bsp_audio_muse_codec();
    return *spk ? ESP_OK : ESP_FAIL;
}
static unsigned poll_buttons(void) {
    int mv = bsp_button_read_mv();
    return passport_keys_poll(&s_keys, mv, muse_menu_is_open());
}
static esp_err_t read_power(muse_power_t *out) {
    int pct = bsp_battery_soc(), mv = bsp_battery_mv();
    *out = (muse_power_t){ .battery_pct = pct, .battery_mv = mv < 0 ? 0 : mv,
        .charging = false, .usb = true };
    /* CW2017 has no validated USB-present/charger-status interface here.
     * Bench profile deliberately disables automatic battery sleep. */
    return pct >= 0 ? ESP_OK : ESP_FAIL;
}
static esp_err_t power_off(void) {
    /* The product's physical power switch is independent. No guessed latch GPIO. */
    return ESP_ERR_NOT_SUPPORTED;
}
static const muse_board_t s_board = {
    .name = "FoloToy AI Passport", .width = BSP_LCD_W, .height = BSP_LCD_H,
    .round = false, .touch = false, .diagonal_in = 2.0f, .keyboard = true,
    .talk_button = "OK", .aux_button = "UP",
    .talk_hint = {LV_ALIGN_RIGHT_MID, -8, 55},
    .aux_hint = {LV_ALIGN_RIGHT_MID, -8, -55}, .frame_ms = 60,
    .init = init, .display_start = display_start,
    .display_lock = bsp_lvgl_lock, .display_unlock = bsp_lvgl_unlock,
    .set_brightness = brightness, .audio_init = audio_init, .mic_slot = 0,
    .poll_buttons = poll_buttons, .read_power = read_power, .power_off = power_off,
};
const muse_board_t *muse_board_get(void) { return &s_board; }
