"""Compile and exercise the actual ADC adapter without ESP-IDF."""
import pathlib, subprocess, tempfile, unittest
class PassportKeys(unittest.TestCase):
 def test_edges(self):
  root=pathlib.Path(__file__).resolve().parents[1]
  code=r'''
#include <assert.h>
#include "passport_keys.h"
unsigned key(passport_keys_t *s,int mv,bool menu) {unsigned e=0;for(int i=0;i<3;i++)e|=passport_keys_poll(s,mv,menu);return e;}
int main(void) {
 passport_keys_t s;passport_keys_init(&s);
 assert(passport_key_mv(-1)==-2);assert(passport_key_mv(0)==0);
 assert(passport_key_mv(149)==0);assert(passport_key_mv(150)==1);
 assert(passport_key_mv(446)==1);assert(passport_key_mv(447)==2);
 assert(passport_key_mv(1899)==2);assert(passport_key_mv(1900)==-1);
 assert(key(&s,3300,false)==0);assert(key(&s,595,false)==1);
 assert(key(&s,595,false)==0);assert(key(&s,-1,false)==0);
 assert(key(&s,3300,false)==2);
 assert(key(&s,0,false)==4);assert(key(&s,3300,true)==8);
 assert(key(&s,0,true)==16);assert(key(&s,3300,true)==0);
 assert(key(&s,300,true)==32);unsigned e=0;
 for(int i=0;i<60;i++)e|=passport_keys_poll(&s,300,true);
 assert(e==512);assert(passport_keys_poll(&s,300,true)==0);
 key(&s,3300,true);assert(key(&s,595,true)==256);
 assert(key(&s,3300,true)==0);
 passport_keys_init(&s);assert(passport_keys_poll(&s,595,false)==0);
 assert(passport_keys_poll(&s,3300,false)==0);
 assert(key(&s,3300,false)==0);
}
'''
  with tempfile.TemporaryDirectory() as t:
   p=pathlib.Path(t);(p/'test.c').write_text(code)
   subprocess.run(['cc','-Wall','-Wextra','-Werror','-std=c11','-I',str(root/'components/muse/boards'),str(p/'test.c'),'-o',str(p/'test')],check=True)
   subprocess.run([str(p/'test')],check=True)
