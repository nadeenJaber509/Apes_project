#ifndef FAMILY_H
#define FAMILY_H

#include <pthread.h>
#include <stdbool.h>

typedef struct BabyApe BabyApe;
typedef struct FemaleApe FemaleApe;
typedef struct MaleApe MaleApe;

struct BabyApe {
    int id;
    int family_id;
    int bananas_eaten;
    int position_row;
    int position_col;
    pthread_t thread;
    bool active;
};

struct FemaleApe {
    int id;
    int family_id;
    int energy;
    int bananas_collected;
    int position_row;
    int position_col;
    bool in_maze;
    bool resting;
    bool fighting;
    pthread_t thread;
    bool active;
};

struct MaleApe {
    int id;
    int family_id;
    int energy;
    bool fighting;
    int position_row;
    int position_col;
    pthread_t thread;
    bool active;
};

typedef struct {
    int id;
    FemaleApe *female;
    MaleApe *male;
    BabyApe *babies;
    int num_babies;
    int basket_bananas;
    bool withdrawn;
    pthread_mutex_t basket_mutex;
    pthread_mutex_t fight_mutex;
} Family;

extern Family *families;
extern int total_families;

void init_families(void);
void cleanup_families(void);
void withdraw_family(int family_id);
int get_family_total_bananas(int family_id);
void add_to_basket(int family_id, int bananas);
int steal_from_basket(int family_id, int amount);
void print_family_stats(void);

#endif