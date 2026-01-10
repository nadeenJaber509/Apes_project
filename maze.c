#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include "maze.h"
#include "config.h"

Maze maze;

// Stack for maze generation (recursive backtracking)
typedef struct {
    int row;
    int col;
} StackCell;

static StackCell *stack = NULL;
static int stack_top = -1;
static int stack_capacity = 0;

static void stack_push(int row, int col) {
    if (stack_top + 1 >= stack_capacity) {
        stack_capacity = stack_capacity == 0 ? 256 : stack_capacity * 2;
        stack = realloc(stack, stack_capacity * sizeof(StackCell));
    }
    stack_top++;
    stack[stack_top].row = row;
    stack[stack_top].col = col;
}

static bool stack_pop(int *row, int *col) {
    if (stack_top < 0) return false;
    *row = stack[stack_top].row;
    *col = stack[stack_top].col;
    stack_top--;
    return true;
}

static bool stack_empty(void) {
    return stack_top < 0;
}

// Remove wall between two adjacent cells
static void remove_wall_between(int r1, int c1, int r2, int c2) {
    if (r2 == r1 - 1) {  // r2 is above r1
        maze.cells[r1][c1].walls &= ~WALL_TOP;
        maze.cells[r2][c2].walls &= ~WALL_BOTTOM;
    } else if (r2 == r1 + 1) {  // r2 is below r1
        maze.cells[r1][c1].walls &= ~WALL_BOTTOM;
        maze.cells[r2][c2].walls &= ~WALL_TOP;
    } else if (c2 == c1 - 1) {  // r2 is left of r1
        maze.cells[r1][c1].walls &= ~WALL_LEFT;
        maze.cells[r2][c2].walls &= ~WALL_RIGHT;
    } else if (c2 == c1 + 1) {  // r2 is right of r1
        maze.cells[r1][c1].walls &= ~WALL_RIGHT;
        maze.cells[r2][c2].walls &= ~WALL_LEFT;
    }
}

// Get unvisited neighbors for maze generation
static int get_unvisited_neighbors(int row, int col, bool **visited, int *nr, int *nc) {
    int count = 0;
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    
    for (int i = 0; i < 4; i++) {
        int new_r = row + dr[i];
        int new_c = col + dc[i];
        
        if (new_r >= 0 && new_r < maze.rows && 
            new_c >= 0 && new_c < maze.cols && 
            !visited[new_r][new_c]) {
            nr[count] = new_r;
            nc[count] = new_c;
            count++;
        }
    }
    return count;
}

// Generate maze using recursive backtracking (creates perfect maze with thin walls)
static void generate_maze_recursive_backtracking(void) {
    // Allocate visited array
    bool **visited = malloc(maze.rows * sizeof(bool *));
    for (int i = 0; i < maze.rows; i++) {
        visited[i] = calloc(maze.cols, sizeof(bool));
    }
    
    // Start from a random cell
    int start_row = rand() % maze.rows;
    int start_col = rand() % maze.cols;
    
    visited[start_row][start_col] = true;
    stack_push(start_row, start_col);
    
    while (!stack_empty()) {
        int current_row, current_col;
        stack_pop(&current_row, &current_col);
        
        int nr[4], nc[4];
        int neighbor_count = get_unvisited_neighbors(current_row, current_col, visited, nr, nc);
        
        if (neighbor_count > 0) {
            stack_push(current_row, current_col);
            
            // Pick random unvisited neighbor
            int idx = rand() % neighbor_count;
            int next_row = nr[idx];
            int next_col = nc[idx];
            
            // Remove wall between current and chosen cell
            remove_wall_between(current_row, current_col, next_row, next_col);
            
            visited[next_row][next_col] = true;
            stack_push(next_row, next_col);
        }
    }
    
    // Free visited array
    for (int i = 0; i < maze.rows; i++) {
        free(visited[i]);
    }
    free(visited);
    
    // Free stack
    if (stack) {
        free(stack);
        stack = NULL;
        stack_top = -1;
        stack_capacity = 0;
    }
}

