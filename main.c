#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "config.h"
#include "maze.h"
#include "family.h"
#include "simulation.h"
#include "utils.h"
#include "graphics.h"

void* female_ape_thread(void *arg);
void* male_ape_thread(void *arg);
void* baby_ape_thread(void *arg);
void* simulation_monitor_thread(void *arg);

void* simulation_monitor_thread(void *arg)
{
    (void)arg;
    while (is_simulation_running()) {
        sleep(3);

        if (check_termination_conditions()) {
            stop_simulation();
            break;
        }

        print_simulation_stats();
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    const char *config_file = (argc > 1) ? argv[1] : "config.txt";
    printf("Reading configuration from: %s\n", config_file);

    read_config(config_file);
    print_config();

    init_maze();
    init_families();
    init_simulation();

    print_maze();

    printf("\n===== STARTING THREADS =====\n");

    for (int i = 0; i < total_families; i++) {

        pthread_create(&families[i].female->thread, NULL,
                       female_ape_thread, families[i].female);

        pthread_create(&families[i].male->thread, NULL,
                       male_ape_thread, families[i].male);

        for (int j = 0; j < families[i].num_babies; j++) {
            pthread_create(&families[i].babies[j].thread, NULL,
                           baby_ape_thread, &families[i].babies[j]);
        }
    }

    pthread_t sim_monitor;
    pthread_create(&sim_monitor, NULL, simulation_monitor_thread, NULL);

    printf("Press ESC or 'q' in graphics window to quit\n");

    /* Graphics (blocking) */
    init_graphics(argc, argv);
    start_graphics();

    /* After glutLeaveMainLoop() */
    stop_simulation();

    pthread_join(sim_monitor, NULL);

    printf("\nWaiting for threads...\n");

    for (int i = 0; i < total_families; i++) {
        pthread_join(families[i].female->thread, NULL);
        pthread_join(families[i].male->thread, NULL);

        for (int j = 0; j < families[i].num_babies; j++) {
            pthread_join(families[i].babies[j].thread, NULL);
        }
    }

    printf("\nFINAL RESULTS\n");
    print_simulation_stats();
    print_family_stats();
    print_maze_stats();

    cleanup_simulation();
    cleanup_families();
    cleanup_maze();

    printf("\nSimulation completed cleanly.\n");
    return 0;
}
