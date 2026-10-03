/* Standalone learning games. BSP owns all hardware facts and buses. */
#include <inttypes.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "tetris_model.h"
#include "word_game.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "passport_games";
enum { BG = 0x111827, PANEL = 0x1F2937, INK = 0xF3F4F6, ACCENT = 0x38BDF8 };
typedef enum { HOME, BLOCKS, WORD_HOME, WORD_ROUND } page_t;
typedef struct { bsp_btn_t button; bsp_btn_ev_t event; } key_t;
static QueueHandle_t keys;
static atomic_bool input_ready;
static page_t page;
static unsigned selection;
static tetris_model_t blocks;
static WgGame words;
static int64_t last_fall, last_battery;
static lv_obj_t *screen, *body, *title, *battery, *board, *stats, *overlay;
static lv_obj_t *rows[3], *row_text[3];
static nvs_handle_t storage;
static bool storage_ok, save_pending;
static unsigned best_score;

static lv_obj_t *label(lv_obj_t *parent, int x, int y, int w,
                       const char *text, bool large) {
    lv_obj_t *o = lv_label_create(parent);
    lv_obj_set_pos(o, x, y); lv_obj_set_width(o, w);
    lv_obj_set_style_text_font(o, large ? &lv_font_montserrat_20 : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(INK), 0);
    lv_label_set_text(o, text); return o;
}

static void select_rows(unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        lv_obj_set_style_bg_color(rows[i], lv_color_hex(i == selection ? ACCENT : PANEL), 0);
        lv_obj_set_style_text_color(row_text[i], lv_color_hex(i == selection ? BG : INK), 0);
    }
}

static void row(unsigned i, int y, const char *text) {
    rows[i] = lv_obj_create(body);
    lv_obj_set_pos(rows[i], 20, y); lv_obj_set_size(rows[i], 200, 40);
    lv_obj_set_style_pad_all(rows[i], 8, 0);
    lv_obj_set_style_border_width(rows[i], 0, 0);
    lv_obj_set_style_radius(rows[i], 9, 0);
    lv_obj_set_style_bg_opa(rows[i], LV_OPA_COVER, 0);
    lv_obj_remove_flag(rows[i], LV_OBJ_FLAG_SCROLLABLE);
    row_text[i] = label(rows[i], 0, 2, 184, text, false);
}

static void clear_body(const char *heading) {
    lv_obj_clean(body);
    board = stats = overlay = NULL;
    memset(rows, 0, sizeof(rows)); memset(row_text, 0, sizeof(row_text));
    lv_label_set_text(title, heading);
}

static void show_home(void) {
    page = HOME; selection = 0; clear_body("PLAY LAB");
    label(body, 20, 8, 200, "Choose your challenge", false);
    row(0, 48, "01  Tetris"); row(1, 98, "02  IELTS words");
    select_rows(2);
    label(body, 24, 176, 192, "UP / DOWN select\nOK start\nHold OK to return", false);
    ESP_LOGI(TAG, "page=home");
}

static void draw_board(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_DRAW_MAIN) return;
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_area_t area; lv_obj_get_coords(board, &area);
    static const uint32_t palette[] = {PANEL, 0x22D3EE, 0xFACC15, 0xC084FC,
                                      0x4ADE80, 0xF87171, 0x60A5FA, 0xFB923C};
    int ghost = tetris_model_ghost_y(&blocks);
    for (int y = 0; y < TETRIS_BOARD_HEIGHT; ++y) {
        for (int x = 0; x < TETRIS_BOARD_WIDTH; ++x) {
            unsigned cell = blocks.board[y][x];
            bool shadow = tetris_piece_cell(blocks.piece, blocks.rotation,
                                           x - blocks.x, y - ghost);
            if (tetris_piece_cell(blocks.piece, blocks.rotation,
                                  x - blocks.x, y - blocks.y)) cell = blocks.piece;
            lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d);
            d.bg_color = lv_color_hex(cell ? palette[cell] : shadow ? 0x475569 : BG);
            d.bg_opa = LV_OPA_COVER;
            lv_area_t tile = {.x1 = area.x1 + x * 12, .y1 = area.y1 + y * 12,
                              .x2 = area.x1 + x * 12 + 10, .y2 = area.y1 + y * 12 + 10};
            lv_draw_rect(layer, &d, &tile);
        }
    }
}

