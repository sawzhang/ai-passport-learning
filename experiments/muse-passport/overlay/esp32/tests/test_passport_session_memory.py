"""Exercise the actual early-reserved workspace layout and session cleanup."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class PassportSessionMemory(unittest.TestCase):
    def test_budget_alias_lifetimes_and_cleanup(self):
        source = (ROOT / 'main/noise_control.cpp').read_text()
        run = source[source.index('static session_result_t run_session('):]
        allocation = run.split('#if CONFIG_MUSE_BOARD_FOLOTOY_AI_PASSPORT\n', 2)[2].split('#else', 1)[0]
        cleanup = run.split('cleanup:\n', 1)[1].split('    esp_tls_conn_destroy(tls);', 1)[0]
        cleanup = cleanup.replace('    clear_pending_json_body(control_tx);', '')
        defines = source[source.index('// On a board with the full UI'):source.index('static char s_node_id')]
        code = r'''
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
static bool fail_allocation = false;
static unsigned allocations = 0;
static void *heap_caps_aligned_alloc(size_t alignment, size_t size, unsigned caps) {
    assert(caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    ++allocations;
    return fail_allocation ? nullptr : aligned_alloc(alignment, size);
}
#define CONFIG_MUSE_ENABLED 1
#define CONFIG_SPIRAM 0
#define CONFIG_HOMEHUB_TUNNEL 0
#define CONFIG_MUSE_BOARD_M5STACK_CARDPUTER_ADV 0
#define CONFIG_MUSE_BOARD_FOLOTOY_AI_PASSPORT 1
''' + defines + r'''
int main() {
    assert(noise_ctrl_prepare_workspace(false));
    assert(s_passport_workspace == nullptr && allocations == 0);
    fail_allocation = true;
    assert(!noise_ctrl_prepare_workspace(true));
    fail_allocation = false;
    assert(noise_ctrl_prepare_workspace(true));
    auto *reserved = s_passport_workspace;
    assert(noise_ctrl_prepare_workspace(true));
    assert(s_passport_workspace == reserved && allocations == 2);
    for (int cycle = 0; cycle < 3; ++cycle) {
        uint8_t *ws_buf = nullptr, *rx_buf = nullptr;
        uint8_t *svc_scratch = nullptr, *env_scratch = nullptr;
        uint8_t *tf_scratch = nullptr, *sr_scratch = nullptr;
        struct { void *body = nullptr; } identity;

''' + allocation + r'''
        static_assert(sizeof(s_passport_rx) + sizeof(s_passport_service) +
                      sizeof(s_passport_tx) + sizeof(s_passport_envelope) <= 41 * 1024);
        {
            assert(rx_buf == tf_scratch && ws_buf == svc_scratch);
            assert(rx_buf != ws_buf && rx_buf != sr_scratch);
            assert(env_scratch != ws_buf && env_scratch != sr_scratch);
            assert(WS_RX_BUF_SIZE >= 16 * 1024 + 512);
            assert(SVC_FRAME_SCRATCH >= 16 * 1024 + 512);
            assert(CTRL_BODY_CHUNK_MAX + 512 <= OUT_ENV_SCRATCH);
        }
        memset(rx_buf, 0xa5, WS_RX_BUF_SIZE);
        memset(sr_scratch, 0xa5, SVC_FRAME_SCRATCH);
        memset(ws_buf, 0xa5, WS_BUF_SIZE);
        memset(env_scratch, 0xa5, OUT_ENV_SCRATCH);
''' + cleanup + r'''
        for (size_t i = 0; i < WS_RX_BUF_SIZE; ++i) assert(rx_buf[i] == 0);
        for (size_t i = 0; i < SVC_FRAME_SCRATCH; ++i) assert(sr_scratch[i] == 0);
        for (size_t i = 0; i < WS_BUF_SIZE; ++i) assert(ws_buf[i] == 0);
        for (size_t i = 0; i < OUT_ENV_SCRATCH; ++i) assert(env_scratch[i] == 0);
    }
    assert(noise_ctrl_prepare_workspace(false));
    assert(s_passport_workspace == nullptr);
    assert(noise_ctrl_prepare_workspace(false));
}
'''
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory)
            (path / 'test.cpp').write_text(code)
            subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', str(path / 'test.cpp'),
                            '-o', str(path / 'test')], check=True)
            subprocess.run([str(path / 'test')], check=True)


if __name__ == '__main__':
    unittest.main()
