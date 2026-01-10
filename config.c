#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

config_t config;

static void set_default_config(void)
{
    config.maze_rows = 15;
    config.maze_cols = 15;
    config.num_families = 3;
    config.num_females_per_family = 1;
    config.num_babies_per_family = 1;
    config.max_simulation_time = 120;

    config.obstacle_percentage = 15;
    config.banana_cell_percentage = 25;
    config.max_bananas_per_cell = 5;

    config.family_max_bananas = 40;
    config.max_withdrawn_families = 2;
    config.max_baby_eaten = 10;

    config.female_initial_energy = 100;
    config.female_collect_cost = 2;
    config.female_trip_end_cost = 5;
    config.female_fight_win_cost = 5;
    config.female_fight_lose_cost = 10;
    config.female_rest_threshold = 25;
    config.female_rest_gain = 20;
    config.female_target_bananas = 6;
    config.female_max_capacity = 10;

    config.male_initial_energy = 120;
    config.male_idle_cost = 1;
    config.male_fight_win_cost = 10;
    config.male_fight_lose_cost = 20;
    config.male_withdraw_threshold = 30;

    config.baby_eat_rate = 2;

    config.base_fight_probability = 0.05f;
    config.banana_fight_factor = 0.01f;
}

void read_config(const char *filename)
{
    set_default_config();

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open config file, using defaults");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || strlen(line) < 3)
            continue;

        sscanf(line, "maze_rows=%d", &config.maze_rows);
        sscanf(line, "maze_cols=%d", &config.maze_cols);
        sscanf(line, "num_families=%d", &config.num_families);
        sscanf(line, "num_females_per_family=%d", &config.num_females_per_family);
        sscanf(line, "num_babies_per_family=%d", &config.num_babies_per_family);
        sscanf(line, "max_simulation_time=%d", &config.max_simulation_time);

        sscanf(line, "obstacle_percentage=%d", &config.obstacle_percentage);
        sscanf(line, "banana_cell_percentage=%d", &config.banana_cell_percentage);
        sscanf(line, "max_bananas_per_cell=%d", &config.max_bananas_per_cell);

        sscanf(line, "family_max_bananas=%d", &config.family_max_bananas);
        sscanf(line, "max_withdrawn_families=%d", &config.max_withdrawn_families);
        sscanf(line, "max_baby_eaten=%d", &config.max_baby_eaten);

        sscanf(line, "female_initial_energy=%d", &config.female_initial_energy);
        sscanf(line, "female_collect_cost=%d", &config.female_collect_cost);
        sscanf(line, "female_trip_end_cost=%d", &config.female_trip_end_cost);
        sscanf(line, "female_fight_win_cost=%d", &config.female_fight_win_cost);
        sscanf(line, "female_fight_lose_cost=%d", &config.female_fight_lose_cost);
        sscanf(line, "female_rest_threshold=%d", &config.female_rest_threshold);
        sscanf(line, "female_rest_gain=%d", &config.female_rest_gain);
        sscanf(line, "female_target_bananas=%d", &config.female_target_bananas);
        sscanf(line, "female_max_capacity=%d", &config.female_max_capacity);

        sscanf(line, "male_initial_energy=%d", &config.male_initial_energy);
        sscanf(line, "male_idle_cost=%d", &config.male_idle_cost);
        sscanf(line, "male_fight_win_cost=%d", &config.male_fight_win_cost);
        sscanf(line, "male_fight_lose_cost=%d", &config.male_fight_lose_cost);
        sscanf(line, "male_withdraw_threshold=%d", &config.male_withdraw_threshold);

        sscanf(line, "baby_eat_rate=%d", &config.baby_eat_rate);

        sscanf(line, "base_fight_probability=%f", &config.base_fight_probability);
        sscanf(line, "banana_fight_factor=%f", &config.banana_fight_factor);
    }

    fclose(file);
}

void print_config(void)
{
    printf("\n========= CONFIG =========\n");
    printf("Maze: %dx%d\n", config.maze_rows, config.maze_cols);
    printf("Families: %d (Females/family=%d, Babies/family=%d)\n",
           config.num_families, config.num_females_per_family, config.num_babies_per_family);
    printf("Max simulation time: %d s\n", config.max_simulation_time);

    printf("Obstacle %%: %d\n", config.obstacle_percentage);
    printf("Banana cell %%: %d\n", config.banana_cell_percentage);
    printf("Max bananas/cell: %d\n", config.max_bananas_per_cell);

    printf("Stop: family_max_bananas=%d, max_withdrawn=%d, max_baby_eaten=%d\n",
           config.family_max_bananas, config.max_withdrawn_families, config.max_baby_eaten);

    printf("Female: E0=%d, collect=%d, trip_end=%d, win=%d, lose=%d, rest_th=%d, rest_gain=%d, target=%d\n",
           config.female_initial_energy, config.female_collect_cost, config.female_trip_end_cost,
           config.female_fight_win_cost, config.female_fight_lose_cost,
           config.female_rest_threshold, config.female_rest_gain, config.female_target_bananas);

    printf("Male: E0=%d, idle=%d, win=%d, lose=%d, withdraw_th=%d\n",
           config.male_initial_energy, config.male_idle_cost,
           config.male_fight_win_cost, config.male_fight_lose_cost,
           config.male_withdraw_threshold);

    printf("Baby eat rate: %d\n", config.baby_eat_rate);

    printf("Fight prob: base=%.3f, factor=%.3f\n",
           config.base_fight_probability, config.banana_fight_factor);

    printf("==========================\n\n");
}
