#pragma once

#include <stdint.h>

typedef struct listbox listbox_t;

typedef struct trophy_gameinfo {
    char trophyid[13];
    char name[256];
} trophy_gameinfo_t;

typedef struct trophy_detail {
    unsigned int id;
    int grade;
    int unlocked;
    char name[128];
    char description[128];
} trophy_detail_t;

int trophy_games_list(trophy_gameinfo_t **games);
int trophy_prepare(const char *trophyid, char game_titleid[16]);
void trophy_finish();

int trophy_list(trophy_detail_t **details, int only_unlockable);
int trophy_unlock(trophy_detail_t *details, int index, long int *platid, uint64_t cust_tick);
int trophy_unlock_all(int rand_days, uint64_t cust_tick);
