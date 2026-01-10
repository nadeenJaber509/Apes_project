#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdbool.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    bool running;
    time_t start_time;
    int withdrawn_families_count;
    pthread_mutex_t simulation_mutex;
} SimulationState;

extern SimulationState sim_state;
extern bool simulation_done;

void init_simulation(void);
void cleanup_simulation(void);
bool is_simulation_running(void);
void stop_simulation(void);
bool check_termination_conditions(void);
int get_elapsed_time(void);
void print_simulation_stats(void);
bool any_family_active(void);

#endif