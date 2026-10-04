/* MIT. Pure ADC-button adapter; bit values match muse_board.h. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
typedef struct { int pressed, candidate; unsigned stable, held; bool talk, aux, back; } passport_keys_t;
static inline void passport_keys_init(passport_keys_t *s) {
    memset(s, 0, sizeof(*s)); s->pressed = s->candidate = -1;
}
static inline int passport_key_mv(int mv) {
    if (mv < 0) return -2;
    if (mv < 150) return 0;
    if (mv < 447) return 1;
    if (mv < 1900) return 2;
    return -1;
}
static inline unsigned passport_keys_poll(passport_keys_t *s, int mv, bool menu) {
    int raw = passport_key_mv(mv);
    if (raw == -2) return 0;
    if (raw != s->candidate) { s->candidate = raw; s->stable = 1; }
    else if (s->stable < 3) ++s->stable;
    unsigned ev = 0;
    if (s->stable >= 3 && raw != s->pressed) {
        if (s->talk) { ev |= 1u << 1; s->talk = false; }
        if (s->aux) { ev |= 1u << 3; s->aux = false; }
        s->pressed = raw; s->held = 0; s->back = false;
        if (raw == 2) {
            if (menu) ev |= 1u << 8;
            else { ev |= 1u; s->talk = true; }
        } else if (raw == 0 || raw == 1) {
            if (menu) ev |= 1u << (raw == 0 ? 4 : 5);
            else { ev |= 1u << 2; s->aux = true; }
        }
    }
    if (s->pressed == 1 && menu && !s->aux && !s->back && ++s->held >= 50) {
        ev |= 1u << 9; s->back = true;
    }
    return ev;
}
