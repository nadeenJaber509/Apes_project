#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

Config config;

void read_config(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open config file");
        exit(1);
    }

    char key[64];
    char eq[8];
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        // Ignore comments and empty lines
        if (line[0] == '#' || line[0] == '\n')
            continue;

        sscanf(line, "%63s %7s", key, eq);

        if (strcmp(key, "maze_rows") == 0)
            sscanf(line, "%*s %*s %d", &config.maze_rows);
        else if (strcmp(key, "maze_cols") == 0)
            sscanf(line, "%*s %*s %d", &config.maze_cols);
        else if (strcmp(key, "obstacle_percentage") == 0)
            sscanf(line, "%*s %*s %d", &config.obstacle_percentage);
        else if (strcmp(key, "max_bananas_per_cell") == 0)
            sscanf(line, "%*s %*s %d", &config.max_bananas_per_cell);
        else if (strcmp(key, "banana_cell_percentage") == 0)
            sscanf(line, "%*s %*s %d", &config.banana_cell_percentage);
        else if (strcmp(key, "num_families") == 0)
            sscanf(line, "%*s %*s %d", &config.num_families);
        else if (strcmp(key, "babies_per_family") == 0)
            sscanf(line, "%*s %*s %d", &config.babies_per_family);
        else if (strcmp(key, "female_target_bananas") == 0)
            sscanf(line, "%*s %*s %d", &config.female_target_bananas);
        else if (strcmp(key, "female_initial_energy") == 0)
            sscanf(line, "%*s %*s %d", &config.female_initial_energy);
        else if (strcmp(key, "female_rest_threshold") == 0)
            sscanf(line, "%*s %*s %d", &config.female_rest_threshold);
        else if (strcmp(key, "female_rest_time") == 0)
            sscanf(line, "%*s %*s %d", &config.female_rest_time);
        else if (strcmp(key, "male_initial_energy") == 0)
            sscanf(line, "%*s %*s %d", &config.male_initial_energy);
        else if (strcmp(key, "male_fight_energy_threshold") == 0)
            sscanf(line, "%*s %*s %d", &config.male_fight_energy_threshold);
        else if (strcmp(key, "base_fight_probability") == 0)
            sscanf(line, "%*s %*s %f", &config.base_fight_probability);
        else if (strcmp(key, "banana_fight_factor") == 0)
            sscanf(line, "%*s %*s %f", &config.banana_fight_factor);
        else if (strcmp(key, "baby_eat_rate") == 0)
            sscanf(line, "%*s %*s %d", &config.baby_eat_rate);
        else if (strcmp(key, "baby_max_eat") == 0)
            sscanf(line, "%*s %*s %d", &config.baby_max_eat);
        else if (strcmp(key, "max_withdrawn_families") == 0)
            sscanf(line, "%*s %*s %d", &config.max_withdrawn_families);
        else if (strcmp(key, "max_family_bananas") == 0)
            sscanf(line, "%*s %*s %d", &config.max_family_bananas);
        else if (strcmp(key, "max_baby_eaten") == 0)
            sscanf(line, "%*s %*s %d", &config.max_baby_eaten);
        else if (strcmp(key, "max_simulation_time") == 0)
            sscanf(line, "%*s %*s %d", &config.max_simulation_time);
    }

    fclose(fp);
}

void print_config(void)
{
    printf("========================================\n");
    printf("     SIMULATION CONFIGURATION\n");
    printf("========================================\n\n");
    
    printf("Maze Configuration:\n");
    printf("  Rows: %d\n", config.maze_rows);
    printf("  Columns: %d\n", config.maze_cols);
    printf("  Obstacle Percentage: %d%%\n", config.obstacle_percentage);
    printf("  Max Bananas per Cell: %d\n", config.max_bananas_per_cell);
    printf("  Banana Cell Percentage: %d%%\n", config.banana_cell_percentage);

    printf("\nFamilies Configuration:\n");
    printf("  Number of Families: %d\n", config.num_families);
    printf("  Babies per Family: %d\n", config.babies_per_family);

    printf("\nFemale Apes Configuration:\n");
    printf("  Target Bananas: %d\n", config.female_target_bananas);
    printf("  Initial Energy: %d\n", config.female_initial_energy);
    printf("  Rest Threshold: %d\n", config.female_rest_threshold);
    printf("  Rest Time: %d seconds\n", config.female_rest_time);

    printf("\nMale Apes Configuration:\n");
    printf("  Initial Energy: %d\n", config.male_initial_energy);
    printf("  Fight Energy Threshold: %d\n", config.male_fight_energy_threshold);

    printf("\nMale Fights Configuration:\n");
    printf("  Base Fight Probability: %.2f\n", config.base_fight_probability);
    printf("  Banana Fight Factor: %.2f\n", config.banana_fight_factor);

    printf("\nBaby Apes Configuration:\n");
    printf("  Eat Rate: %d bananas/steal\n", config.baby_eat_rate);
    printf("  Max Baby Eaten: %d\n", config.baby_max_eat);

    printf("\nTermination Conditions:\n");
    printf("  Max Withdrawn Families: %d\n", config.max_withdrawn_families);
    printf("  Max Family Bananas: %d\n", config.max_family_bananas);
    printf("  Max Baby Eaten: %d\n", config.max_baby_eaten);
    printf("  Max Simulation Time: %d seconds\n", config.max_simulation_time);
    
    printf("\n========================================\n\n");
}