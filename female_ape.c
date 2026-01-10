#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

#include "family.h"
#include "maze.h"
#include "config.h"
#include "simulation.h"
#include "utils.h"

/**
 * Calculate Manhattan distance between two positions
 * Used for pathfinding and proximity detection
 */
static int manhattan_distance(int r1, int c1, int r2, int c2)
{
    return abs(r1 - r2) + abs(c1 - c2);
}

/**
 * Find nearest banana cell within sight range
 * Returns true if banana found, updates target_r and target_c
 */
static bool find_nearest_banana(int from_r, int from_c, int *target_r, int *target_c, int sight_range)
{
    int best_dist = sight_range + 1;
    bool found = false;

    for (int r = from_r - sight_range; r <= from_r + sight_range; r++) {
        for (int c = from_c - sight_range; c <= from_c + sight_range; c++) {
            if (!is_valid_position(r, c) || !is_cell_accessible(r, c))
                continue;

            if (get_cell_bananas(r, c) > 0) {
                int dist = manhattan_distance(from_r, from_c, r, c);
                if (dist < best_dist) {
                    best_dist = dist;
                    *target_r = r;
                    *target_c = c;
                    found = true;
                }
            }
        }
    }

    return found;
}

/**
 * Move one step toward target using greedy best-first search
 * Returns true if moved, false if stuck
 */
static bool move_toward_target(int *current_r, int *current_c, int target_r, int target_c)
{
    int best_r = *current_r;
    int best_c = *current_c;
    int best_dist = manhattan_distance(*current_r, *current_c, target_r, target_c);

    /* Check all 4 adjacent cells (up, down, left, right) */
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    for (int i = 0; i < 4; i++) {
        int new_r = *current_r + dr[i];
        int new_c = *current_c + dc[i];

        if (is_cell_accessible(new_r, new_c)) {
            int dist = manhattan_distance(new_r, new_c, target_r, target_c);
            if (dist < best_dist) {
                best_dist = dist;
                best_r = new_r;
                best_c = new_c;
            }
        }
    }

    /* If we found a better position, move there */
    if (best_r != *current_r || best_c != *current_c) {
        *current_r = best_r;
        *current_c = best_c;
        return true;
    }

    /* If stuck, try random adjacent cell */
    for (int attempts = 0; attempts < 4; attempts++) {
        int dir = random_int(0, 3);
        int new_r = *current_r + dr[dir];
        int new_c = *current_r + dc[dir];

        if (is_cell_accessible(new_r, new_c)) {
            *current_r = new_r;
            *current_c = new_c;
            return true;
        }
    }

    return false;
}

/**
 * Check if another female is nearby (within radius)
 * Returns pointer to nearby female or NULL
 * IMPORTANT: Only finds females who have bananas worth fighting for
 */
static FemaleApe* find_nearby_female(int my_family_id, int pos_r, int pos_c, int radius, int my_bananas)
{
    /* DON'T look for fights if we have nothing to defend */
    if (my_bananas <= 0) {
        return NULL;
    }
    
    for (int f = 0; f < total_families; f++) {
        if (f == my_family_id || families[f].withdrawn)
            continue;

        FemaleApe *other = families[f].female;
        if (!other || !other->active || !other->in_maze)
            continue;

        /* Only fight if opponent has bananas too */
        if (other->bananas_collected <= 0)
            continue;

        int dist = manhattan_distance(pos_r, pos_c, 
                                     other->position_row, other->position_col);
        
        if (dist <= radius) {
            return other;
        }
    }
    return NULL;
}

/**
 * Female Ape Thread - Main behavior loop
 * 
 * Behavior:
 * - Intelligently searches for bananas using pathfinding
 * - Moves step-by-step with realistic travel time
 * - Detects nearby females while exiting (2-unit radius)
 * - Fights over collected bananas with proximity detection
 * - AVOIDS fighting when empty-handed (no infinite loops!)
 * 
 * Movement: Greedy best-first search toward nearest visible banana
 * Sight range: 5 cells
 * Combat range: 2 cells
 */
