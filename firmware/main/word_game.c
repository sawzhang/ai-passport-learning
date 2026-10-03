#include "word_game.h"
#include <string.h>
static uint32_t random_value(WgGame *g) {
    uint32_t x = g->random_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return g->random_state = x;
}
static void question(WgGame *g) {
    size_t target = g->order[g->position];
    g->correct_option = random_value(g) % WG_OPTIONS;
    /* Choose distinct distractors in bounded time, even with a tiny dictionary. */
    size_t pool[WG_MAX_WORDS], n = 0;
    for (size_t i = 0; i < g->count; ++i) if (i != target) pool[n++] = i;
    for (size_t i = 0; i < WG_OPTIONS; ++i) {
        if (i == g->correct_option) { g->options[i] = target; continue; }
        size_t j = random_value(g) % n;
        g->options[i] = pool[j]; pool[j] = pool[--n];
    }
    g->selected = 0; g->phase = WG_QUESTION;
}
bool wg_init(WgGame *g, size_t count, uint32_t seed) {
    if (!g || count < WG_OPTIONS || count > WG_MAX_WORDS) return false;
    memset(g, 0, sizeof(*g));
    g->count = count; g->random_state = seed ? seed : 1;
    wg_start(g, false); return true;
}
void wg_start(WgGame *g, bool review) {
    g->length = 0; g->position = 0;
    g->score = g->correct = g->streak = 0;
    g->last_correct = false;
    for (size_t i = 0; i < g->count; ++i)
        if (!review || g->mistakes[i]) g->order[g->length++] = i;
    for (size_t i = g->length; i > 1; --i) {
        size_t j = random_value(g) % i, temp = g->order[i-1];
        g->order[i-1] = g->order[j]; g->order[j] = temp;
    }
    if (g->length > WG_ROUND_SIZE) g->length = WG_ROUND_SIZE;
    if (g->length) question(g); else g->phase = WG_FINISHED;
}
void wg_move(WgGame *g, int direction) {
    if (g->phase != WG_QUESTION || !direction) return;
    g->selected = (g->selected + (direction > 0 ? 1 : WG_OPTIONS-1)) % WG_OPTIONS;
}
bool wg_submit(WgGame *g) {
    if (g->phase != WG_QUESTION) return false;
    g->last_correct = g->selected == g->correct_option;
    size_t target = g->order[g->position];
    if (g->last_correct) {
        ++g->correct; ++g->streak;
        g->score += 10 + (g->streak > 1 ? 5 : 0);
        g->mistakes[target] = false;
    } else { g->streak = 0; g->mistakes[target] = true; }
    g->phase = WG_FEEDBACK; return true;
}
void wg_next(WgGame *g) {
    if (g->phase != WG_FEEDBACK) return;
    if (++g->position == g->length) g->phase = WG_FINISHED;
    else question(g);
}
size_t wg_mistake_count(const WgGame *g) {
    size_t n = 0;
    for (size_t i = 0; i < g->count; ++i) n += g->mistakes[i];
    return n;
}
