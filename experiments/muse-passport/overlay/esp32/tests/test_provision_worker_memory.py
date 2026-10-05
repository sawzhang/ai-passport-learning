"""Exercise actual GATT worker handoff and failure cleanup with host stubs."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_ble_rx_memory import function
ROOT=Path(__file__).resolve().parents[1]
class ProvisionWorkerMemory(unittest.TestCase):
 def test_worker_starts_after_rx_free_and_handles_stale_or_failed_task(self):
  source=(ROOT/'main/ble_server.c').read_text()
  code=r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define BLE_ATT_ERR_UNLIKELY 1
#define BLE_ATT_ERR_INSUFFICIENT_RES 2
#define BLE_GATT_ACCESS_OP_WRITE_CHR 3
#define OS_MBUF_PKTLEN(x) 253
#define pdPASS 1
static bool s_shutting_down, valid=true, task_ok=true;
static int packet_buffers, tasks, errors, disconnected, cleanups;
static void *packet;
typedef struct {char *ssid,*password,*access_token,*refresh_token,*username,*ota_url;bool ota_force;char *api_url,*api_url_v2,*noise_host;uint32_t session_generation;} provision_args_t;
struct ble_gatt_access_ctxt {int op;void *om;};
static provision_args_t *owned;
static void *rx_malloc(size_t n){packet=malloc(n);if(packet)packet_buffers++;return packet;}
static void rx_free(void *p){if(p==packet){packet_buffers--;packet=NULL;}free(p);}
static bool link_pairing_provisioning_session_valid(uint32_t g){return valid && g==1;}
static void provision_task(void *a){(void)a;}
static int xTaskCreate(void (*fn)(void*),const char *name,int stack,void *a,int prio,void *out){
 (void)fn;(void)name;(void)prio;(void)out;assert(packet_buffers==0);assert(stack==8192);tasks++;
 if(task_ok){owned=a;return pdPASS;}return 0;
}
static void secure_free_str(char *p){if(p){cleanups++;free(p);}}
static void ble_server_send_pairing_status(const char *s,uint32_t g){(void)g;assert(!strcmp(s,"error_operation_in_progress"));errors++;}
static void ble_server_disconnect_pairing_session(uint32_t g){(void)g;disconnected++;}
'''
  code+=function(source,'static void start_provision(')+'\n'
  code+=r'''
static int ble_hs_mbuf_to_flat(void *m,void *b,uint16_t n,uint16_t *out){(void)m;memset(b,0,n);*out=n;return 0;}
static void handle_rx_write(const uint8_t *b,size_t n,provision_args_t **a){
 (void)b;assert(n==253 && packet_buffers==1);*a=calloc(1,sizeof(**a));assert(*a);
 (*a)->session_generation=1;(*a)->password=malloc(8);assert((*a)->password);
}
#define malloc rx_malloc
#define free rx_free
'''
  code+=function(source,'static int rx_access(')+'\n'
  code+=r'''
#undef malloc
#undef free
int main(void){
 struct ble_gatt_access_ctxt c={BLE_GATT_ACCESS_OP_WRITE_CHR,NULL};
 assert(rx_access(0,0,&c,NULL)==0);assert(tasks==1 && owned && !packet_buffers && !errors);
 free(owned->password);free(owned);owned=NULL;
 valid=false;assert(rx_access(0,0,&c,NULL)==0);assert(tasks==1 && errors==1 && disconnected==1 && cleanups==1);
 valid=true;task_ok=false;assert(rx_access(0,0,&c,NULL)==0);assert(tasks==2 && errors==2 && disconnected==2 && cleanups==2);
 assert(!packet_buffers && !owned);
}
'''
  with tempfile.TemporaryDirectory() as td:
   p=Path(td);(p/'test.c').write_text(code)
   subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-parameter',str(p/'test.c'),'-o',str(p/'test')],check=True)
   subprocess.run([str(p/'test')],check=True)