// Remove some additional walls to make the maze less perfect (more paths)
static void add_extra_passages(int percentage) {
    // For very high percentages (low obstacles), remove almost all internal walls
    if (percentage >= 90) {
        printf("High openness (%d%%) - removing most internal walls\n", percentage);
        // Remove all internal walls (keep only border walls)
        for (int i = 0; i < maze.rows; i++) {
            for (int j = 0; j < maze.cols; j++) {
                // Remove right wall if not at right edge
                if (j < maze.cols - 1) {
                    maze.cells[i][j].walls &= ~WALL_RIGHT;
                    maze.cells[i][j+1].walls &= ~WALL_LEFT;
                }
                // Remove bottom wall if not at bottom edge
                if (i < maze.rows - 1) {
                    maze.cells[i][j].walls &= ~WALL_BOTTOM;
                    maze.cells[i+1][j].walls &= ~WALL_TOP;
                }
            }
        }
        return;
    }
    
    // Calculate walls to remove - multiply by 3 to make it more effective
    int walls_to_remove = (maze.rows * maze.cols * percentage * 3) / 100;
    
    printf("Removing up to %d wall segments for openness\n", walls_to_remove);
    
    for (int i = 0; i < walls_to_remove; i++) {
        int row = rand() % maze.rows;
        int col = rand() % maze.cols;
        int dir = rand() % 4;
        
        int dr[] = {-1, 1, 0, 0};
        int dc[] = {0, 0, -1, 1};
        
        int new_r = row + dr[dir];
        int new_c = col + dc[dir];
        
        if (new_r >= 0 && new_r < maze.rows && new_c >= 0 && new_c < maze.cols) {
            remove_wall_between(row, col, new_r, new_c);
        }
    }
}

// Get family exit point (bottom of maze)
static void get_family_exit(int family_id, int *row, int *col) {
    *row = maze.rows - 1;
    *col = (family_id * (maze.cols / config.num_families)) % maze.cols;
    if (*col >= maze.cols) *col = maze.cols - 1;
}

// Ensure exits are accessible by removing walls near family exits
static void ensure_family_exits(void) {
    for (int f = 0; f < config.num_families; f++) {
        int exit_row, exit_col;
        get_family_exit(f, &exit_row, &exit_col);
        
        // Remove bottom wall of exit cell
        maze.cells[exit_row][exit_col].walls &= ~WALL_BOTTOM;
        
        // Clear some walls around the exit to make it accessible
        if (exit_row > 0) {
            remove_wall_between(exit_row, exit_col, exit_row - 1, exit_col);
        }
        if (exit_col > 0) {
            remove_wall_between(exit_row, exit_col, exit_row, exit_col - 1);
        }
        if (exit_col < maze.cols - 1) {
            remove_wall_between(exit_row, exit_col, exit_row, exit_col + 1);
        }
    }
}

