"""Run the real Link send admission against a full queue and allocation failures."""
import pathlib
import subprocess
import tempfile
import unittest

class SendPressure(unittest.TestCase):
    def test_wait_does_not_allocate_and_failures_release(self):
        root=pathlib.Path(__file__).resolve().parents[1]
        text=(root/'main/noise_control.cpp').read_text()
        function=text.split('extern "C" bool noise_ctrl_req_send(',1)[1].split('extern "C" void noise_ctrl_req_cancel',1)[0]
        function='extern "C" bool noise_ctrl_req_send('+function
        code=r'''
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#define REQ_STREAM_BASE 10
#define REQ_QUEUE_BYTES 2048
#define pdMS_TO_TICKS(n) (n)
static bool s_req_q=true, drain=false, put_ok=true, alloc_ok=true;
static int s_req_queued_bytes=0, allocated=0, live=0;
static int64_t now=0;
static int64_t esp_timer_get_time(){return now;}
static void vTaskDelay(int n){assert(allocated==0);now+=n*1000;if(drain)s_req_queued_bytes=0;}
static void *allocate(size_t n){++allocated;if(!alloc_ok)return nullptr;++live;return std::malloc(n);}
static void release(void *p){if(p){--live;std::free(p);}}
enum class req_op_kind {Body};
struct req_op{req_op_kind kind;bool end;int64_t id;uint8_t *data;size_t len;void *cb,*ctx;};
static bool req_put(req_op op,int wait){(void)wait;assert(op.len==1024);if(put_ok)release(op.data);return put_ok;}
#define malloc allocate
#define free release
''' + function + r'''
int main(){
 char data[1024]={};
 s_req_queued_bytes=2048;
 assert(!noise_ctrl_req_send(10,data,sizeof(data),false,10));
 assert(allocated==0 && live==0);
 now=0;drain=true;
 assert(noise_ctrl_req_send(10,data,sizeof(data),false,20));
 assert(allocated==1 && live==0);
 allocated=0;put_ok=false;
 assert(!noise_ctrl_req_send(10,data,sizeof(data),false,20));
 assert(allocated==1 && live==0);
 allocated=0;alloc_ok=false;
 assert(!noise_ctrl_req_send(10,data,sizeof(data),false,20));
 assert(allocated==1 && live==0);
}
'''
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d);(p/'test.cpp').write_text(code)
            subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
            subprocess.run([str(p/'test')],check=True)

if __name__=='__main__':unittest.main()
