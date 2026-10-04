"""Run real BLE reassembly code with a fragmented-heap allocator."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    cursor = brace + 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[start:cursor]

class BleRxMemory(unittest.TestCase):
    def test_real_reassembly_with_fragmented_heap(self):
        source = (ROOT / 'main/ble_server.c').read_text()
        harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
#define CHUNK_HEADER_BYTES 3
#define CHUNK_MAGIC 0xfe
#define MAX_RX_TOTAL_BYTES 8192
static uint8_t *s_rx_buf;
static size_t s_rx_len, s_rx_cap;
static uint8_t s_rx_total, s_rx_count, s_rx_next_idx;
static size_t largest=4096, largest_request;
static void *limited_malloc(size_t n) {
 if(n>largest_request)largest_request=n;
 return n<=largest ? malloc(n) : NULL;
}
static void *limited_realloc(void *p,size_t n) {return n<=largest ? realloc(p,n) : NULL;}
#define malloc limited_malloc
#define realloc limited_realloc
'''
        harness += function(source, 'static void rx_reset_locked(void)') + '\n'
        harness += function(source, 'static void handle_rx_write_locked(') + '\n'
        harness += r'''
#undef malloc
#undef realloc
static uint8_t *done;
static size_t done_len;
static void fragment(unsigned idx,unsigned total,size_t n) {
 uint8_t buf[8200]; assert(n<=8197); buf[0]=0xfe;buf[1]=idx;buf[2]=total;
 memset(buf+3,idx,n);handle_rx_write_locked(buf,n+3,&done,&done_len);
}
int main(void) {
 // Real iPhone trace: eleven 250-byte fragments, then 37 bytes (2787 total).
 for(unsigned i=0;i<11;i++){fragment(i,12,250);assert(!done);}
 fragment(11,12,37);assert(done && done_len==2787 && largest_request==3000);
 for(size_t i=0;i<done_len;i++)assert(done[i]==(i<2750?i/250:11));
 free(done);done=NULL;assert(!s_rx_buf && !s_rx_len);
 // Variable fragment lengths must grow rather than reject a valid record.
 fragment(0,2,2);fragment(1,2,10);assert(done && done_len==12);free(done);done=NULL;
 // Missing/duplicate/out-of-order fragments cannot dispatch a partial record.
 fragment(1,2,20);assert(!done && !s_rx_buf);
 fragment(0,3,20);fragment(2,3,20);assert(!done && !s_rx_buf);
 fragment(0,0,20);assert(!done && !s_rx_buf);
 // Explicit allocation and growth failure must reset and allow recovery.
 largest=10;fragment(0,2,20);assert(!done && !s_rx_buf);
 fragment(0,2,2);fragment(1,2,20);assert(!done && !s_rx_buf);
 largest=8192;fragment(0,2,4096);fragment(1,2,4097);assert(!done && !s_rx_buf);
 fragment(0,2,4096);fragment(1,2,4096);assert(done && done_len==8192);free(done);done=NULL;
 fragment(0,1,5);assert(done && done_len==5);free(done);
}
'''
        with tempfile.TemporaryDirectory() as td:
            p=Path(td);(p/'test.c').write_text(harness)
            subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
            subprocess.run([str(p/'test')],check=True)