void init_maze(void)
{
    maze.rows = config.maze_rows;
    maze.cols = config.maze_cols;
    
    maze.cells = (Cell **)malloc(maze.rows * sizeof(Cell *));
    for (int i = 0; i < maze.rows; i++) {
        maze.cells[i] = (Cell *)malloc(maze.cols * sizeof(Cell));
    }
    
    // Initialize all cells with all walls
    for (int i = 0; i < maze.rows; i++) {
        for (int j = 0; j < maze.cols; j++) {
            maze.cells[i][j].type = CELL_EMPTY;
            maze.cells[i][j].bananas = 0;
            maze.cells[i][j].walls = WALL_TOP | WALL_RIGHT | WALL_BOTTOM | WALL_LEFT;
            pthread_mutex_init(&maze.cells[i][j].mutex, NULL);
        }
    }
    
    pthread_mutex_init(&maze.maze_mutex, NULL);
    
    printf("\n=== Generating Maze with Thin Walls ===\n");
    printf("Size: %dx%d cells\n", maze.rows, maze.cols);
    
    // Generate perfect maze using recursive backtracking
    generate_maze_recursive_backtracking();
    
    // Add extra passages to make it less perfect (easier to navigate)
    // obstacle_percentage controls wall density:
    // - 0% obstacles = 100% extra passages removed = very open/easy maze
    // - 100% obstacles = 0% extra passages removed = perfect maze (hardest)
    int extra_passages = 100 - config.obstacle_percentage;
    printf("Obstacle percentage: %d%%, removing %d%% extra walls\n", 
           config.obstacle_percentage, extra_passages);
    if (extra_passages > 0) {
        add_extra_passages(extra_passages);
    }
    
    // Ensure family exits are accessible
    ensure_family_exits();
    
    // Place bananas on cells
    int banana_count = 0;
    for (int i = 0; i < maze.rows; i++) {
        for (int j = 0; j < maze.cols; j++) {
            if (rand() % 100 < config.banana_cell_percentage) {
                int num_bananas = 1 + (rand() % config.max_bananas_per_cell);
                maze.cells[i][j].bananas = num_bananas;
                maze.cells[i][j].type = CELL_BANANA;
                banana_count += num_bananas;
            }
        }
    }
    
    printf("\n=== Maze Initialized ===\n");
    printf("Size: %dx%d\n", maze.rows, maze.cols);
    printf("Wall openness: %d%% extra passages\n", extra_passages);
    printf("Total Bananas: %d\n", banana_count);
    printf("Family exit points:\n");
    for (int f = 0; f < config.num_families; f++) {
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

// Check if cell is accessible (not an obstacle type)
bool is_cell_accessible(int row, int col)
{
    if (!is_valid_position(row, col))
        return false;
    
    return (maze.cells[row][col].type != CELL_OBSTACLE);
}

// NEW: Check if movement between two adjacent cells is allowed (no wall blocking)
bool can_move(int from_row, int from_col, int to_row, int to_col)
{
    if (!is_valid_position(from_row, from_col) || !is_valid_position(to_row, to_col))
        return false;
    
    Cell *from_cell = &maze.cells[from_row][from_col];
    
    // Check which direction we're moving and if there's a wall
    if (to_row == from_row - 1 && to_col == from_col) {
        // Moving up
        return !(from_cell->walls & WALL_TOP);
    } else if (to_row == from_row + 1 && to_col == from_col) {
        // Moving down
        return !(from_cell->walls & WALL_BOTTOM);
    } else if (to_row == from_row && to_col == from_col - 1) {
        // Moving left
        return !(from_cell->walls & WALL_LEFT);
    } else if (to_row == from_row && to_col == from_col + 1) {
        // Moving right
        return !(from_cell->walls & WALL_RIGHT);
    }
    
    // Not adjacent cells
    return false;
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
    
    cell->bananas += amount;
    if (cell->bananas > 0) {
        cell->type = CELL_BANANA;
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
        
        if (attempts > max_attempts) {
            *row = 0;
            *col = 0;
            return;
        }
    } while (maze.cells[*row][*col].type == CELL_OBSTACLE);
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
    printf("\n=== Current Maze State (Thin Walls) ===\n");
    
    // Top border
    for (int c = 0; c < maze.cols; c++) {
        printf("+---");
    }
    printf("+\n");
    
    for (int r = 0; r < maze.rows; r++) {
        // Cell contents row
        printf("|");
        for (int c = 0; c < maze.cols; c++) {
            Cell *cell = &maze.cells[r][c];
            if (cell->bananas > 0) {
                printf(" %d ", cell->bananas > 9 ? 9 : cell->bananas);
            } else {
                printf("   ");
            }
            if (cell->walls & WALL_RIGHT) {
                printf("|");
            } else {
                printf(" ");
            }
        }
        printf("\n");
        
        // Bottom walls row
        for (int c = 0; c < maze.cols; c++) {
            printf("+");
            if (maze.cells[r][c].walls & WALL_BOTTOM) {
                printf("---");
            } else {
                printf("   ");
            }
        }
        printf("+\n");
    }
    printf("\n");
}

void print_maze_stats(void)
{
    int total_bananas = count_total_bananas();
    int banana_cells = 0;
    int empty_cells = 0;
    
    for (int i = 0; i < maze.rows; i++) {
        for (int j = 0; j < maze.cols; j++) {
            pthread_mutex_lock(&maze.cells[i][j].mutex);
            if (maze.cells[i][j].bananas > 0) {
                banana_cells++;
            } else {
                empty_cells++;
            }
            pthread_mutex_unlock(&maze.cells[i][j].mutex);
        }
    }
    
    printf("\n=== Maze Statistics ===\n");
    printf("Total Cells: %d\n", maze.rows * maze.cols);
    printf("Banana Cells: %d\n", banana_cells);
    printf("Empty Cells: %d\n", empty_cells);
    printf("Total Bananas: %d\n", total_bananas);
    printf("======================\n\n");
}

/* Spatial awareness and pathfinding utilities */

bool is_cell_accessible_safe(int row, int col)
{
    if (row < 0 || row >= maze.rows || col < 0 || col >= maze.cols)
        return false;
    
    pthread_mutex_lock(&maze.cells[row][col].mutex);
    bool accessible = (maze.cells[row][col].type != CELL_OBSTACLE);
    pthread_mutex_unlock(&maze.cells[row][col].mutex);
    
    return accessible;
}

void get_random_empty_position_safe(int *row, int *col)
{
    get_random_empty_position(row, col);
}

int get_manhattan_distance(int r1, int c1, int r2, int c2)
{
    return abs(r1 - r2) + abs(c1 - c2);
}

float get_euclidean_distance(int r1, int c1, int r2, int c2)
{
    int dr = r1 - r2;
    int dc = c1 - c2;
    return sqrt(dr * dr + dc * dc);
}

// Get accessible neighbors (respecting walls!)
int get_accessible_neighbors(int row, int col, int *neighbor_rows, int *neighbor_cols)
{
    int count = 0;
    
    // Check 4 cardinal directions, but respect walls
    if (can_move(row, col, row - 1, col)) {  // Up
        neighbor_rows[count] = row - 1;
        neighbor_cols[count] = col;
        count++;
    }
    if (can_move(row, col, row + 1, col)) {  // Down
        neighbor_rows[count] = row + 1;
        neighbor_cols[count] = col;
        count++;
    }
    if (can_move(row, col, row, col - 1)) {  // Left
        neighbor_rows[count] = row;
        neighbor_cols[count] = col - 1;
        count++;
    }
    if (can_move(row, col, row, col + 1)) {  // Right
        neighbor_rows[count] = row;
        neighbor_cols[count] = col + 1;
        count++;
    }
    
    return count;
}

bool has_line_of_sight(int r1, int c1, int r2, int c2)
{
    int dx = abs(r2 - r1);
    int dy = abs(c2 - c1);
    int sx = (r1 < r2) ? 1 : -1;
    int sy = (c1 < c2) ? 1 : -1;
    int err = dx - dy;
    
    int r = r1, c = c1;
    int prev_r = r, prev_c = c;
    
    while (true) {
        if (r == r2 && c == c2)
            break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            prev_r = r;
            prev_c = c;
            r += sx;
            // Check if we can move through the wall
            if (!can_move(prev_r, prev_c, r, c))
                return false;
        }
        if (e2 < dx) {
            err += dx;
            prev_r = r;
            prev_c = c;
            c += sy;
            if (!can_move(prev_r, prev_c, r, c))
                return false;
        }
    }
    
    return true;
}

int count_accessible_cells_in_region(int center_r, int center_c, int radius)
{
    int count = 0;
    
    for (int r = center_r - radius; r <= center_r + radius; r++) {
        for (int c = center_c - radius; c <= center_c + radius; c++) {
            if (is_valid_position(r, c) && maze.cells[r][c].type != CELL_OBSTACLE) {
                count++;
            }
        }
    }
    
    return count;
}
