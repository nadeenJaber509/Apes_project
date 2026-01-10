#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    /* Maze & Simulation */
    int maze_rows;
    int maze_cols;
    int num_families;
    int num_females_per_family;
    int num_babies_per_family;
    int max_simulation_time;

    /* Maze generation */
    int obstacle_percentage;      // % of cells that are obstacles
    int banana_cell_percentage;   // % of non-obstacle cells that contain bananas
    int max_bananas_per_cell;     // max bananas in a banana cell

    /* Bananas & Stop Conditions */
    int family_max_bananas;
    int max_withdrawn_families;
    int max_baby_eaten;

    /* Female */
    int female_initial_energy;
    int female_collect_cost;
    int female_trip_end_cost;
    int female_fight_win_cost;
    int female_fight_lose_cost;
    int female_rest_threshold;
    int female_rest_gain;
    int female_target_bananas;
    int female_max_capacity;      // Maximum bananas female can carry

    /* Male */
    int male_initial_energy;
    int male_idle_cost;
    int male_fight_win_cost;
    int male_fight_lose_cost;
    int male_withdraw_threshold;

    /* Baby */
    int baby_eat_rate;            // how many bananas baby attempts to steal each time

    /* Fight Probability */
    float base_fight_probability;
    float banana_fight_factor;

} config_t;

extern config_t config;

void read_config(const char *filename);
void print_config(void);

#endif