static void refresh_blocks(void) {
    static const char *names[] = {"-", "I", "O", "T", "S", "Z", "J", "L"};
    lv_label_set_text_fmt(stats, "SCORE\n%" PRIu32 "\n\nLINES\n%u\n\nNEXT\n%s",
                          blocks.score, blocks.lines, names[blocks.next_piece]);
    if (blocks.state == TETRIS_PLAYING) lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
    else {
        lv_obj_remove_flag(overlay, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(overlay, blocks.state == TETRIS_PAUSED ? "PAUSED\nHold UP" : "GAME OVER\nOK retry");
    }
    if (blocks.score > best_score) { best_score = blocks.score; save_pending = true; }
    lv_obj_invalidate(board);
}

static void show_blocks(void) {
    page = BLOCKS; clear_body("TETRIS");
    tetris_model_init(&blocks, esp_random()); last_fall = esp_timer_get_time();
    board = lv_obj_create(body);
    lv_obj_set_pos(board, 14, 4); lv_obj_set_size(board, 120, 240);
    lv_obj_set_style_pad_all(board, 0, 0); lv_obj_set_style_border_width(board, 0, 0);
    lv_obj_set_style_bg_color(board, lv_color_hex(BG), 0);
    lv_obj_set_style_radius(board, 0, 0); lv_obj_remove_flag(board, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(board, draw_board, LV_EVENT_DRAW_MAIN, NULL);
    stats = label(body, 150, 8, 76, "", false);
    label(body, 148, 174, 80, "UP left\nDN right\nOK rotate\nHold UP:\npause", false);
    overlay = label(body, 21, 100, 106, "", false);
    lv_obj_set_style_text_align(overlay, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(PANEL), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    refresh_blocks(); ESP_LOGI(TAG, "page=tetris best=%u", best_score);
}

static void show_word_home(void) {
    page = WORD_HOME; selection = 0; clear_body("IELTS");
    label(body, 20, 8, 200, "36 words / 6 topics\n10 questions per round", false);
    row(0, 70, "Practice");
    char review[40]; snprintf(review, sizeof(review), "Review mistakes (%u)", (unsigned)wg_mistake_count(&words));
    row(1, 120, review); select_rows(2);
    label(body, 24, 181, 192, "Read the definition.\nChoose the matching word.", false);
    label(body, 24, 234, 190, storage_ok ? "Progress saved locally" : "Session progress only", false);
}

static void show_question(void) {
    page = WORD_ROUND; clear_body("IELTS");
    if (words.phase == WG_FINISHED) {
        label(body, 24, 18, 192, words.length ? "ROUND COMPLETE" : "NO MISTAKES", true);
        char summary[100];
        snprintf(summary, sizeof(summary), "%u / %u correct\nScore %u\nMistakes remaining: %u",
                 words.correct, (unsigned)words.length, words.score, (unsigned)wg_mistake_count(&words));
        label(body, 24, 88, 192, summary, false);
        label(body, 24, 205, 192, "OK: practice / review", false);
        ESP_LOGI(TAG, "round_done correct=%u total=%u score=%u", words.correct, (unsigned)words.length, words.score);
        return;
    }
    const WgWord *word = &wg_words[words.order[words.position]];
    char progress[64]; snprintf(progress, sizeof(progress), "%u / %u  %s",
                               (unsigned)words.position + 1, (unsigned)words.length, word->topic);
    label(body, 20, 0, 200, progress, false);
    label(body, 20, 28, 200, word->definition, false);
    for (unsigned i = 0; i < WG_OPTIONS; ++i) row(i, 108 + 45 * (int)i, wg_words[words.options[i]].english);
    selection = (unsigned)words.selected; select_rows(WG_OPTIONS);
    label(body, 24, 243, 192, "UP / DOWN select; OK", false);
}

static void word_feedback(void) {
    clear_body("IELTS");
    const WgWord *word = &wg_words[words.order[words.position]];
    label(body, 24, 10, 192, words.last_correct ? "CORRECT!" : "KEEP LEARNING", true);
    label(body, 24, 62, 192, word->english, true);
    label(body, 24, 105, 192, word->definition, false);
    char result[64]; snprintf(result, sizeof(result), "Score %u\nStreak %u", words.score, words.streak);
    label(body, 24, 178, 192, result, false);
    label(body, 24, 235, 192, "OK: next question", false);
}

static void process_key(key_t key) {
    if (key.event == BSP_BTN_LONG && key.button == BSP_BTN_OK) {
        show_home(); save_pending = true; return;
    }
    if (page == BLOCKS) {
        bool changed = false;
        if (blocks.state == TETRIS_GAME_OVER && key.button == BSP_BTN_OK && key.event == BSP_BTN_CLICK) {
            tetris_model_reset(&blocks); last_fall = esp_timer_get_time(); changed = true;
        } else if (key.event == BSP_BTN_LONG && key.button == BSP_BTN_UP) {
            changed = tetris_model_toggle_pause(&blocks); last_fall = esp_timer_get_time();
        } else if (key.event == BSP_BTN_LONG && key.button == BSP_BTN_DOWN) {
            changed = tetris_model_hard_drop(&blocks) != TETRIS_EVENT_NONE;
            last_fall = esp_timer_get_time();
        } else if (key.event == BSP_BTN_CLICK) {
            if (key.button == BSP_BTN_UP) changed = tetris_model_move(&blocks, -1);
            if (key.button == BSP_BTN_DOWN) changed = tetris_model_move(&blocks, 1);
            if (key.button == BSP_BTN_OK) changed = tetris_model_rotate(&blocks);
        }
        if (changed) refresh_blocks();
        return;
    }
    if (key.event != BSP_BTN_CLICK) return;
    if (page == HOME || page == WORD_HOME) {
        if (key.button == BSP_BTN_UP || key.button == BSP_BTN_DOWN) { selection ^= 1U; select_rows(2); }
        else if (key.button == BSP_BTN_OK) {
            if (page == HOME) { if (!selection) show_blocks(); else show_word_home(); }
            else { wg_start(&words, selection == 1); show_question(); }
        }
    } else if (page == WORD_ROUND) {
        if (words.phase == WG_QUESTION) {
            if (key.button == BSP_BTN_UP || key.button == BSP_BTN_DOWN) {
                wg_move(&words, key.button == BSP_BTN_UP ? -1 : 1);
                selection = (unsigned)words.selected; select_rows(WG_OPTIONS);
            } else if (key.button == BSP_BTN_OK) {
                wg_submit(&words); word_feedback(); save_pending = true;
            }
        } else if (key.button == BSP_BTN_OK) {
            if (words.phase == WG_FINISHED) show_word_home();
            else { wg_next(&words); show_question(); }
        }
    }
}

static void save_progress(void) {
    if (!save_pending) return;
    save_pending = false;
    if (!storage_ok) return;
    uint64_t mask = 0;
    for (size_t i = 0; i < wg_word_count; ++i) if (words.mistakes[i]) mask |= UINT64_C(1) << i;
    esp_err_t e = nvs_set_u64(storage, "mistakes_v1", mask);
    if (e == ESP_OK) e = nvs_set_u32(storage, "best_v1", best_score);
    if (e == ESP_OK) e = nvs_commit(storage);
    if (e != ESP_OK) { storage_ok = false; ESP_LOGE(TAG, "progress save failed: %s", esp_err_to_name(e)); }
}

#ifdef PASSPORT_DEVICE_SELF_TEST
#include "../tests/device_games_selftest.inc"
#endif

static void on_button(bsp_btn_t button, bsp_btn_ev_t event, void *user) {
    (void)user;
    if (!atomic_load_explicit(&input_ready, memory_order_acquire)) return;
    if (event != BSP_BTN_CLICK && event != BSP_BTN_LONG) return;
    const key_t key = {button, event}; (void)xQueueSend(keys, &key, 0);
}

void app_main(void) {
    ESP_LOGI(TAG, "Passport Games v0.1: Tetris then IELTS, 36-word bank v1");
    if (wg_word_count > 64 || !wg_init(&words, wg_word_count, esp_random())) return;
    esp_err_t e = nvs_flash_init();
    /* Existing data is never automatically erased when NVS needs recovery. */
    if (e == ESP_OK && nvs_open("learning_games", NVS_READWRITE, &storage) == ESP_OK) {
        storage_ok = true; uint64_t mask = 0; uint32_t score = 0;
        if (nvs_get_u64(storage, "mistakes_v1", &mask) == ESP_OK)
            for (size_t i = 0; i < wg_word_count; ++i) words.mistakes[i] = (mask & (UINT64_C(1) << i)) != 0;
        if (nvs_get_u32(storage, "best_v1", &score) == ESP_OK) best_score = score;
    } else ESP_LOGW(TAG, "NVS unavailable; using session-only progress");
    bool battery_ok = bsp_battery_init() == ESP_OK;
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) { ESP_LOGE(TAG, "display init failed"); return; }
    keys = xQueueCreate(12, sizeof(key_t));
    if (!keys) { ESP_LOGE(TAG, "input queue allocation failed"); return; }
    if (!bsp_lvgl_lock(2000)) { vQueueDelete(keys); keys = NULL; return; }
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(BG), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0); lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    title = label(screen, 22, 20, 148, "", true);
    battery = label(screen, 177, 24, 44, "--%", false);
    body = lv_obj_create(screen); lv_obj_set_pos(body, 0, 52); lv_obj_set_size(body, 240, 268);
    lv_obj_set_style_pad_all(body, 0, 0); lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_radius(body, 0, 0); lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    show_home(); lv_screen_load(screen); bsp_lvgl_unlock(); bsp_display_backlight(80);
    e = bsp_button_init(on_button, NULL);
    if (e != ESP_OK) { ESP_LOGE(TAG, "button init failed: %s", esp_err_to_name(e)); return; }
    ESP_LOGI(TAG, "ready: input/display initialized; battery=%d storage=%d", battery_ok, storage_ok);
#ifdef PASSPORT_DEVICE_SELF_TEST
    device_games_selftest();
#endif
    atomic_store_explicit(&input_ready, true, memory_order_release);
    for (;;) {
        key_t key; bool have_key = xQueueReceive(keys, &key, pdMS_TO_TICKS(20)) == pdTRUE;
        int64_t now = esp_timer_get_time();
        bool battery_due = now - last_battery >= INT64_C(10000000) || last_battery == 0;
        int soc = battery_due && battery_ok ? bsp_battery_soc() : -1;
        if (bsp_lvgl_lock(200)) {
            if (have_key) process_key(key);
            if (page == BLOCKS && blocks.state == TETRIS_PLAYING &&
                now - last_fall >= (int64_t)tetris_model_drop_interval_ms(&blocks) * 1000) {
                tetris_model_step(&blocks); refresh_blocks(); last_fall = now;
            }
            if (battery_due) {
                if (soc >= 0 && soc <= 100) lv_label_set_text_fmt(battery, "%d%%", soc);
                else lv_label_set_text(battery, "--%");
                last_battery = now;
            }
            bsp_lvgl_unlock();
        }
        /* Flash writes and I2C reads run outside callbacks and the LVGL lock. */
        if (save_pending && (page != BLOCKS || blocks.state != TETRIS_PLAYING)) save_progress();
    }
}
