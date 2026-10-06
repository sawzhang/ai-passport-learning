"""Keep Passport's smaller TX reserve local, without dropping fragmentation gates."""
import pathlib
import subprocess
import tempfile
import unittest

class DmaBudget(unittest.TestCase):
    def test_board_and_tunnel_reserves(self):
        root=pathlib.Path(__file__).resolve().parents[1]
        source=(root/'main/noise_tunnel.cpp').read_text()
        source=source[source.index('// TOTAL free DMA'):source.index('// Tunnel-stream keepalive sentinel')]
        for passport,tunnel,reserve in [(0,0,16384),(1,0,8192),(1,1,16384)]:
            code=f'#define CONFIG_MUSE_BOARD_FOLOTOY_AI_PASSPORT {passport}\n#define CONFIG_HOMEHUB_TUNNEL {tunnel}\n'+r'''
#include <cassert>
#include <cstddef>
#include <cstdint>
#define CONFIG_MUSE_ENABLED 1
#define MALLOC_CAP_DMA 1
static size_t available, largest;
static size_t heap_caps_get_free_size(int){return available;}
static size_t heap_caps_get_largest_free_block(int){return largest;}
''' + source + f'\nint main(){{size_t reserve={reserve};\n'+r'''
 (void)TUN_TX_HEAP_LOG_INTERVAL_US;(void)s_next_tx_heap_log_us;
 available=reserve-1;largest=2048;
 assert(!noise_tx_has_dma_headroom(nullptr));
 assert(noise_tx_has_dma_headroom_reclaiming(1));
 ++available;assert(noise_tx_has_dma_headroom(nullptr));
 assert(noise_tx_has_contiguous_dma_headroom());
 --largest;assert(!noise_tx_has_contiguous_dma_headroom());
 available=0;assert(!noise_tx_has_dma_headroom_reclaiming(2048));
}
'''
            with tempfile.TemporaryDirectory() as d:
                p=pathlib.Path(d);(p/'test.cpp').write_text(code)
                subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
                subprocess.run([str(p/'test')],check=True)

if __name__=='__main__':unittest.main()
