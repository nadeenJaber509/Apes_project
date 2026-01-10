#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

#include "family.h"
#include "config.h"
#include "simulation.h"
#include "utils.h"
#include "graphics.h"

/**
 * Calculate Euclidean distance between two male apes based on their positions
 * Used for determining if males are neighbors
 */
static float calculate_distance(MaleApe *male1, MaleApe *male2)
{
    int dr = male1->position_row - male2->position_row;
    int dc = male1->position_col - male2->position_col;
    return sqrt(dr * dr + dc * dc);
}

/**
 * Find neighboring male apes within radius
 * Returns number of neighbors found
 */
static int find_neighbors(int my_family_id, int *neighbors, int max_neighbors, float radius)
{
    int count = 0;
    MaleApe *my_male = families[my_family_id].male;
    
    if (!my_male)
        return 0;
    
    for (int i = 0; i < total_families && count < max_neighbors; i++) {
        if (i == my_family_id || families[i].withdrawn)
            continue;
            
        if (!families[i].male || !families[i].male->active)
            continue;
        
        float dist = calculate_distance(my_male, families[i].male);
        
        /* Check if within neighboring radius */
        if (dist <= radius) {
            neighbors[count++] = i;
        }
    }
    
    return count;
}

/**
 * Select fight opponent based on proximity and basket size
 * Weights selection toward opponents with more bananas
 * Returns family_id of opponent or -1 if no valid opponent
 */
static int select_fight_opponent(int my_family_id)
{
    const float NEIGHBOR_RADIUS = 5.0f; /* Males within 5 units are neighbors */
    int neighbors[10];
    int neighbor_count = find_neighbors(my_family_id, neighbors, 10, NEIGHBOR_RADIUS);
    
    if (neighbor_count == 0)
        return -1; /* No neighbors to fight */
    
    /* Weight selection by opponent's basket size */
    int total_weight = 0;
    int weights[10];
    
    for (int i = 0; i < neighbor_count; i++) {
        int opp_bananas = get_family_total_bananas(neighbors[i]);
        /* More bananas = more attractive target */
        weights[i] = opp_bananas + 5; /* +5 base weight so empty baskets still possible */
        total_weight += weights[i];
    }
    
    if (total_weight == 0)
        return -1;
    
    /* Weighted random selection */
    int roll = random_int(1, total_weight);
    int cumulative = 0;
    
    for (int i = 0; i < neighbor_count; i++) {
        cumulative += weights[i];
        if (roll <= cumulative) {
            return neighbors[i];
        }
    }
    
    /* Fallback to first neighbor */
    return neighbors[0];
}

/**
 * Male Ape Thread - Main behavior loop
 * 
 * Behavior:
 * - Guards family basket at specific position
 * - Only fights neighboring males (5-unit radius)
 * - Patrols area with occasional position shifts
 * - Target selection weighted by opponent basket size
 * 
 * Neighbor radius: 5.0 units
 * Fight probability: Base + (bananas × factor)
 * Patrol: Occasional small movements
 */
