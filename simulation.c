/*
// Global simulation state
- Start/stop control
- Termination condition checking
- Time tracking
- Statistics collection

// Functions:
- start_simulation()
- check_termination_conditions()
- print_statistics()
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "simulation.h"
#include "family.h"
#include "maze.h"
#include "config.h"

SimulationState sim_state;
bool simulation_done = false;

void init_simulation(void)
{
    sim_state.running = true;
    sim_state.start_time = time(NULL);
    sim_state.withdrawn_families_count = 0;
    pthread_mutex_init(&sim_state.simulation_mutex, NULL);
    
    printf("\n========================================\n");
    printf("   SIMULATION STARTED\n");
    printf("========================================\n");
    printf("Start Time: %s", ctime(&sim_state.start_time));
    printf("========================================\n\n");
}

void cleanup_simulation(void)
{
    pthread_mutex_destroy(&sim_state.simulation_mutex);
    printf("Simulation cleanup completed.\n");
}

bool is_simulation_running(void)
{
    pthread_mutex_lock(&sim_state.simulation_mutex);
    bool running = sim_state.running;
    pthread_mutex_unlock(&sim_state.simulation_mutex);
    return running;
}

void stop_simulation(void)
{
    pthread_mutex_lock(&sim_state.simulation_mutex);
    sim_state.running = false;
    pthread_mutex_unlock(&sim_state.simulation_mutex);
    
    printf("\n========================================\n");
    printf("   SIMULATION STOPPED\n");
    printf("========================================\n\n");

    simulation_done = true;
}

int get_elapsed_time(void)
{
    return (int)difftime(time(NULL), sim_state.start_time);
}

bool check_termination_conditions(void)
{
    if (get_elapsed_time() >= config.max_simulation_time) {
        printf("\n>>> Termination: Time limit reached (%d seconds)\n", 
               config.max_simulation_time);
        return true;
    }
    
    int withdrawn_count = 0;
    for (int i = 0; i < total_families; i++) {
        if (families[i].withdrawn) {
            withdrawn_count++;
        }
    }
    
    if (withdrawn_count >= config.max_withdrawn_families) {
        printf("\n>>> Termination: Too many withdrawn families (%d)\n", 
               withdrawn_count);
        return true;
    }
    
    for (int i = 0; i < total_families; i++) {
        int bananas = get_family_total_bananas(i);
        if (bananas >= config.family_max_bananas) {
            printf("\n>>> Termination: Family %d has %d bananas (limit: %d)\n",
                   i, bananas, config.family_max_bananas);
            return true;
        }
    }
    
    for (int i = 0; i < total_families; i++) {
        for (int j = 0; j < families[i].num_babies; j++) {
            if (families[i].babies[j].bananas_eaten >= config.max_baby_eaten) {
                printf("\n>>> Termination: Baby %d from Family %d ate %d bananas\n",
                       j, i, families[i].babies[j].bananas_eaten);
                return true;
            }
        }
    }
    
    return false;
}

void print_simulation_stats(void)
{
    time_t current_time = time(NULL);
    int elapsed = get_elapsed_time();
    
    printf("\n========================================\n");
    printf("     SIMULATION STATISTICS\n");
    printf("========================================\n");
    printf("Elapsed Time: %d seconds\n", elapsed);
    printf("Current Time: %s", ctime(&current_time));
    printf("----------------------------------------\n");
    
    int active_families = 0;
    int withdrawn_families = 0;
    int total_collected = 0;
    
    for (int i = 0; i < total_families; i++) {
        if (families[i].withdrawn) {
            withdrawn_families++;
        } else {
            active_families++;
        }
        total_collected += get_family_total_bananas(i);
    }
    
    printf("Active Families: %d\n", active_families);
    printf("Withdrawn Families: %d\n", withdrawn_families);
    printf("Total Bananas Collected: %d\n", total_collected);
    printf("Bananas Remaining in Maze: %d\n", count_total_bananas());
    printf("========================================\n\n");
}

bool any_family_active(void)
{
    for (int i = 0; i < total_families; i++) {
        if (!families[i].withdrawn) {
            return true;
        }
    }
    return false;
}
