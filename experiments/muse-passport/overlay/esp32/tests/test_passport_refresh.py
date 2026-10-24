"""Execute the real Passport credential refresh scheduler without task allocation."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

class PassportRefresh(unittest.TestCase):
    def test_main_task_refresh_releases_failed_transport_and_respects_gate(self):
        source = (ROOT / 'main/app.c').read_text()
        functions = '\n'.join(function(source, x) for x in [
            'static void finish_vm_auth_refresh_task(',
            'static void vm_auth_refresh_run(', 'static void schedule_vm_auth_refresh('])
        prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#define CONFIG_MUSE_BOARD_FOLOTOY_AI_PASSPORT 1
#define ESP_LOGI(tag, ...) ((void)(tag))
#define ESP_LOGW(tag, ...) ((void)(tag))
#define pdMS_TO_TICKS(x) (x)
static const char *TAG = "test";
static bool s_vm_auth_refresh_pending, connected, stopped, gate = true;
static bool wifi = true, provisioned = true, operation = true;
static int s_operation_gate = 1, calls, takes, gives;
static int64_t now = 10000000, s_last_vm_auth_refresh_attempt_us;
static const int64_t s_vm_auth_refresh_interval_us = 1000000;
static void auth_lock_take(void) {}
static void auth_lock_give(void) {}
static int64_t esp_timer_get_time(void) { return now; }
static bool operation_gate_take(int n, const char *why) { (void)n; (void)why; ++takes; return gate; }
static void operation_gate_give(void) { ++gives; }
static bool noise_ctrl_is_connected(void) { return connected; }
static bool wifi_mgr_is_connected(void) { return wifi; }
static bool config_is_provisioned(void) { return provisioned; }
static void disconnect_vm_transports(void) { stopped = true; }
static void vTaskDelay(int ticks) { assert(ticks == 20 && stopped); }
static bool connect_preferred_vm_with_gate_held(int n) { assert(n == 0 && stopped); ++calls; return operation; }
'''
        cases = r'''
int main(void) {
    schedule_vm_auth_refresh(); assert(s_vm_auth_refresh_pending);
    now += 1; schedule_vm_auth_refresh(); assert(s_last_vm_auth_refresh_attempt_us == 10000000);
    vm_auth_refresh_run(); assert(!s_vm_auth_refresh_pending && calls == 1 && gives == 1);
    schedule_vm_auth_refresh(); assert(!s_vm_auth_refresh_pending); /* cooldown */
    now += 2000000; schedule_vm_auth_refresh(); gate = false;
    vm_auth_refresh_run(); assert(!s_vm_auth_refresh_pending && calls == 1 && gives == 1);
    now += 2000000; schedule_vm_auth_refresh(); gate = true; connected = true; stopped = false;
    int before = takes; vm_auth_refresh_run();
    assert(!s_vm_auth_refresh_pending && calls == 1 && !stopped && takes == before);
    connected = false; now += 2000000; schedule_vm_auth_refresh(); wifi = false;
    vm_auth_refresh_run(); assert(!s_vm_auth_refresh_pending && calls == 1 && gives == 2);
    wifi = true; now += 2000000; schedule_vm_auth_refresh(); operation = false;
    vm_auth_refresh_run(); assert(!s_vm_auth_refresh_pending && calls == 2 && gives == 3);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp); (p/'test.c').write_text(prefix + functions + cases)
            subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', str(p/'test.c'), '-o', str(p/'test')], check=True)
            subprocess.run([str(p/'test')], check=True)

    def test_ui_ready_gate_is_bounded_and_storage_precedes_wait(self):
        source = (ROOT / 'main/muse_glue.c').read_text()
        body = function(source, 'void muse_glue_storage_ready(')
        code = r"""
#include <assert.h>
#define CONFIG_MUSE_BOARD_FOLOTOY_AI_PASSPORT 1
#define BIT_STORAGE 1
#define BIT_MUSE 4
#define pdFALSE 0
#define pdTRUE 1
#define pdMS_TO_TICKS(x) (x)
#define ESP_LOGW(tag,...) ((void)(tag))
typedef unsigned EventBits_t;
static int s_ready = 1, storage_set, waits;
static const char *TAG = "test";
static const char *identity_ble_name(void) { return "test"; }
static void muse_ble_set_name(const char *name) { assert(name); }
static void xEventGroupSetBits(int group, int bit) { assert(group == 1 && bit == BIT_STORAGE); storage_set = 1; }
static EventBits_t xEventGroupWaitBits(int group, int bits, int clear, int all, int timeout) {
 assert(storage_set && group == 1 && bits == BIT_MUSE && !clear && all && timeout == 20000);
 return ++waits == 1 ? BIT_MUSE : 0;
}
""" + body + r"""
int main(void) { muse_glue_storage_ready(); muse_glue_storage_ready(); assert(waits == 2); return 0; }
"""
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp); (p/'test.c').write_text(code)
            subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', str(p/'test.c'), '-o', str(p/'test')], check=True)
            subprocess.run([str(p/'test')], check=True)

if __name__ == '__main__': unittest.main()
