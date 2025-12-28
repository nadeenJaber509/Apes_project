#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    /* Maze */
    int maze_rows;
    int maze_cols;
    int obstacle_percentage;
    int max_bananas_per_cell;
    int banana_cell_percentage;

    /* Families */
    int num_families;
    int babies_per_family;

    /* Female apes */
    int female_target_bananas;
    int female_initial_energy;
    int female_rest_threshold;
    int female_rest_time;

    /* Male apes */
    int male_initial_energy;
    int male_fight_energy_threshold;

    /* Male fights */
    float base_fight_probability;
    float banana_fight_factor;

    /* Baby apes */
    int baby_eat_rate;
    int baby_max_eat;

    /* Termination conditions */
    int max_withdrawn_families;
    int max_family_bananas;
    int max_baby_eaten;
    int max_simulation_time;

} Config;

extern Config config;

void read_config(const char *filename);
void print_config(void);

#endif