void* female_ape_thread(void *arg)
{
    FemaleApe *female = (FemaleApe *)arg;
    int family_id = female->family_id;

    log_event("Female %d (Family %d) started", female->id, family_id);

    while (is_simulation_running() && female->active) {

        if (families[family_id].withdrawn)
            break;

        /* Rest if tired */
        if (female->energy < config.female_rest_threshold) {
            log_event("Female %d resting (energy=%d)", female->id, female->energy);
            female->resting = true;

            sleep_seconds(2);

            female->energy += config.female_rest_gain;
            if (female->energy > config.female_initial_energy)
                female->energy = config.female_initial_energy;

            female->resting = false;
            continue;
        }

        /* Enter maze at random position */
        female->in_maze = true;
        female->bananas_collected = 0;
        get_random_empty_position(&female->position_row, &female->position_col);
        
        log_event("Female %d entered maze at (%d,%d)", 
                  female->id, female->position_row, female->position_col);

        int collected = 0;
        int moves_without_banana = 0;
        int sight_range = 5; /* Can see bananas within 5 cells */

        /* INTELLIGENT BANANA COLLECTION with pathfinding */
        while (collected < config.female_target_bananas &&
               moves_without_banana < 30 &&
               is_simulation_running() &&
               !families[family_id].withdrawn) {

            /* Try to find nearest banana */
            int target_r, target_c;
            if (find_nearest_banana(female->position_row, female->position_col, 
                                   &target_r, &target_c, sight_range)) {
                
                /* Move toward banana */
                if (move_toward_target(&female->position_row, &female->position_col,
                                      target_r, target_c)) {
                    
                    /* Try to collect from current cell */
                    int got = collect_bananas_from_cell(
                        female->position_row, female->position_col, 
                        config.female_target_bananas - collected);

                    if (got > 0) {
                        collected += got;
                        female->bananas_collected = collected;
                        female->energy -= config.female_collect_cost;
                        moves_without_banana = 0;
                        
                        log_event("Female %d collected %d bananas at (%d,%d), total=%d",
                                  female->id, got, female->position_row, 
                                  female->position_col, collected);
                    } else {
                        moves_without_banana++;
                    }
                }
            } else {
                /* No banana in sight, explore randomly */
                move_toward_target(&female->position_row, &female->position_col,
                                  random_int(0, config.maze_rows - 1),
                                  random_int(0, config.maze_cols - 1));
                moves_without_banana++;
            }

            /* Movement takes time - realistic travel delay */
            sleep_milliseconds(150);
        }

        /* EXIT MAZE with realistic travel time and fight detection */
        if (collected > 0 && is_simulation_running()) {
            log_event("Female %d exiting maze with %d bananas", female->id, collected);
            
            /* Move toward exit (bottom of maze, near family position) */
            int exit_row = config.maze_rows - 1;
            int exit_col = female->family_id * 3;
            
            int exit_steps = 0;
            int max_exit_steps = 100; /* Prevent infinite loop */
            
            while ((female->position_row != exit_row || 
                    abs(female->position_col - exit_col) > 2) &&
                   is_simulation_running() &&
                   !families[family_id].withdrawn &&
                   exit_steps < max_exit_steps &&
                   collected > 0) { /* Stop trying to fight if we lose all bananas */
                
                exit_steps++;
                
                /* Check for nearby females while exiting - THREAD SAFE */
                /* IMPORTANT: find_nearby_female now checks if we have bananas! */
                FemaleApe *nearby = find_nearby_female(family_id, 
                                                      female->position_row,
                                                      female->position_col, 2,
                                                      collected);
                
                if (nearby && !female->fighting && !nearby->fighting) {
                    /* Lock both females' fight flags to prevent race condition */
                    int first_id = (family_id < nearby->family_id) ? family_id : nearby->family_id;
                    int second_id = (family_id < nearby->family_id) ? nearby->family_id : family_id;
                    
                    pthread_mutex_lock(&families[first_id].fight_mutex);
                    pthread_mutex_lock(&families[second_id].fight_mutex);
                    
                    /* Double-check still valid after acquiring locks */
                    if (!female->fighting && !nearby->fighting && 
                        nearby->in_maze && nearby->bananas_collected > 0 && collected > 0) {
                        
                        female->fighting = true;
                        nearby->fighting = true;
                        
                        log_event("Female %d fighting Female %d at (%d,%d) (%d vs %d bananas)",
                                  female->id, nearby->id,
                                  female->position_row, female->position_col,
                                  collected, nearby->bananas_collected);
                        
                        sleep_milliseconds(500); /* Fight takes time */

                        int my_score = female->energy + random_int(0, 50);
                        int ot_score = nearby->energy + random_int(0, 50);

                        if (my_score >= ot_score) {
                            /* Win: keep bananas */
                            female->energy -= config.female_fight_win_cost;
                            nearby->energy -= config.female_fight_lose_cost;
                            log_event("Female %d won fight! (kept %d bananas)", 
                                      female->id, collected);
                        } else {
                            /* Lose: lose some bananas */
                            int lost = collected / 2;
                            collected -= lost;
                            female->bananas_collected = collected;
                            female->energy -= config.female_fight_lose_cost;
                            nearby->energy -= config.female_fight_win_cost;
                            nearby->bananas_collected += lost;
                            log_event("Female %d lost fight, lost %d bananas (left with %d)", 
                                      female->id, lost, collected);
                        }

                        female->fighting = false;
                        nearby->fighting = false;
                    }
                    
                    pthread_mutex_unlock(&families[second_id].fight_mutex);
                    pthread_mutex_unlock(&families[first_id].fight_mutex);
                }
                
                /* Continue moving toward exit */
                move_toward_target(&female->position_row, &female->position_col,
                                  exit_row, exit_col);
                sleep_milliseconds(150); /* Travel time */
            }
            
            /* Check if we got stuck in infinite loop */
            if (exit_steps >= max_exit_steps) {
                log_event("Female %d couldn't reach exit, teleporting out", female->id);
            }
            
            /* If we lost all bananas, note it */
            if (collected == 0) {
                log_event("Female %d lost all bananas in fights, fleeing empty-handed", 
                          female->id);
            }
        }

        female->in_maze = false;

        /* Deliver bananas to basket */
        if (collected > 0 && !families[family_id].withdrawn) {
            add_to_basket(family_id, collected);
            log_event("Female %d delivered %d bananas to basket", 
                      female->id, collected);
        } else if (collected == 0 && female->bananas_collected > 0) {
            log_event("Female %d returned empty-handed (lost all in fights)", female->id);
        }

        female->bananas_collected = 0;
        female->energy -= config.female_trip_end_cost;

        sleep_seconds(1);
    }

    log_event("Female %d stopped", female->id);
    return NULL;
}