void* male_ape_thread(void *arg)
{
    MaleApe *male = (MaleApe *)arg;
    int family_id = male->family_id;

    log_event("Male %d (Family %d) started at position (%d,%d)", 
              male->id, family_id, male->position_row, male->position_col);

    while (is_simulation_running() && male->active) {

        if (families[family_id].withdrawn)
            break;

        /* Check if too tired to continue - withdraw family */
        if (male->energy < config.male_withdraw_threshold) {
            log_event("Male %d exhausted (energy=%d), withdrawing family %d",
                      male->id, male->energy, family_id);
            withdraw_family(family_id);
            break;
        }

        int my_bananas = get_family_total_bananas(family_id);

        /* Calculate fight probability based on banana count */
        float fight_prob = config.base_fight_probability +
                           my_bananas * config.banana_fight_factor;

        /* Cap probability at reasonable level */
        if (fight_prob > 0.5f)
            fight_prob = 0.5f;

        /* Decide whether to initiate fight */
        if (random_float(0, 1) < fight_prob) {

            /* PROXIMITY-BASED OPPONENT SELECTION - not random! */
            int opp = select_fight_opponent(family_id);
            
            if (opp >= 0 && 
                !families[opp].withdrawn &&
                families[opp].male &&
                families[opp].male->active) {

                int opp_bananas = get_family_total_bananas(opp);
                
                /* Don't fight if both have nothing to fight for */
                if (my_bananas > 0 || opp_bananas > 0) {
                    
                    float distance = calculate_distance(male, families[opp].male);
                    
                    log_event("Male %d challenging nearby Male %d (distance: %.1f, bananas: %d vs %d)",
                              male->id, families[opp].male->id, distance, my_bananas, opp_bananas);

                    /* Lock in consistent order to prevent deadlock */
                    int first = (family_id < opp) ? family_id : opp;
                    int second = (family_id < opp) ? opp : family_id;
                    
                    pthread_mutex_lock(&families[first].fight_mutex);
                    pthread_mutex_lock(&families[second].fight_mutex);

                    male->fighting = true;
                    families[opp].male->fighting = true;

                    /* Fight takes time - babies can steal during this window */
                    sleep_seconds(2);

                    /* Fight resolution with energy and basket size factors */
                    int my_score = male->energy + random_int(0, 20) + 
                                   (my_bananas / 2); /* More bananas = more motivated */
                    int ot_score = families[opp].male->energy + random_int(0, 20) +
                                   (opp_bananas / 2);

                    if (my_score >= ot_score) {
                        /* Victory! Steal all opponent's bananas */
                        int stolen = steal_from_basket(opp, opp_bananas);
                        add_to_basket(family_id, stolen);
                        male->energy -= config.male_fight_win_cost;
                        if (male->energy < 0) male->energy = 0;
                        families[opp].male->energy -= config.male_fight_lose_cost;
                        if (families[opp].male->energy < 0) families[opp].male->energy = 0;
                        graphics_add_male_fight();
                        
                        log_event("Male %d WON fight vs Male %d, stole %d bananas! (score: %d vs %d)",
                                  male->id, families[opp].male->id, stolen, my_score, ot_score);
                    } else {
                        /* Defeat - lose all bananas */
                        int lost = steal_from_basket(family_id, my_bananas);
                        add_to_basket(opp, lost);
                        male->energy -= config.male_fight_lose_cost;
                        if (male->energy < 0) male->energy = 0;
                        families[opp].male->energy -= config.male_fight_win_cost;
                        if (families[opp].male->energy < 0) families[opp].male->energy = 0;
                        
                        log_event("Male %d LOST fight vs Male %d, lost %d bananas (score: %d vs %d)",
                                  male->id, families[opp].male->id, lost, my_score, ot_score);
                    }

                    male->fighting = false;
                    families[opp].male->fighting = false;

                    pthread_mutex_unlock(&families[second].fight_mutex);
                    pthread_mutex_unlock(&families[first].fight_mutex);
                }
            }
        }

        /* Idle energy cost - guarding the basket is tiring */
        male->energy -= config.male_idle_cost;
        if (male->energy < 0) male->energy = 0;
        
        /* PATROL BEHAVIOR - occasionally move position slightly */
        if (random_int(0, 10) == 0) {
            int old_col = male->position_col;
            int old_row = male->position_row;
            
            /* Small patrol movement */
            male->position_col += random_int(-1, 1);
            male->position_row += random_int(-1, 1);
            
            /* Keep within reasonable bounds */
            if (male->position_col < 0) 
                male->position_col = 0;
            if (male->position_col > config.maze_cols * 2)
                male->position_col = config.maze_cols * 2;
            
            if (male->position_row < config.maze_rows)
                male->position_row = config.maze_rows;
            if (male->position_row > config.maze_rows + 5)
                male->position_row = config.maze_rows + 5;
                
            if (old_col != male->position_col || old_row != male->position_row) {
                log_event("Male %d patrolling, moved to (%d,%d)", 
                          male->id, male->position_row, male->position_col);
            }
        }
        
        sleep_seconds(2);
    }

    log_event("Male %d stopped", male->id);
    return NULL;
}