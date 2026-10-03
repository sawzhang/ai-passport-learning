#include "tetris_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t rng = 0x12345678;
static uint32_t next(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
static void invariant(const tetris_model_t *g) {
    for (int y = 0; y < 20; ++y) for (int x = 0; x < 10; ++x)
        assert(g->board[y][x] <= TETRIS_PIECE_L);
    assert(g->piece >= TETRIS_PIECE_I && g->piece <= TETRIS_PIECE_L);
    assert(g->rotation < 4 && g->bag_index <= 7);
    assert(tetris_model_drop_interval_ms(g) >= 150);
    if (g->state != TETRIS_GAME_OVER) {
        int count = 0;
        for (int y = 0; y < 4; ++y) for (int x = 0; x < 4; ++x) {
            if (!tetris_piece_cell(g->piece, g->rotation, x, y)) continue;
            ++count;
            int bx = g->x + x, by = g->y + y;
            assert(bx >= 0 && bx < 10 && by >= 0 && by < 20);
            assert(g->board[by][bx] == 0);
        }
        assert(count == 4);
        assert(tetris_model_ghost_y(g) >= g->y);
    }
}
int main(void) {
    tetris_model_t g;
    for (uint32_t seed = 0; seed < 1000; ++seed) {
        tetris_model_init(&g, seed);
        for (int i = 0; i < 1000; ++i) {
            invariant(&g);
            if (g.state == TETRIS_GAME_OVER) { tetris_model_reset(&g); continue; }
            unsigned score = g.score;
            switch (next() % 6) {
            case 0: tetris_model_move(&g, -1); break;
            case 1: tetris_model_move(&g, 1); break;
            case 2: tetris_model_rotate(&g); break;
            case 3: tetris_model_step(&g); break;
            case 4: tetris_model_hard_drop(&g); break;
            default: {
                tetris_model_toggle_pause(&g);
                tetris_model_t before = g;
                assert(!tetris_model_move(&g, 1)); assert(!tetris_model_rotate(&g));
                assert(tetris_model_step(&g) == TETRIS_EVENT_NONE);
                assert(tetris_model_hard_drop(&g) == TETRIS_EVENT_NONE);
                assert(memcmp(&before, &g, sizeof(g)) == 0);
                tetris_model_toggle_pause(&g); break;
            }
            }
            assert(g.score >= score); invariant(&g);
        }
    }
    tetris_model_init(&g, 7);
    memset(g.board, 0, sizeof(g.board));
    for (int y = 16; y < 20; ++y) for (int x = 0; x < 10; ++x)
        if (x != 5) g.board[y][x] = TETRIS_PIECE_J;
    g.piece = TETRIS_PIECE_I; g.rotation = 1; g.x = 3; g.y = 16;
    assert(tetris_model_hard_drop(&g) == TETRIS_EVENT_LINE_CLEAR);
    assert(g.lines == 4 && g.score == 800);
    for (int y = 0; y < 20; ++y) for (int x = 0; x < 10; ++x) assert(!g.board[y][x]);
    puts("PASS: 1,000,000 random actions; board bounds, collision, pause, score, four-line clear");
}
