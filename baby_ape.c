#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

#include "family.h"
#include "config.h"
#include "simulation.h"
#include "utils.h"
#include "graphics.h"
#include "maze.h"

/**
 * BFS pathfinding to find next step toward target
 * Returns the direction to move (0-3) or -1 if no path
 */
static int baby_bfs_next_step(int start_r, int start_c, int target_r, int target_c)
{
    /* Validate inputs */
    if (!is_valid_position(start_r, start_c) || !is_valid_position(target_r, target_c))
        return -1;
    
    if (start_r == target_r && start_c == target_c)
        return -1;
    
    int rows = config.maze_rows;
    int cols = config.maze_cols;
    
    if (rows <= 0 || cols <= 0)
        return -1;
    
    bool **visited = malloc(rows * sizeof(bool *));
    int **parent_dir = malloc(rows * sizeof(int *));
    if (!visited || !parent_dir) {
        free(visited);
        free(parent_dir);
        return -1;
    }
    
    for (int i = 0; i < rows; i++) {
        visited[i] = calloc(cols, sizeof(bool));
        parent_dir[i] = malloc(cols * sizeof(int));
        if (!visited[i] || !parent_dir[i]) {
            for (int j = 0; j <= i; j++) {
                free(visited[j]);
                free(parent_dir[j]);
            }
            free(visited);
            free(parent_dir);
            return -1;
        }
        for (int j = 0; j < cols; j++) {
            parent_dir[i][j] = -1;
        }
    }
    
    int *queue_r = malloc(rows * cols * sizeof(int));
    int *queue_c = malloc(rows * cols * sizeof(int));
    if (!queue_r || !queue_c) {
        for (int i = 0; i < rows; i++) {
            free(visited[i]);
            free(parent_dir[i]);
        }
        free(visited);
        free(parent_dir);
        free(queue_r);
        free(queue_c);
        return -1;
    }
    
    int front = 0, back = 0;
    
    queue_r[back] = target_r;
    queue_c[back] = target_c;
    back++;
    visited[target_r][target_c] = true;
    
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    int opposite[] = {1, 0, 3, 2};
    
    bool found = false;
    
    while (front < back && !found) {
        int curr_r = queue_r[front];
        int curr_c = queue_c[front];
        front++;
        
        for (int i = 0; i < 4; i++) {
            int new_r = curr_r + dr[i];
            int new_c = curr_c + dc[i];
            
            if (is_valid_position(new_r, new_c) && !visited[new_r][new_c] &&
                can_move(new_r, new_c, curr_r, curr_c)) {
                
                visited[new_r][new_c] = true;
                parent_dir[new_r][new_c] = opposite[i];
                
                if (new_r == start_r && new_c == start_c) {
                    found = true;
                    break;
                }
                
                queue_r[back] = new_r;
                queue_c[back] = new_c;
                back++;
            }
        }
    }
    
    int result = -1;
    if (found) {
        result = parent_dir[start_r][start_c];
    }
    
    for (int i = 0; i < rows; i++) {
        free(visited[i]);
        free(parent_dir[i]);
    }
    free(visited);
    free(parent_dir);
    free(queue_r);
    free(queue_c);
    
    return result;
}

/**
 * Calculate Euclidean distance between baby and target basket
 * Returns distance in units, or 999.9 if invalid target
 */
static float calculate_distance_to_basket(BabyApe *baby, int target_family)
{
    if (target_family < 0 || target_family >= total_families)
        return 999.9f;
        
    MaleApe *target_male = families[target_family].male;
    if (!target_male)
        return 999.9f;
    
    int dr = baby->position_row - target_male->position_row;
    int dc = baby->position_col - target_male->position_col;
    return sqrt(dr * dr + dc * dc);
}

/**
 * Find nearby fighting males that baby can steal from
 * Only detects fights within max_distance range
 * Returns number of valid targets found
 */
static int find_steal_opportunities(BabyApe *baby, int my_family_id, 
                                   int *targets, int max_targets, float max_distance)
{
    int count = 0;
    
    for (int i = 0; i < total_families && count < max_targets; i++) {
        if (i == my_family_id || families[i].withdrawn)
            continue;

        if (families[i].male && families[i].male->fighting) {
            float dist = calculate_distance_to_basket(baby, i);
            
            /* Only steal from nearby baskets (baby has limited range) */
            if (dist <= max_distance && families[i].basket_bananas > 0) {
                targets[count++] = i;
            }
        }
    }
    
    return count;
}

/**
 * Move baby toward target position using BFS pathfinding
 * Baby moves gradually, respecting walls
 */
static void move_baby_toward(BabyApe *baby, int target_row, int target_col, float speed)
{
    (void)speed; /* Speed is 1 step at a time with BFS */
    
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    
    /* Use BFS to find next step */
    int direction = baby_bfs_next_step(baby->position_row, baby->position_col, 
                                        target_row, target_col);
    
    if (direction >= 0 && direction < 4) {
        int new_r = baby->position_row + dr[direction];
        int new_c = baby->position_col + dc[direction];
        
        if (can_move(baby->position_row, baby->position_col, new_r, new_c)) {
            baby->position_row = new_r;
            baby->position_col = new_c;
            return;
        }
    }
    
    /* BFS failed, try any available direction */
    for (int i = 0; i < 4; i++) {
        int new_r = baby->position_row + dr[i];
        int new_c = baby->position_col + dc[i];
        if (can_move(baby->position_row, baby->position_col, new_r, new_c)) {
            baby->position_row = new_r;
            baby->position_col = new_c;
            return;
        }
    }
}

