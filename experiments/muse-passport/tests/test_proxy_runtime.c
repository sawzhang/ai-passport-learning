#include "passport_proxy.h"
#include "proxy_protocol.h"
#include "esp_tls.h"
#include "nvs.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

int __wrap_esp_tls_conn_new_sync(const char *, int, int, const esp_tls_cfg_t *, esp_tls_t *);
int __wrap_esp_tls_conn_new_async(const char *, int, int, const esp_tls_cfg_t *, esp_tls_t *);
static char saved[64];
static int storage_error, tcp_error, peer, real_calls, tunnels;
static const char *response;
static bool stall;
static const esp_tls_cfg_t *original;
static pthread_t thread;
int64_t esp_timer_get_time(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (int64_t)t.tv_sec*1000000+t.tv_nsec/1000; }
esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *h) { (void)mode; assert(!strcmp(ns,"passport_proxy")); *h=1; return storage_error; }
esp_err_t nvs_get_str(nvs_handle_t h,const char *key,char *out,size_t *cap) { (void)h; assert(!strcmp(key,"endpoint")); if(strlen(saved)+1>*cap)return 2; strcpy(out,saved); return 0; }
esp_err_t nvs_set_str(nvs_handle_t h,const char *key,const char *s) { (void)h; assert(!strcmp(key,"endpoint")); strcpy(saved,s); return 0; }
esp_err_t nvs_commit(nvs_handle_t h) { (void)h; return 0; }
void nvs_close(nvs_handle_t h) { (void)h; }
static void *server(void *arg) {
    (void)arg; char header[576]; size_t n=0;
    while(n<sizeof(header)-1) { ssize_t r=recv(peer,header+n,1,0); if(r!=1)break; ++n; if(n>=4&&!memcmp(header+n-4,"\r\n\r\n",4))break; }
    header[n]=0;
    assert(!strcmp(header,"CONNECT cloud.example:443 HTTP/1.1\r\nHost: cloud.example:443\r\n\r\n"));
    if (stall) { struct timespec delay={.tv_nsec=200000000}; nanosleep(&delay,NULL); }
    if(response) {
        /* Fragmented headers plus bytes immediately after the CONNECT header. */
        size_t size=strlen(response);
        for(size_t i=0;i<size;++i) { if(send(peer,response+i,1,0)!=1)break; }
    }
    close(peer); return NULL;
}
esp_err_t esp_tls_plain_tcp_connect(const char *host,int len,int port,const esp_tls_cfg_t *cfg,void *error,int *fd) {
    (void)error; assert(len==12&&!strcmp(host,"192.168.1.10")&&port==18087);
    assert(cfg->is_plain_tcp&&!cfg->non_block); ++tunnels;
    if(tcp_error)return 2;
    int pair[2]; assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair)); *fd=pair[0]; peer=pair[1];
    assert(!pthread_create(&thread,NULL,server,NULL)); return 0;
}
int __real_esp_tls_conn_new_sync(const char *host,int len,int port,const esp_tls_cfg_t *cfg,esp_tls_t *tls) {
    assert(len==13&&!strcmp(host,"cloud.example")&&port==443&&cfg==original); ++real_calls;
    if(saved[0]&&!cfg->is_plain_tcp) {
        assert(tls->conn_state==ESP_TLS_CONNECTING&&tls->is_tls&&tls->sockfd>=0);
        assert(FD_ISSET(tls->sockfd,&tls->rset));
    }
    tls->conn_state=ESP_TLS_HANDSHAKE; return 1;
}
int __real_esp_tls_conn_new_async(const char *host,int len,int port,const esp_tls_cfg_t *cfg,esp_tls_t *tls) {
    if(tls->conn_state==ESP_TLS_HANDSHAKE) { ++real_calls; return 1; }
    __real_esp_tls_conn_new_sync(host,len,port,cfg,tls); return 0;
}
static esp_tls_t init(void) { esp_tls_t t={0}; t.sockfd=-1; return t; }
int main(void) {
    signal(SIGPIPE, SIG_IGN);
    passport_proxy_endpoint e;
    assert(passport_proxy_parse("192.168.1.10:18087",&e));
    assert(!strcmp(e.host,"192.168.1.10")&&e.port==18087);
    const char *bad[]={"", "off", "0.0.0.0:80", "127.0.0.1:80", "256.1.1.1:80", "192.168.1.1:0", "192.168.1.1:65536", "192.168.1.1:1x", "01.2.3.4:80", "1.2.3:80", "1.2.3.4", "224.1.1.1:80", "1.2.3.4:999999999999999999"};
    for(size_t i=0;i<sizeof(bad)/sizeof(*bad);++i)assert(!passport_proxy_parse(bad[i],&e));
    char request[576];
    assert(passport_proxy_request(request,sizeof(request),"cloud.example",13,443)>0);
    assert(passport_proxy_request(request,5,"cloud.example",13,443)<0);
    assert(passport_proxy_request(request,sizeof(request),"x\r\nX: evil",10,443)<0);
    assert(!passport_proxy_response("HTTP/1.1 407 Proxy Authentication Required\r\n\r\n",46));
    esp_tls_cfg_t cfg={.timeout_ms=100,.crt_bundle_attach=(void *)1,.common_name="cloud.example"}; original=&cfg;
    esp_tls_t t=init();
    assert(__wrap_esp_tls_conn_new_sync("cloud.example",13,443,&cfg,&t)==1&&tunnels==0);
    assert(!passport_proxy_command("wifi.connect"));
    assert(passport_proxy_command("proxy=192.168.1.10:18087"));
    assert(!strcmp(saved,"192.168.1.10:18087"));
    assert(passport_proxy_command("proxy=0.0.0.0:80")&&!strcmp(saved,"192.168.1.10:18087"));
    response="HTTP/1.1 200 Connection established\r\nProxy: test\r\n\r\nTLS";
    t=init(); assert(__wrap_esp_tls_conn_new_sync("cloud.example",13,443,&cfg,&t)==1);
    assert(!pthread_join(thread,NULL)); char tail[3]; assert(recv(t.sockfd,tail,3,0)==3&&!memcmp(tail,"TLS",3)); close(t.sockfd);
    cfg.non_block=true; t=init(); int before=tunnels;
    assert(__wrap_esp_tls_conn_new_async("cloud.example",13,443,&cfg,&t)==0);
    assert(__wrap_esp_tls_conn_new_async("cloud.example",13,443,&cfg,&t)==1&&tunnels==before+1);
    assert(!pthread_join(thread,NULL)); close(t.sockfd);
    response="HTTP/1.1 407 Authentication required\r\n\r\n"; t=init(); before=real_calls;
    assert(__wrap_esp_tls_conn_new_sync("cloud.example",13,443,&cfg,&t)==-1&&real_calls==before&&t.sockfd==-1);
    assert(!pthread_join(thread,NULL));
    response=NULL; t=init(); assert(__wrap_esp_tls_conn_new_sync("cloud.example",13,443,&cfg,&t)==-1); assert(!pthread_join(thread,NULL));
    stall=true; t=init(); int64_t began=esp_timer_get_time();
    assert(__wrap_esp_tls_conn_new_sync("cloud.example",13,443,&cfg,&t)==-1);
    assert(esp_timer_get_time()-began<180000); assert(!pthread_join(thread,NULL)); stall=false;
    char oversized[2100]; memset(oversized,'x',sizeof(oversized)); memcpy(oversized,"HTTP/1.1 200 ",13); oversized[2099]=0;
    response=oversized; t=init(); assert(__wrap_esp_tls_conn_new_sync("cloud.example",13,443,&cfg,&t)==-1); assert(!pthread_join(thread,NULL));
    tcp_error=1; t=init(); assert(__wrap_esp_tls_conn_new_sync("cloud.example",13,443,&cfg,&t)==-1&&real_calls==before); tcp_error=0;
    storage_error=2; t=init(); assert(__wrap_esp_tls_conn_new_sync("cloud.example",13,443,&cfg,&t)==-1&&real_calls==before); storage_error=0;
    strcpy(saved,"corrupt"); t=init(); assert(__wrap_esp_tls_conn_new_sync("cloud.example",13,443,&cfg,&t)==-1&&real_calls==before);
    passport_proxy_command("proxy=off"); assert(!saved[0]);
    puts("PASS: proxy parsing, fragmented CONNECT, TLS preservation, async reentry, timeout/header limits, failures and disable");
}
