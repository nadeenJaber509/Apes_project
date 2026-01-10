#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include "maze.h"
#include "config.h"

Maze maze;

// Allocate a 2D visited array
static bool **alloc_visited(void) {
    bool **visited = (bool **)malloc(maze.rows * sizeof(bool *));
    for (int i = 0; i < maze.rows; i++) {
        visited[i] = (bool *)calloc(maze.cols, sizeof(bool));
    }
    return visited;
}

// Free a 2D visited array
static void free_visited(bool **visited) {
    for (int i = 0; i < maze.rows; i++) {
        free(visited[i]);
    }
    free(visited);
}

// Clear a visited array
static void clear_visited(bool **visited) {
    for (int i = 0; i < maze.rows; i++) {
        for (int j = 0; j < maze.cols; j++) {
            visited[i][j] = false;
        }
    }
}

// Helper function for flood fill - marks all reachable cells from start
static void flood_fill_mark(int start_row, int start_col, bool **visited) {
    if (start_row < 0 || start_row >= maze.rows || 
        start_col < 0 || start_col >= maze.cols)
        return;
    if (visited[start_row][start_col])
        return;
    if (maze.cells[start_row][start_col].type == CELL_OBSTACLE)
        return;
    
    visited[start_row][start_col] = true;
    
    // Check all 4 directions
    flood_fill_mark(start_row - 1, start_col, visited);
    flood_fill_mark(start_row + 1, start_col, visited);
    flood_fill_mark(start_row, start_col - 1, visited);
    flood_fill_mark(start_row, start_col + 1, visited);
}

// Check if a specific cell is reachable from another cell
static bool is_reachable(int from_row, int from_col, int to_row, int to_col, bool **visited) {
    if (maze.cells[from_row][from_col].type == CELL_OBSTACLE)
        return false;
    if (maze.cells[to_row][to_col].type == CELL_OBSTACLE)
        return false;
    
    clear_visited(visited);
    flood_fill_mark(from_row, from_col, visited);
    return visited[to_row][to_col];
}

// Get family exit point (bottom of maze)
static void get_family_exit(int family_id, int *row, int *col) {
    *row = maze.rows - 1;
    *col = (family_id * 3) % maze.cols;
    // Ensure column is within bounds
    if (*col >= maze.cols) *col = maze.cols - 1;
}

// Check if all family exit points are connected to each other
static bool all_families_connected(int num_families, bool **visited) {
    if (num_families <= 0) return true;
    
    // Get first family's exit as reference point
    int ref_row, ref_col;
    get_family_exit(0, &ref_row, &ref_col);
    
    // Make sure reference point is not an obstacle
    if (maze.cells[ref_row][ref_col].type == CELL_OBSTACLE)
        return false;
    
    // Flood fill from reference point
    clear_visited(visited);
    flood_fill_mark(ref_row, ref_col, visited);
    
    // Check that all other family exits are reachable
    for (int f = 1; f < num_families; f++) {
        int exit_row, exit_col;
        get_family_exit(f, &exit_row, &exit_col);
        
        // Exit point itself must not be obstacle
        if (maze.cells[exit_row][exit_col].type == CELL_OBSTACLE)
            return false;
        
        // Must be reachable from family 0's exit
        if (!visited[exit_row][exit_col])
            return false;
    }
    
    // Also check that there are accessible cells in the interior of the maze
    // (so females can actually find bananas)
    int interior_accessible = 0;
    for (int r = 1; r < maze.rows - 1; r++) {
        for (int c = 1; c < maze.cols - 1; c++) {
            if (visited[r][c]) interior_accessible++;
        }
    }
    
    // Require at least 30% of interior cells to be accessible
    int interior_total = (maze.rows - 2) * (maze.cols - 2);
    if (interior_total > 0 && interior_accessible < interior_total * 0.3)
        return false;
    
    return true;
}

// Check if a cell is a family exit point
static bool is_family_exit(int row, int col, int num_families) {
    for (int f = 0; f < num_families; f++) {
        int exit_row, exit_col;
        get_family_exit(f, &exit_row, &exit_col);
        if (row == exit_row && col == exit_col)
            return true;
        // Also protect cells adjacent to exit
        if (row == exit_row && abs(col - exit_col) <= 1)
            return true;
    }
    return false;
}

