/* Diagnostic build only. Synthetic events do not validate physical switches
 * or visible pixels. Run the actual application/UI on the ESP32 and restore
 * the normal verified image after observing the result. */
#include "esp_heap_caps.h"
#include "esp_system.h"

static unsigned device_failures;
static void device_check(bool condition, const char *name) {
    if (!condition) {
        ++device_failures;
        ESP_LOGE(TAG, "SELFTEST FAIL: %s", name);
    }
}

static bool device_key(bsp_btn_t button, bsp_btn_ev_t event) {
    key_t key = {button, event}, received;
    if (xQueueSend(keys, &key, 0) != pdTRUE ||
        xQueueReceive(keys, &received, pdMS_TO_TICKS(100)) != pdTRUE ||
        !bsp_lvgl_lock(2000)) {
        device_check(false, "input queue / UI lock");
        return false;
    }
    process_key(received);
    bsp_lvgl_unlock();
    /* Allow the real LVGL task and LCD DMA to render between inputs. */
    vTaskDelay(pdMS_TO_TICKS(25));
    return true;
}

static void device_cycle(void) {
    device_key(BSP_BTN_OK, BSP_BTN_LONG);
    device_key(BSP_BTN_OK, BSP_BTN_CLICK);
    device_check(page == BLOCKS && blocks.state == TETRIS_PLAYING, "enter tetris");
    int x = blocks.x;
    device_key(BSP_BTN_UP, BSP_BTN_CLICK);
    device_check(blocks.x == x - 1, "tetris left");
    device_key(BSP_BTN_DOWN, BSP_BTN_CLICK);
    device_check(blocks.x == x, "tetris right");
    unsigned rotation = blocks.rotation;
    device_key(BSP_BTN_OK, BSP_BTN_CLICK);
    device_check(blocks.rotation == ((rotation + 1U) & 3U), "rotation");
    device_key(BSP_BTN_UP, BSP_BTN_LONG);
    device_check(blocks.state == TETRIS_PAUSED, "pause");
    device_check(tetris_model_step(&blocks) == TETRIS_EVENT_NONE, "paused gravity");
    device_key(BSP_BTN_UP, BSP_BTN_LONG);
    device_check(blocks.state == TETRIS_PLAYING, "resume");
    device_key(BSP_BTN_DOWN, BSP_BTN_LONG);
    device_check(blocks.score > 0, "hard drop");
    if (!bsp_lvgl_lock(2000)) { device_check(false, "fixture lock"); return; }
    memset(blocks.board, 0, sizeof(blocks.board));
    for (int col = 0; col < 6; ++col) blocks.board[19][col] = TETRIS_PIECE_J;
    blocks.piece = TETRIS_PIECE_I; blocks.rotation = 0;
    blocks.x = 6; blocks.y = 18; blocks.score = 0; blocks.lines = 0;
    bsp_lvgl_unlock();
    device_key(BSP_BTN_DOWN, BSP_BTN_LONG);
    device_check(blocks.lines == 1 && blocks.score == 100, "line clear / score");
    if (!bsp_lvgl_lock(2000)) { device_check(false, "collision lock"); return; }
    blocks.piece = TETRIS_PIECE_T; blocks.rotation = 0; blocks.x = 0;
    device_check(!tetris_model_move(&blocks, -1), "wall collision");
    for (int col = 3; col < 7; ++col)
        blocks.board[0][col] = blocks.board[1][col] = TETRIS_PIECE_J;
    blocks.x = 3; blocks.y = 17;
    device_check(tetris_model_hard_drop(&blocks) == TETRIS_EVENT_GAME_OVER, "game over");
    refresh_blocks();
    bsp_lvgl_unlock();
    device_key(BSP_BTN_OK, BSP_BTN_CLICK);
    device_check(blocks.state == TETRIS_PLAYING && blocks.score == 0, "restart");
    device_key(BSP_BTN_OK, BSP_BTN_LONG);
    device_key(BSP_BTN_DOWN, BSP_BTN_CLICK);
    device_key(BSP_BTN_OK, BSP_BTN_CLICK);
    device_check(page == WORD_HOME, "enter words");
    memset(words.mistakes, 0, sizeof(words.mistakes));
    device_key(BSP_BTN_OK, BSP_BTN_CLICK);
    device_check(page == WORD_ROUND && words.length == 10, "ten question round");
    for (unsigned question = 0; question < 10; ++question) {
        size_t target = question == 0 ? (words.correct_option + 1) % WG_OPTIONS : words.correct_option;
        for (unsigned move = 0; words.selected != target && move < WG_OPTIONS; ++move)
            device_key(BSP_BTN_DOWN, BSP_BTN_CLICK);
        device_key(BSP_BTN_OK, BSP_BTN_CLICK);
        device_check(words.phase == WG_FEEDBACK && words.last_correct == (question != 0), "answer feedback");
        device_key(BSP_BTN_OK, BSP_BTN_CLICK);
    }
    device_check(words.phase == WG_FINISHED && words.correct == 9 &&
                 words.score == 130 && wg_mistake_count(&words) == 1, "round score / mistakes");
    device_key(BSP_BTN_OK, BSP_BTN_CLICK);
    device_key(BSP_BTN_DOWN, BSP_BTN_CLICK);
    device_key(BSP_BTN_OK, BSP_BTN_CLICK);
    device_check(words.length == 1 && words.phase == WG_QUESTION, "review one mistake");
    for (unsigned move = 0; words.selected != words.correct_option && move < WG_OPTIONS; ++move)
        device_key(BSP_BTN_DOWN, BSP_BTN_CLICK);
    device_key(BSP_BTN_OK, BSP_BTN_CLICK);
    device_check(wg_mistake_count(&words) == 0, "review removes correct word");
    device_key(BSP_BTN_OK, BSP_BTN_CLICK);
    device_key(BSP_BTN_OK, BSP_BTN_LONG);
    save_progress();
    device_check(storage_ok && heap_caps_check_integrity_all(true), "NVS save / heap integrity");
}

