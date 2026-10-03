#define _POSIX_C_SOURCE 200809L
#include "tetris_model.h"
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios saved_terminal;
static volatile sig_atomic_t stopping;
static void stop(int signal_number) { (void)signal_number; stopping = 1; }
static void restore(void) {
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &saved_terminal);
    fputs("\033[?25h\033[0m\n", stdout);
}
static uint64_t now_ms(void) {
    struct timespec t;
    (void)clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000 + (uint64_t)t.tv_nsec / 1000000;
}
static void render(const tetris_model_t *g) {
    uint8_t cells[TETRIS_BOARD_HEIGHT][TETRIS_BOARD_WIDTH];
    memcpy(cells, g->board, sizeof(cells));
    int ghost = tetris_model_ghost_y(g);
    for (int pass = 0; pass < 2; ++pass) {
        for (int y = 0; y < 4; ++y) for (int x = 0; x < 4; ++x) {
            if (!tetris_piece_cell(g->piece, g->rotation, x, y)) continue;
            int bx = g->x + x, by = (pass ? g->y : ghost) + y;
            if (bx >= 0 && bx < TETRIS_BOARD_WIDTH && by >= 0 && by < TETRIS_BOARD_HEIGHT)
                cells[by][bx] = pass ? (uint8_t)g->piece : 8;
        }
    }
    printf("\033[HFOLOTOY / TETRIS - Mac host demo\033[K\n");
    printf("Score %-8u Lines %-5u Level %-3u\033[K\n", g->score, g->lines, g->level);
    printf("%-40s\033[K\n", g->state == TETRIS_PAUSED ? "PAUSED" :
           g->state == TETRIS_GAME_OVER ? "GAME OVER - R to restart" : "PLAYING");
    puts("+--------------------+\033[K");
    const int colors[] = {0, 36, 33, 35, 32, 31, 34, 37, 90};
    for (int y = 0; y < TETRIS_BOARD_HEIGHT; ++y) {
        putchar('|');
        for (int x = 0; x < TETRIS_BOARD_WIDTH; ++x) {
            unsigned cell = cells[y][x];
            if (!cell) fputs("  ", stdout);
            else printf("\033[%dm%s\033[0m", colors[cell], cell == 8 ? ".." : "[]");
        }
        puts("|\033[K");
    }
    puts("+--------------------+\033[K");
    puts("A/D move | W rotate | Space drop\033[K");
    puts("P pause/resume | R restart | Q quit\033[K");
    puts("Official C model; host display, no device I/O.\033[K");
    fflush(stdout);
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--help")) {
        puts("Run in a terminal with at least 45 columns and 29 rows.\n"
             "A/D: left/right; W: rotate; Space: drop; P: pause; R: restart; Q: quit.");
        return 0;
    }
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fputs("Interactive terminal required. Use --help for controls.\n", stderr); return 1;
    }
    if (tcgetattr(STDIN_FILENO, &saved_terminal)) { perror("tcgetattr"); return 1; }
    struct termios raw = saved_terminal;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw)) { perror("tcsetattr"); return 1; }
    atexit(restore);
    signal(SIGINT, stop); signal(SIGTERM, stop); signal(SIGHUP, stop);
    fputs("\033[2J\033[?25l", stdout);
    tetris_model_t game;
    tetris_model_init(&game, (uint32_t)time(NULL));
    uint64_t last_drop = now_ms(); render(&game);
    while (!stopping) {
        struct pollfd input = {.fd = STDIN_FILENO, .events = POLLIN};
        int result = poll(&input, 1, 20);
        if (result < 0 && errno != EINTR) break;
        if (input.revents & (POLLHUP | POLLERR | POLLNVAL)) break;
        bool changed = false;
        if (input.revents & POLLIN) {
            char key;
            if (read(STDIN_FILENO, &key, 1) == 1) {
                switch (key) {
                case 'a': case 'A': changed = tetris_model_move(&game, -1); break;
                case 'd': case 'D': changed = tetris_model_move(&game, 1); break;
                case 'w': case 'W': changed = tetris_model_rotate(&game); break;
                case ' ': changed = tetris_model_hard_drop(&game) != TETRIS_EVENT_NONE;
                          last_drop = now_ms(); break;
                case 'p': case 'P': changed = tetris_model_toggle_pause(&game);
                          last_drop = now_ms(); break;
                case 'r': case 'R': tetris_model_reset(&game); changed = true;
                          last_drop = now_ms(); break;
                case 'q': case 'Q': stopping = 1; break;
                default: break;
                }
            }
        }
        uint64_t now = now_ms();
        if (game.state == TETRIS_PLAYING && now - last_drop >= tetris_model_drop_interval_ms(&game)) {
            changed |= tetris_model_step(&game) != TETRIS_EVENT_NONE; last_drop = now;
        }
        if (changed) render(&game);
    }
    return 0;
}