void init_maze(void)
{
    maze.rows = config.maze_rows;
    maze.cols = config.maze_cols;
    
    maze.cells = (Cell **)malloc(maze.rows * sizeof(Cell *));
    for (int i = 0; i < maze.rows; i++) {
        maze.cells[i] = (Cell *)malloc(maze.cols * sizeof(Cell));
    }
    
    for (int i = 0; i < maze.rows; i++) {
        for (int j = 0; j < maze.cols; j++) {
            maze.cells[i][j].type = CELL_EMPTY;
            maze.cells[i][j].bananas = 0;
            pthread_mutex_init(&maze.cells[i][j].mutex, NULL);
        }
    }
    
    pthread_mutex_init(&maze.maze_mutex, NULL);
    
    int num_families = config.num_families;
    int total_cells = maze.rows * maze.cols;
    int target_obstacles = (total_cells * config.obstacle_percentage) / 100;
    int actual_obstacles = 0;
    int max_attempts = target_obstacles * 20;  // More attempts to find valid placements
    int attempts = 0;
    
    printf("\n=== Generating Maze ===\n");
    printf("Target obstacles: %d (%.1f%% of %d cells)\n", 
           target_obstacles, (float)config.obstacle_percentage, total_cells);
    printf("Ensuring paths for %d families...\n", num_families);
    
    // Pre-allocate visited array for efficiency
    bool **visited = alloc_visited();
    
    // First, ensure all family exit points are clear
    for (int f = 0; f < num_families; f++) {
        int exit_row, exit_col;
        get_family_exit(f, &exit_row, &exit_col);
        maze.cells[exit_row][exit_col].type = CELL_EMPTY;
        // Also clear adjacent cells for easier access
        if (exit_col > 0)
            maze.cells[exit_row][exit_col - 1].type = CELL_EMPTY;
        if (exit_col < maze.cols - 1)
            maze.cells[exit_row][exit_col + 1].type = CELL_EMPTY;
    }
    
    // Place obstacles one by one, ensuring all families stay connected
    while (actual_obstacles < target_obstacles && attempts < max_attempts) {
        int row = rand() % maze.rows;
        int col = rand() % maze.cols;
        
        // Skip if already an obstacle
        if (maze.cells[row][col].type == CELL_OBSTACLE) {
            attempts++;
            continue;
        }
        
        // Never block family exit points
        if (is_family_exit(row, col, num_families)) {
            attempts++;
            continue;
        }
        
        // Temporarily place obstacle
        maze.cells[row][col].type = CELL_OBSTACLE;
        
        // Check if all families are still connected
        if (all_families_connected(num_families, visited)) {
            actual_obstacles++;
        } else {
            // Remove obstacle if it breaks family connectivity
            maze.cells[row][col].type = CELL_EMPTY;
        }
        attempts++;
    }
    
    free_visited(visited);
    
    // Place bananas on empty cells
    for (int i = 0; i < maze.rows; i++) {
        for (int j = 0; j < maze.cols; j++) {
            if (maze.cells[i][j].type == CELL_EMPTY) {
                if (rand() % 100 < config.banana_cell_percentage) {
                    int num_bananas = 1 + (rand() % config.max_bananas_per_cell);
                    maze.cells[i][j].bananas = num_bananas;
                    maze.cells[i][j].type = CELL_BANANA;
                }
            }
        }
    }
    
    printf("\n=== Maze Initialized ===\n");
    printf("Size: %dx%d\n", maze.rows, maze.cols);
    printf("Obstacles: %d (%.1f%% - target was %.1f%%)\n", actual_obstacles, 
           (actual_obstacles * 100.0) / total_cells, (float)config.obstacle_percentage);
    printf("Total Bananas: %d\n", count_total_bananas());
    printf("Family exit points guaranteed accessible:\n");
    for (int f = 0; f < num_families; f++) {
        int exit_row, exit_col;
        get_family_exit(f, &exit_row, &exit_col);
        printf("  Family %d: exit at (%d,%d)\n", f, exit_row, exit_col);
    }
    printf("========================\n\n");
}