/**
 * Baby Ape Thread - Main behavior loop
 * 
 * Behavior:
 * - Detects fights within limited range (8 units)
 * - Physically moves to target basket
 * - Steals only when close enough (≤3 units)
 * - Runs back to dad after stealing
 * - Wanders near dad when no opportunities
 * 
 * Detection range: 8.0 units
 * Steal range: 3.0 units
 * Movement speed: 1 unit per 200ms
 */
void* baby_ape_thread(void *arg)
{
    BabyApe *baby = (BabyApe *)arg;
    int family_id = baby->family_id;
    
    /* Baby starts near dad's basket */
    baby->position_row = families[family_id].male->position_row;
    baby->position_col = families[family_id].male->position_col + 1;

    log_event("Baby %d (Family %d) started at position (%d,%d)", 
              baby->id, family_id, baby->position_row, baby->position_col);

    const float STEAL_RANGE = 8.0f;   /* Baby can detect fights within 8 units */
    const float APPROACH_DIST = 3.0f; /* Must be within 3 units to steal */
    const float BABY_SPEED = 1.0f;    /* Baby moves 1 unit per step */

    while (is_simulation_running() && baby->active) {

        if (families[family_id].withdrawn)
            break;

        /* Check if baby has eaten too much */
        if (baby->bananas_eaten >= config.max_baby_eaten) {
            log_event("Baby %d is full (eaten %d bananas)", 
                      baby->id, baby->bananas_eaten);
            break;
        }

        /* SPATIAL AWARENESS: Look for nearby opportunities based on proximity */
        int targets[10];
        int target_count = find_steal_opportunities(baby, family_id, 
                                                    targets, 10, STEAL_RANGE);
        
        if (target_count > 0) {
            /* Select closest target for efficiency */
            int best_target = targets[0];
            float best_dist = calculate_distance_to_basket(baby, targets[0]);
            
            for (int i = 1; i < target_count; i++) {
                float dist = calculate_distance_to_basket(baby, targets[i]);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_target = targets[i];
                }
            }
            
            /* PHYSICAL MOVEMENT: Move toward target basket */
            MaleApe *target_male = families[best_target].male;
            log_event("Baby %d spotted fight at Family %d (distance: %.1f), moving to steal",
                      baby->id, best_target, best_dist);
            
            /* Move toward the basket during the fight - takes time! */
            int move_attempts = 0;
            while (best_dist > APPROACH_DIST && 
                   families[best_target].male->fighting &&
                   is_simulation_running() &&
                   !families[family_id].withdrawn &&
                   move_attempts < 30) {
                
                move_baby_toward(baby, target_male->position_row, 
                               target_male->position_col, BABY_SPEED);
                
                best_dist = calculate_distance_to_basket(baby, best_target);
                sleep_milliseconds(200); /* Movement takes time */
                move_attempts++;
            }
            
            /* Try to steal if close enough and fight still ongoing */
            if (best_dist <= APPROACH_DIST && 
                families[best_target].male->fighting &&
                !families[best_target].withdrawn) {
                
                int steal_amount = config.baby_eat_rate;
                int stolen = steal_from_basket(best_target, steal_amount);
                
                if (stolen > 0) {
                    graphics_add_baby_steal();
                    log_event("Baby %d stole %d bananas from Family %d basket!",
                              baby->id, stolen, best_target);
                    
                    /* Decide: eat or give to dad? (50/50 chance) */
                    if (random_int(0, 1) == 0) {
                        /* Eat it! */
                        baby->bananas_eaten += stolen;
                        log_event("Baby %d ate %d bananas (total eaten: %d)",
                                  baby->id, stolen, baby->bananas_eaten);
                    } else {
                        /* Give to dad */
                        add_to_basket(family_id, stolen);
                        log_event("Baby %d gave %d bananas to dad's basket",
                                  baby->id, stolen);
                    }
                    
                    /* ESCAPE BEHAVIOR: Run away after stealing! */
                    log_event("Baby %d running back to safety", baby->id);
                    int escape_steps = 5;
                    while (escape_steps > 0 && is_simulation_running()) {
                        move_baby_toward(baby, 
                                       families[family_id].male->position_row,
                                       families[family_id].male->position_col,
                                       BABY_SPEED);
                        escape_steps--;
                        sleep_milliseconds(150); /* Running is slightly faster */
                    }
                }
            } else if (move_attempts >= 30) {
                log_event("Baby %d couldn't reach target in time", baby->id);
            }
        } else {
            /* No opportunities nearby - STAY NEAR DAD */
            MaleApe *dad = families[family_id].male;
            float dist_from_dad = calculate_distance_to_basket(baby, family_id);
            
            if (dist_from_dad > 5.0f) {
                /* Too far from dad, go back for safety */
                move_baby_toward(baby, dad->position_row, dad->position_col, BABY_SPEED);
                sleep_milliseconds(200);
            } else {
                /* Safe distance - wander nearby casually */
                if (random_int(0, 3) == 0) {
                    baby->position_row += random_int(-1, 1);
                    baby->position_col += random_int(-1, 1);
                    
                    /* Keep within bounds */
                    if (baby->position_row < config.maze_rows)
                        baby->position_row = config.maze_rows;
                    if (baby->position_col < 0)
                        baby->position_col = 0;
                }
            }
        }

        /* Babies act less frequently than adults */
        sleep_milliseconds(500);
    }

    log_event("Baby %d stopped (eaten=%d)", baby->id, baby->bananas_eaten);
    return NULL;
}