static void device_games_selftest(void) {
    nvs_handle_t diagnostic;
    if (!storage_ok || nvs_open("games_selftest", NVS_READWRITE, &diagnostic) != ESP_OK) {
        device_check(false, "NVS initialization"); return;
    }
    uint32_t stage = 0;
    nvs_get_u32(diagnostic, "stage", &stage);
    ESP_LOGI(TAG, "SELFTEST boot stage=%u best=%u mistake1=%d", (unsigned)stage, best_score, words.mistakes[1]);
    if (stage == 2) {
        ESP_LOGI(TAG, "SELFTEST previously completed; restore normal firmware");
        nvs_close(diagnostic); return;
    }
    if (stage == 0) {
        ESP_LOGI(TAG, "SELFTEST synthetic inputs; warm-restart persistence probe");
        best_score = 1234;
        memset(words.mistakes, 0, sizeof(words.mistakes));
        words.mistakes[1] = true; save_pending = true; save_progress();
        device_check(storage_ok, "persistence write");
        esp_err_t result = nvs_set_u32(diagnostic, "stage", 1);
        if (result == ESP_OK) result = nvs_commit(diagnostic);
        device_check(result == ESP_OK, "persistence marker");
        nvs_close(diagnostic);
        if (!device_failures) { vTaskDelay(pdMS_TO_TICKS(200)); esp_restart(); }
        return;
    }
    if (stage == 1) {
        device_check(best_score == 1234 && words.mistakes[1], "progress survives warm restart");
        /* Subsequent game rounds deliberately update progress. Mark the probe
         * completed before stress so opening USB serial mid-test can restart
         * stress without comparing the updated records to the old fixture. */
        if (!device_failures) {
            device_check(nvs_set_u32(diagnostic, "stage", 3) == ESP_OK &&
                         nvs_commit(diagnostic) == ESP_OK, "probe completion marker");
        }
    } else {
        device_check(stage == 3, "resume verified persistence probe");
    }
    ESP_LOGI(TAG, "SELFTEST persistence failures=%u; starting 600-second UI/game stress", device_failures);
    int64_t start = esp_timer_get_time();
    size_t baseline = 0;
    unsigned cycles = 0;
    while (esp_timer_get_time() - start < INT64_C(600000000) && !device_failures) {
        device_cycle(); ++cycles;
        size_t available = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        if (cycles == 5) baseline = available;
        if (cycles > 5) device_check(available + 8192 >= baseline, "retained heap growth under 8KB");
        if (cycles % 10 == 0)
            ESP_LOGI(TAG, "SELFTEST cycles=%u elapsed=%llds heap=%u failures=%u", cycles,
                     (long long)((esp_timer_get_time() - start) / 1000000), (unsigned)available, device_failures);
    }
    device_check(cycles >= 10, "repeated navigation");
    memset(words.mistakes, 0, sizeof(words.mistakes)); best_score = 0;
    save_pending = true; save_progress();
    ESP_LOGI(TAG, "SELFTEST RESULT %s cycles=%u heap=%u failures=%u; physical pixels/ADC/power-cycle unverified",
             device_failures ? "FAIL" : "PASS", cycles,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT), device_failures);
    nvs_set_u32(diagnostic, "stage", 2); nvs_commit(diagnostic); nvs_close(diagnostic);
}