void cleanup_maze(void)
{
    for (int i = 0; i < maze.rows; i++) {
        for (int j = 0; j < maze.cols; j++) {
            pthread_mutex_destroy(&maze.cells[i][j].mutex);
        }
    }
    
    pthread_mutex_destroy(&maze.maze_mutex);
    
    for (int i = 0; i < maze.rows; i++) {
        free(maze.cells[i]);
    }
    free(maze.cells);
    
    printf("\nMaze cleanup completed.\n");
}

bool is_valid_position(int row, int col)
{
    return (row >= 0 && row < maze.rows && col >= 0 && col < maze.cols);
}

bool is_cell_accessible(int row, int col)
{
    if (!is_valid_position(row, col))
        return false;
    
    return (maze.cells[row][col].type != CELL_OBSTACLE);
}

int collect_bananas_from_cell(int row, int col, int amount)
{
    if (!is_valid_position(row, col))
        return 0;
    
    Cell *cell = &maze.cells[row][col];
    
    pthread_mutex_lock(&cell->mutex);
    
    int collected = 0;
    if (cell->bananas > 0) {
        collected = (amount < cell->bananas) ? amount : cell->bananas;
        cell->bananas -= collected;
        
        if (cell->bananas == 0) {
            cell->type = CELL_EMPTY;
        }
    }
    
    pthread_mutex_unlock(&cell->mutex);
    
    return collected;
}

int get_cell_bananas(int row, int col)
{
    if (!is_valid_position(row, col))
        return 0;
    
    Cell *cell = &maze.cells[row][col];
    
    pthread_mutex_lock(&cell->mutex);
    int count = cell->bananas;
    pthread_mutex_unlock(&cell->mutex);
    
    return count;
}

void add_bananas_to_cell(int row, int col, int amount)
{
    if (!is_valid_position(row, col) || amount <= 0)
        return;
    
    Cell *cell = &maze.cells[row][col];
    
    pthread_mutex_lock(&cell->mutex);
    
    if (cell->type != CELL_OBSTACLE) {
        cell->bananas += amount;
        if (cell->bananas > 0) {
            cell->type = CELL_BANANA;
        }
    }
    
    pthread_mutex_unlock(&cell->mutex);
}

void get_random_empty_position(int *row, int *col)
{
    int attempts = 0;
    int max_attempts = maze.rows * maze.cols * 2;
    
    do {
        *row = rand() % maze.rows;
        *col = rand() % maze.cols;
        attempts++;
        
        /* Prevent infinite loop if maze is too crowded */
        if (attempts > max_attempts) {
            /* Fallback: search systematically */
            for (int r = 0; r < maze.rows; r++) {
                for (int c = 0; c < maze.cols; c++) {
                    if (is_cell_accessible(r, c)) {
                        *row = r;
                        *col = c;
                        return;
                    }
                }
            }
            /* Last resort: just use (0,0) */
            *row = 0;
            *col = 0;
            return;
        }
    } while (!is_cell_accessible(*row, *col));
}

int count_total_bananas(void)
{
    int total = 0;
    
    for (int i = 0; i < maze.rows; i++) {
        for (int j = 0; j < maze.cols; j++) {
            pthread_mutex_lock(&maze.cells[i][j].mutex);
            total += maze.cells[i][j].bananas;
            pthread_mutex_unlock(&maze.cells[i][j].mutex);
        }
    }
    
    return total;
}

void print_maze(void)
{
    printf("\n=== Current Maze State ===\n");
    printf("Legend: . = Empty, # = Obstacle, 0-9 = Bananas\n\n");
    
    for (int i = 0; i < maze.rows; i++) {
        for (int j = 0; j < maze.cols; j++) {
            pthread_mutex_lock(&maze.cells[i][j].mutex);
            
            if (maze.cells[i][j].type == CELL_OBSTACLE) {
                printf("# ");
            } else if (maze.cells[i][j].bananas > 0) {
                if (maze.cells[i][j].bananas > 9) {
                    printf("+ ");
                } else {
                    printf("%d ", maze.cells[i][j].bananas);
                }
            } else {
                printf(". ");
            }
            
            pthread_mutex_unlock(&maze.cells[i][j].mutex);
        }
        printf("\n");
    }
    printf("\n");
}

