#ifndef WORD_GAME_H
#define WORD_GAME_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define WG_MAX_WORDS 128
#define WG_OPTIONS 3
#define WG_ROUND_SIZE 10
typedef struct { const char *english, *chinese, *topic, *definition; } WgWord;
extern const WgWord wg_words[];
extern const size_t wg_word_count;
typedef enum { WG_QUESTION, WG_FEEDBACK, WG_FINISHED } WgPhase;
typedef struct {
    uint32_t random_state;
    size_t count, order[WG_MAX_WORDS], length, position;
    size_t options[WG_OPTIONS], selected, correct_option;
    unsigned score, correct, streak;
    bool mistakes[WG_MAX_WORDS], last_correct;
    WgPhase phase;
} WgGame;
bool wg_init(WgGame *game, size_t count, uint32_t seed);
void wg_start(WgGame *game, bool review);
void wg_move(WgGame *game, int direction);
bool wg_submit(WgGame *game);
void wg_next(WgGame *game);
size_t wg_mistake_count(const WgGame *game);
#endif
