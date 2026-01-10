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
#include "graphics.h"

/**
 * Calculate Manhattan distance between two positions
 * Used for pathfinding and proximity detection
 */
static int manhattan_distance(int r1, int c1, int r2, int c2)
{
    return abs(r1 - r2) + abs(c1 - c2);
}

/**
 * Find nearest banana using BFS (actual walking distance, not Manhattan)
 * This prevents looping by finding the truly closest reachable banana
 * Returns true if banana found, updates target_r and target_c
 */
static bool find_nearest_banana_bfs(int from_r, int from_c, int *target_r, int *target_c)
{
    int rows = config.maze_rows;
    int cols = config.maze_cols;
    
    if (rows <= 0 || cols <= 0)
        return false;
    
    /* Check current cell first */
    if (get_cell_bananas(from_r, from_c) > 0) {
        *target_r = from_r;
        *target_c = from_c;
        return true;
    }
    
    /* Allocate visited array */
    bool **visited = malloc(rows * sizeof(bool *));
    if (!visited) return false;
    
    for (int i = 0; i < rows; i++) {
        visited[i] = calloc(cols, sizeof(bool));
        if (!visited[i]) {
            for (int j = 0; j < i; j++) free(visited[j]);
            free(visited);
            return false;
        }
    }
    
    /* BFS queue */
    int *queue_r = malloc(rows * cols * sizeof(int));
    int *queue_c = malloc(rows * cols * sizeof(int));
    if (!queue_r || !queue_c) {
        for (int i = 0; i < rows; i++) free(visited[i]);
        free(visited);
        free(queue_r);
        free(queue_c);
        return false;
    }
    
    int front = 0, back = 0;
    queue_r[back] = from_r;
    queue_c[back] = from_c;
    back++;
    visited[from_r][from_c] = true;
    
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    
    bool found = false;
    
    /* BFS finds nearest by actual walking distance */
    while (front < back && !found) {
        int curr_r = queue_r[front];
        int curr_c = queue_c[front];
        front++;
        
        for (int i = 0; i < 4; i++) {
            int new_r = curr_r + dr[i];
            int new_c = curr_c + dc[i];
            
            if (is_valid_position(new_r, new_c) && !visited[new_r][new_c] &&
                can_move(curr_r, curr_c, new_r, new_c)) {
                
                visited[new_r][new_c] = true;
                
                /* Found a banana! This is the nearest one by walking distance */
                if (get_cell_bananas(new_r, new_c) > 0) {
                    *target_r = new_r;
                    *target_c = new_c;
                    found = true;
                    break;
                }
                
                queue_r[back] = new_r;
                queue_c[back] = new_c;
                back++;
            }
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < rows; i++) free(visited[i]);
    free(visited);
    free(queue_r);
    free(queue_c);
    
    return found;
}

/**
 * BFS pathfinding to find next step toward target
 * Returns the direction to move (0-3) or -1 if no path
 * This properly navigates around walls in a maze
 */
static int bfs_find_next_step(int start_r, int start_c, int target_r, int target_c)
{
    /* Validate inputs */
    if (!is_valid_position(start_r, start_c) || !is_valid_position(target_r, target_c))
        return -1;
    
    if (start_r == target_r && start_c == target_c)
        return -1; /* Already at target */
    
    int rows = config.maze_rows;
    int cols = config.maze_cols;
    
    if (rows <= 0 || cols <= 0)
        return -1;
    
    /* Allocate visited and parent arrays */
    bool **visited = malloc(rows * sizeof(bool *));
    int **parent_dir = malloc(rows * sizeof(int *)); /* Direction we came from */
    if (!visited || !parent_dir) {
        free(visited);
        free(parent_dir);
        return -1;
    }
    
    for (int i = 0; i < rows; i++) {
        visited[i] = calloc(cols, sizeof(bool));
        parent_dir[i] = malloc(cols * sizeof(int));
        if (!visited[i] || !parent_dir[i]) {
            /* Cleanup on allocation failure */
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
    
    /* BFS queue */
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
    
    /* Start BFS from target (backwards) to find path */
    queue_r[back] = target_r;
    queue_c[back] = target_c;
    back++;
    visited[target_r][target_c] = true;
    
    int dr[] = {-1, 1, 0, 0};  /* up, down, left, right */
    int dc[] = {0, 0, -1, 1};
    int opposite[] = {1, 0, 3, 2}; /* opposite directions */
    
    bool found = false;
    
    while (front < back && !found) {
        int curr_r = queue_r[front];
        int curr_c = queue_c[front];
        front++;
        
        for (int i = 0; i < 4; i++) {
            int new_r = curr_r + dr[i];
            int new_c = curr_c + dc[i];
            
            /* Check if we can move FROM new cell TO current cell (backwards BFS) */
            if (is_valid_position(new_r, new_c) && !visited[new_r][new_c] &&
                can_move(new_r, new_c, curr_r, curr_c)) {
                
                visited[new_r][new_c] = true;
                parent_dir[new_r][new_c] = opposite[i]; /* Direction to go toward target */
                
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
    
    /* Cleanup */
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
 * Move one step toward target using BFS pathfinding
 * Returns true if moved, false if stuck (no path exists)
 * Properly navigates around walls in a maze
 */
static bool move_toward_target(int *current_r, int *current_c, int target_r, int target_c)
{
    int dr[] = {-1, 1, 0, 0};  /* up, down, left, right */
    int dc[] = {0, 0, -1, 1};
    
    /* Use BFS to find the best direction */
    int direction = bfs_find_next_step(*current_r, *current_c, target_r, target_c);
    
    if (direction >= 0 && direction < 4) {
        int new_r = *current_r + dr[direction];
        int new_c = *current_c + dc[direction];
        
        if (can_move(*current_r, *current_c, new_r, new_c)) {
            *current_r = new_r;
            *current_c = new_c;
            return true;
        }
    }
    
    /* BFS failed, try any available move */
    for (int i = 0; i < 4; i++) {
        int new_r = *current_r + dr[i];
        int new_c = *current_c + dc[i];
        if (can_move(*current_r, *current_c, new_r, new_c)) {
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
 * Female Ape Thread - Main behavior loop (GREEDY ALGORITHM)
 * 
 * Behavior:
 * - Uses GREEDY algorithm: always goes to nearest banana in entire maze
 * - Collects bananas until reaching target
 * - Immediately delivers to basket
 * - Repeats until tired
 * 
 * Movement: Greedy best-first search toward nearest banana globally
 * Combat range: 2 cells
 */
void* female_ape_thread(void *arg)
{
    FemaleApe *female = (FemaleApe *)arg;
    int family_id = female->family_id;
    
    /* Track if this is the first trip (start random) or subsequent (start from exit) */
    bool first_trip = true;
    int last_exit_row = -1;
    int last_exit_col = -1;

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

        /* Check if there are any bananas left in the maze */
        int dummy_r, dummy_c;
        MaleApe *husband = families[family_id].male;
        
        /* Use female's current position or a default if no valid position */
        int check_row = (last_exit_row >= 0) ? last_exit_row : 0;
        int check_col = (last_exit_col >= 0) ? last_exit_col : 0;
        
        bool bananas_exist = find_nearest_banana_bfs(check_row, check_col, &dummy_r, &dummy_c);
        
        if (!bananas_exist && husband != NULL) {
            /* No bananas in maze - go to husband and idle */
            female->in_maze = false;
            female->position_row = husband->position_row;
            female->position_col = husband->position_col;
            
            if (!female->resting) {
                log_event("Female %d: No bananas left, idling with husband at (%d,%d)",
                          female->id, female->position_row, female->position_col);
                female->resting = true;  /* Mark as idle/resting */
            }
            
            sleep_seconds(2);  /* Idle for a bit, then check again */
            continue;
        }
        
        /* Bananas available - stop idling if we were */
        if (female->resting) {
            female->resting = false;
        }

        /* Enter maze - first trip is random, subsequent trips start from last exit */
        female->in_maze = true;
        female->bananas_collected = 0;
        
        if (first_trip || last_exit_row < 0) {
            get_random_empty_position(&female->position_row, &female->position_col);
            log_event("Female %d entered maze at random (%d,%d)", 
                      female->id, female->position_row, female->position_col);
            first_trip = false;
        } else {
            female->position_row = last_exit_row;
            female->position_col = last_exit_col;
            log_event("Female %d re-entered maze from exit (%d,%d)", 
                      female->id, female->position_row, female->position_col);
        }

        int collected = 0;
        int moves_without_progress = 0;

        /* GREEDY BANANA COLLECTION: Find nearest banana, collect on the way up to max_capacity */
        while (collected < config.female_max_capacity &&
               moves_without_progress < 50 &&
               is_simulation_running() &&
               !families[family_id].withdrawn) {

            /* First, check if current cell has bananas - always collect if there's room */
            int current_bananas = get_cell_bananas(female->position_row, female->position_col);
            if (current_bananas > 0 && collected < config.female_max_capacity) {
                int can_take = config.female_max_capacity - collected;
                int got = collect_bananas_from_cell(
                    female->position_row, female->position_col, can_take);

                if (got > 0) {
                    collected += got;
                    female->bananas_collected = collected;
                    female->energy -= config.female_collect_cost;
                    if (female->energy < 0) female->energy = 0;
                    moves_without_progress = 0;
                    
                    log_event("Female %d collected %d bananas at (%d,%d), total=%d",
                              female->id, got, female->position_row, 
                              female->position_col, collected);
                    
                    /* If we reached target, start exiting */
                    if (collected >= config.female_target_bananas) {
                        break;
                    }
                }
            }

            /* If at max capacity, stop collecting and exit */
            if (collected >= config.female_max_capacity) {
                break;
            }

            /* BFS: Find the nearest banana by actual walking distance */
            int target_r, target_c;
            if (find_nearest_banana_bfs(female->position_row, female->position_col, 
                                        &target_r, &target_c)) {
                
                /* Move toward the nearest banana */
                if (move_toward_target(&female->position_row, &female->position_col,
                                      target_r, target_c)) {
                    moves_without_progress = 0;
                } else {
                    moves_without_progress++;
                }
            } else {
                /* No bananas left in maze, stop collecting */
                log_event("Female %d found no more bananas in maze", female->id);
                break;
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
                            if (female->energy < 0) female->energy = 0;
                            nearby->energy -= config.female_fight_lose_cost;
                            if (nearby->energy < 0) nearby->energy = 0;
                            graphics_add_female_fight();
                            log_event("Female %d won fight! (kept %d bananas)", 
                                      female->id, collected);
                        } else {
                            /* Lose: lose some bananas */
                            int lost = collected / 2;
                            collected -= lost;
                            female->bananas_collected = collected;
                            female->energy -= config.female_fight_lose_cost;
                            if (female->energy < 0) female->energy = 0;
                            nearby->energy -= config.female_fight_win_cost;
                            if (nearby->energy < 0) nearby->energy = 0;
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

        /* Save current position as exit point for next trip */
        last_exit_row = female->position_row;
        last_exit_col = female->position_col;
        
        female->in_maze = false;

        /* Deliver bananas to basket */
        if (collected > 0 && !families[family_id].withdrawn) {
            add_to_basket(family_id, collected);
            graphics_add_collected(collected);
            log_event("Female %d delivered %d bananas to basket", 
                      female->id, collected);
        } else if (collected == 0 && female->bananas_collected > 0) {
            log_event("Female %d returned empty-handed (lost all in fights)", female->id);
        }

        female->bananas_collected = 0;
        female->energy -= config.female_trip_end_cost;
        if (female->energy < 0) female->energy = 0;

        sleep_seconds(1);
    }

    log_event("Female %d stopped", female->id);
    return NULL;
}