void print_maze_stats(void)
{
    int total_bananas = count_total_bananas();
    int banana_cells = 0;
    int obstacle_cells = 0;
    int empty_cells = 0;
    
    for (int i = 0; i < maze.rows; i++) {
        for (int j = 0; j < maze.cols; j++) {
            pthread_mutex_lock(&maze.cells[i][j].mutex);
            
            if (maze.cells[i][j].type == CELL_OBSTACLE) {
                obstacle_cells++;
            } else if (maze.cells[i][j].bananas > 0) {
                banana_cells++;
            } else {
                empty_cells++;
            }
            
            pthread_mutex_unlock(&maze.cells[i][j].mutex);
        }
    }
    
    printf("\n=== Maze Statistics ===\n");
    printf("Total Cells: %d\n", maze.rows * maze.cols);
    printf("Obstacle Cells: %d\n", obstacle_cells);
    printf("Banana Cells: %d\n", banana_cells);
    printf("Empty Cells: %d\n", empty_cells);
    printf("Total Bananas: %d\n", total_bananas);
    printf("======================\n\n");
}

/* ========================================================================
   NEW UTILITY FUNCTIONS FOR SPATIAL AWARENESS AND PATHFINDING
   ======================================================================== */

/**
 * Check if position is valid and accessible (thread-safe version)
 * Same as is_cell_accessible but with explicit mutex locking shown
 */
bool is_cell_accessible_safe(int row, int col)
{
    if (row < 0 || row >= maze.rows || col < 0 || col >= maze.cols)
        return false;
    
    pthread_mutex_lock(&maze.cells[row][col].mutex);
    bool accessible = (maze.cells[row][col].type != CELL_OBSTACLE);
    pthread_mutex_unlock(&maze.cells[row][col].mutex);
    
    return accessible;
}

/**
 * Get random empty position with retry limit to prevent infinite loops
 * Improved version with systematic fallback
 */
void get_random_empty_position_safe(int *row, int *col)
{
    get_random_empty_position(row, col);
}

/**
 * Calculate Manhattan distance between two points (grid-based)
 * Used for pathfinding and proximity checks
 */
int get_manhattan_distance(int r1, int c1, int r2, int c2)
{
    return abs(r1 - r2) + abs(c1 - c2);
}

/**
 * Calculate Euclidean distance between two points (straight-line)
 * Used for realistic distance calculations
 */
float get_euclidean_distance(int r1, int c1, int r2, int c2)
{
    int dr = r1 - r2;
    int dc = c1 - c2;
    return sqrt(dr * dr + dc * dc);
}

/**
 * Find all accessible neighbors of a cell (4-directional)
 * Returns number of neighbors found
 */
int get_accessible_neighbors(int row, int col, int *neighbor_rows, int *neighbor_cols)
{
    int count = 0;
    
    /* Check 4 cardinal directions: up, down, left, right */
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    
    for (int i = 0; i < 4; i++) {
        int new_r = row + dr[i];
        int new_c = col + dc[i];
        
        if (is_cell_accessible(new_r, new_c)) {
            neighbor_rows[count] = new_r;
            neighbor_cols[count] = new_c;
            count++;
        }
    }
    
    return count;
}

/**
 * Check if there's a clear line-of-sight path between two points
 * Uses Bresenham's line algorithm
 */
bool has_line_of_sight(int r1, int c1, int r2, int c2)
{
    /* Use Bresenham's line algorithm to check if path is clear */
    int dx = abs(r2 - r1);
    int dy = abs(c2 - c1);
    int sx = (r1 < r2) ? 1 : -1;
    int sy = (c1 < c2) ? 1 : -1;
    int err = dx - dy;
    
    int r = r1, c = c1;
    
    while (true) {
        if (!is_cell_accessible(r, c))
            return false;
            
        if (r == r2 && c == c2)
            break;
            
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            r += sx;
        }
        if (e2 < dx) {
            err += dx;
            c += sy;
        }
    }
    
    return true;
}

/**
 * Count accessible cells in a region (for checking if maze is too crowded)
 * Useful for validating maze generation and spawn points
 */
int count_accessible_cells_in_region(int center_r, int center_c, int radius)
{
    int count = 0;
    
    for (int r = center_r - radius; r <= center_r + radius; r++) {
        for (int c = center_c - radius; c <= center_c + radius; c++) {
            if (is_cell_accessible(r, c)) {
                count++;
            }
        }
    }
    
    return count;
}