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
        female->bananas_collected = 0;  // Reset for this trip
        get_random_empty_position(&female->position_row, &female->position_col);
        log_event("Female %d entered maze at (%d,%d)", 
             female->id, female->position_row, female->position_col);
        
        int collected = 0;
        int attempts = 0;
        int max_attempts = 50;
        
        // Phase 1: Collect bananas (no fighting during collection)
        while (collected < config.female_target_bananas && 
               attempts < max_attempts && 
               is_simulation_running() &&
               !families[family_id].withdrawn) {
            
            int new_row = female->position_row + random_int(-1, 1);
            int new_col = female->position_col + random_int(-1, 1);
            
            if (is_cell_accessible(new_row, new_col)) {
                female->position_row = new_row;
                female->position_col = new_col;
                
                int bananas_here = collect_bananas_from_cell(new_row, new_col, 
                                   config.female_target_bananas - collected);
                
                if (bananas_here > 0) {
                    collected += bananas_here;
                    female->bananas_collected = collected;
                    female->energy -= 2;
                    log_event("Female %d collected %d bananas at (%d,%d) - Total: %d",
                             female->id, bananas_here, new_row, new_col, collected);
                }
            }
            
            attempts++;
            sleep_milliseconds(100);
        }
        
        // Phase 2: Leaving the maze - fights can happen here!
        // Female moves a few steps to "exit" and may encounter other females
        if (collected > 0 && is_simulation_running() && !families[family_id].withdrawn) {
            log_event("Female %d leaving maze with %d bananas", female->id, collected);
            
            int exit_steps = random_int(3, 8);  // Random exit path length
            for (int step = 0; step < exit_steps && is_simulation_running() && 
                 !families[family_id].withdrawn; step++) {
                
                int new_row = female->position_row + random_int(-1, 1);
                int new_col = female->position_col + random_int(-1, 1);
                
                if (is_cell_accessible(new_row, new_col)) {
                    female->position_row = new_row;
                    female->position_col = new_col;
                    
                    // Check for other females on the way out - potential fight!
                    for (int f = 0; f < total_families; f++) {
                        if (f == family_id || families[f].withdrawn) continue;
                        FemaleApe *other = families[f].female;
                        if (other && other->active && other->in_maze &&
                            other->position_row == new_row && 
                            other->position_col == new_col &&
                            !female->fighting && !other->fighting &&
                            (collected > 0 || other->bananas_collected > 0)) {
                            
                            // Fight over collected bananas!
                            female->fighting = true;
                            other->fighting = true;
                            
                            log_event("FIGHT: Female %d (Family %d, %d bananas) vs Female %d (Family %d, %d bananas) - on exit!",
                                     female->id, family_id, collected,
                                     other->id, f, other->bananas_collected);
                            
                            // Fight duration
                            sleep_milliseconds(500);
                            
                            // Determine winner (random + energy factor)
                            int winner = (random_int(0, 100) + female->energy > 
                                         random_int(0, 100) + other->energy) ? 0 : 1;
                            
                            if (winner == 0) {
                                // This female wins - steal bananas from the other
                                int stolen = other->bananas_collected > 0 ? 
                                            random_int(1, other->bananas_collected) : 0;
                                if (stolen > 0) {
                                    other->bananas_collected -= stolen;
                                    collected += stolen;
                                    female->bananas_collected = collected;
                                    log_event("Female %d WON and stole %d bananas from Female %d (now has %d)",
                                             female->id, stolen, other->id, collected);
                                } else {
                                    log_event("Female %d WON but Female %d had no bananas to steal",
                                             female->id, other->id);
                                }
                                female->energy -= 5;
                                other->energy -= 10;
                            } else {
                                // Other female wins - loses bananas
                                int stolen = collected > 0 ? random_int(1, collected) : 0;
                                if (stolen > 0) {
                                    collected -= stolen;
                                    female->bananas_collected = collected;
                                    other->bananas_collected += stolen;
                                    log_event("Female %d LOST and gave %d bananas to Female %d (now has %d)",
                                             female->id, stolen, other->id, collected);
                                } else {
                                    log_event("Female %d LOST but had no bananas to give",
                                             female->id);
                                }
                                female->energy -= 10;
                                other->energy -= 5;
                            }
                            
                            sleep_milliseconds(500);
                            female->fighting = false;
                            other->fighting = false;
                            break;
                        }
                    }
                }
                
                sleep_milliseconds(150);
            }
        }
        
        female->in_maze = false;
        
        // Deliver bananas to basket (protected by male)
        if (collected > 0 && !families[family_id].withdrawn) {
            add_to_basket(family_id, collected);
            log_event("Female %d delivered %d bananas to Family %d basket (Total in basket: %d)",
                     female->id, collected, family_id, get_family_total_bananas(family_id));
        }
        
        female->bananas_collected = 0;  // Reset after delivery
        female->energy -= 5;
        
        sleep_seconds(1);
    }
    
    log_event("Female %d (Family %d) stopped", female->id, family_id);
    return NULL;
}


