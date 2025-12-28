/*
void* female_ape_thread(void* arg);
// Logic:
- Enter maze
- Collect bananas
- Handle fights with other females
- Rest when tired
- Return to basket
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "family.h"
#include "maze.h"
#include "config.h"
#include "simulation.h"
#include "utils.h"

void* female_ape_thread(void *arg)
{
    FemaleApe *female = (FemaleApe *)arg;
    int family_id = female->family_id;
    
    log_event("Female %d (Family %d) started", female->id, family_id);
    
    while (is_simulation_running() && female->active) {
        
        if (families[family_id].withdrawn) {
            break;
        }
        
        if (female->energy < config.female_rest_threshold) {
             log_event("Female %d is resting (energy: %d)", female->id, female->energy);
            female->resting = true;
            sleep_seconds(config.female_rest_time);
            female->energy = config.female_initial_energy;
            female->resting = false;
            log_event("Female %d finished resting (energy: %d)", female->id, female->energy);
            continue;
        }
        
        female->in_maze = true;
        get_random_empty_position(&female->position_row, &female->position_col);
        log_event("Female %d entered maze at (%d,%d)", 
             female->id, female->position_row, female->position_col);
        
        int collected = 0;
        int attempts = 0;
        int max_attempts = 50;
        
        while (collected < config.female_target_bananas && 
               attempts < max_attempts && 
               is_simulation_running()) {
            
            int new_row = female->position_row + random_int(-1, 1);
            int new_col = female->position_col + random_int(-1, 1);
            
            if (is_cell_accessible(new_row, new_col)) {
                female->position_row = new_row;
                  female->position_col = new_col;
                
                int bananas_here = collect_bananas_from_cell(new_row, new_col, 
                                   config.female_target_bananas - collected);
                
                if (bananas_here > 0) {
                    collected += bananas_here;
                    female->energy -= 2;
                    log_event("Female %d collected %d bananas at (%d,%d) - Total: %d",
                             female->id, bananas_here, new_row, new_col, collected);
                }
                 }
            
            attempts++;
            sleep_milliseconds(100);
        }
        
        female->in_maze = false;
        
        if (collected > 0) {
            add_to_basket(family_id, collected);
            female->bananas_collected += collected;
            log_event("Female %d delivered %d bananas to Family %d basket (Total in basket: %d)",
                     female->id, collected, family_id, get_family_total_bananas(family_id));
        }
         
        female->energy -= 5;
        
        sleep_seconds(1);
    }
    
    log_event("Female %d (Family %d) stopped", female->id, family_id);
    return NULL;
}


