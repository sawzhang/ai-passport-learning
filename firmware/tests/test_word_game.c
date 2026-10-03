#include "word_game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void answer(WgGame *g, bool correct) {
    g->selected = (g->correct_option + (correct ? 0 : 1)) % WG_OPTIONS;
    assert(wg_submit(g));
    unsigned score = g->score;
    assert(!wg_submit(g)); assert(g->score == score);
    wg_next(g);
}
int main(void) {
    WgGame g;
    assert(!wg_init(NULL, 3, 1));
    assert(!wg_init(&g, 2, 1)); assert(!wg_init(&g, WG_MAX_WORDS+1, 1));
    for (size_t i = 0; i < wg_word_count; ++i) {
        assert(*wg_words[i].english && *wg_words[i].chinese && *wg_words[i].topic);
        assert(wg_words[i].definition && *wg_words[i].definition);
        assert(strlen(wg_words[i].definition) <= 64);
        for (const unsigned char *p = (const unsigned char *)wg_words[i].definition; *p; ++p)
            assert(*p >= 32 && *p <= 126);
        for (size_t j = 0; j < i; ++j)
            assert(strcmp(wg_words[i].english, wg_words[j].english));
    }
    /* Cover smallest/largest banks and many random seeds. */
    for (size_t count = 3; count <= WG_MAX_WORDS; ++count) {
        for (uint32_t seed = 0; seed < 100; ++seed) {
            assert(wg_init(&g, count, seed));
            bool seen[WG_MAX_WORDS] = {false};
            while (g.phase != WG_FINISHED) {
                size_t target = g.order[g.position];
                assert(target < count && !seen[target]); seen[target] = true;
                assert(g.options[g.correct_option] == target);
                for (size_t i = 0; i < WG_OPTIONS; ++i) {
                    assert(g.options[i] < count);
                    for (size_t j = 0; j < i; ++j) assert(g.options[i] != g.options[j]);
                }
                wg_move(&g, -1); assert(g.selected == 2);
                wg_move(&g, 1); assert(g.selected == 0);
                wg_next(&g); assert(g.phase == WG_QUESTION);
                answer(&g, true);
            }
            assert(g.correct == g.length && g.score == 10 + 15*(g.length-1));
            assert(wg_mistake_count(&g) == 0);
            assert(!wg_submit(&g));
            wg_start(&g, true); assert(g.phase == WG_FINISHED && !g.length);
        }
    }
    assert(wg_init(&g, wg_word_count, 42));
    while (g.phase != WG_FINISHED) answer(&g, false);
    assert(wg_mistake_count(&g) == 10 && !g.score);
    wg_start(&g, true);
    while (g.phase != WG_FINISHED) {
        assert(g.mistakes[g.order[g.position]]); answer(&g, true);
    }
    assert(wg_mistake_count(&g) == 0);
    wg_start(&g, false); answer(&g, true); answer(&g, false); answer(&g, true);
    assert(g.score == 20 && g.streak == 1 && g.correct == 2);
    wg_start(&g, false); assert(!g.score && !g.streak && !g.correct);
    assert(wg_mistake_count(&g) == 1);
    puts("PASS: word bank, 12,600 randomized rounds, scoring, input, duplicate submission, review");
}
