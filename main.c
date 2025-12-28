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

    // ==== GRAPHICS THREAD ====
    init_graphics(argc, argv);

    printf("\n========================================\n");
    printf("   STARTING THREADS\n");
    printf("========================================\n\n");

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

    while (is_simulation_running()) {
        sleep(3);

        if (check_termination_conditions()) {
            stop_simulation();
            break;
        }

        print_simulation_stats();
    }

    printf("\nWaiting for threads...\n");

    for (int i = 0; i < total_families; i++) {
        pthread_join(families[i].female->thread, NULL);
        pthread_join(families[i].male->thread, NULL);

        for (int j = 0; j < families[i].num_babies; j++)
            pthread_join(families[i].babies[j].thread, NULL);
    }

    stop_graphics();

    printf("\nFINAL RESULTS\n");
    print_simulation_stats();
    print_family_stats();
    print_maze_stats();

    cleanup_simulation();
    cleanup_families();
    cleanup_maze();

    printf("\nSimulation completed.\n");
    return 0;
}
