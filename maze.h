#ifndef MAZE_H
#define MAZE_H

#include <pthread.h>
#include <stdbool.h>

typedef enum {
    CELL_EMPTY,
    CELL_OBSTACLE,  // Keep for compatibility but won't be used much
    CELL_BANANA
} CellType;

// Wall flags for each cell (thin walls between cells)
#define WALL_TOP    0x01
#define WALL_RIGHT  0x02
#define WALL_BOTTOM 0x04
#define WALL_LEFT   0x08

typedef struct {
    CellType type;
    int bananas;
    unsigned char walls;  // Bit flags for walls (WALL_TOP, WALL_RIGHT, etc.)
    pthread_mutex_t mutex;
} Cell;

typedef struct {
    int rows;
    int cols;
    Cell **cells;
    pthread_mutex_t maze_mutex;
} Maze;

extern Maze maze;

/* Initialization and cleanup */
void init_maze(void);
void cleanup_maze(void);
void print_maze(void);
void print_maze_stats(void);

/* Basic cell operations */
bool is_valid_position(int row, int col);
bool is_cell_accessible(int row, int col);
bool can_move(int from_row, int from_col, int to_row, int to_col);  // NEW: Check if movement is allowed (no wall blocking)
int collect_bananas_from_cell(int row, int col, int amount);
int get_cell_bananas(int row, int col);
void add_bananas_to_cell(int row, int col, int amount);

/* Position and movement utilities */
void get_random_empty_position(int *row, int *col);
void get_random_empty_position_safe(int *row, int *col);
int count_total_bananas(void);

/* NEW: Spatial awareness and pathfinding utilities */
bool is_cell_accessible_safe(int row, int col);
int get_manhattan_distance(int r1, int c1, int r2, int c2);
float get_euclidean_distance(int r1, int c1, int r2, int c2);
int get_accessible_neighbors(int row, int col, int *neighbor_rows, int *neighbor_cols);
bool has_line_of_sight(int r1, int c1, int r2, int c2);
int count_accessible_cells_in_region(int center_r, int center_c, int radius);

#endif