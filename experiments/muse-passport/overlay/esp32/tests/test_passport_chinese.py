"""Validate the shipped glyph inventory and real C caption helpers."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class ChineseReply(unittest.TestCase):
    def test_font_covers_gb2312_han_and_bounded_metrics(self):
        source = (ROOT / "components/muse/fonts/passport_font_cjk_16.c").read_text()
        glyphs = {int(x, 16) for x in re.findall(r"/\* U\+([0-9A-Fa-f]+)", source)}
        required = set()
        for cp in range(0x4e00, 0xa000):
            try:
                chr(cp).encode("gb2312")
                required.add(cp)
            except UnicodeEncodeError:
                pass
        self.assertEqual(len(required), 6763)
        self.assertFalse(required - glyphs)
        self.assertTrue(set(map(ord, "中文显示测试你好世界机器学习计划，。！？")) <= glyphs)
        self.assertNotIn(0x20000, glyphs)  # no claim of full Unicode support
        advances = [int(x) for x in re.findall(r"\.adv_w = (\d+)", source)]
        self.assertLessEqual(max(advances), 16 * 16)
        self.assertIn(".bitmap_format = 0", source)  # no decompression buffer

    def test_utf8_boundaries_and_wide_character_pages(self):
        code = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "muse_text.h"
#include "muse_chat_priv.h"
void muse_state_page(int *cols, int *lines) { *cols = 8; *lines = 2; }
int main(void) {
    size_t n;
    assert(muse_text_columns("中", &n) == 2 && n == 3);
    assert(muse_text_columns("，", &n) == 2 && n == 3);
    assert(muse_text_columns("A", &n) == 1 && n == 1);
    assert(muse_text_columns("—", &n) == 2 && n == 3);
    const char *text = "A中文B你好世界机器学习";
    char out[128];
    assert(muse_hatch_caption_at(text, 0, out, sizeof(out)));
    assert(strcmp(out, "A中文B你\n好世界机") == 0);
    char buf[64];
    const char *mixed = "A中B文😀";
    for (size_t cap = 1; cap <= strlen(mixed) + 1; ++cap) {
        snprintf(buf, cap, "%s", mixed);
        muse_text_trim_utf8(buf);
        size_t len = strlen(buf);
        assert(len == 0 || len == 1 || len == 4 || len == 5 || len == 8 || len == 12);
        assert(strncmp(buf, mixed, len) == 0);
    }
    strcpy(buf, "你好，世界");
    muse_text_to_ascii(buf, sizeof(buf));
    assert(strcmp(buf, "你好，世界") == 0);
    muse_hatch_tail_words("中文测试", out, 6);
    assert(strcmp(out, "中") == 0);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp); (p / "test.c").write_text(code)
            subprocess.run(["cc", "-std=gnu11", "-Wall", "-Wextra", "-Werror",
                "-fsanitize=address,undefined", "-DCONFIG_MUSE_BOARD_FOLOTOY_AI_PASSPORT=1",
                "-include", str(ROOT / "tests/host_compat.h"),
                "-I", str(ROOT / "components/muse"), str(p / "test.c"),
                str(ROOT / "components/muse/muse_text.c"),
                str(ROOT / "components/muse/muse_chat_text.c"), "-o", str(p / "test")], check=True)
            subprocess.run([str(p / "test")], check=True)

if __name__ == "__main__":
    unittest.main()
