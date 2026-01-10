#ifndef MAZE_H
#define MAZE_H

#include <pthread.h>
#include <stdbool.h>

typedef enum {
    CELL_EMPTY,
    CELL_OBSTACLE,
    CELL_BANANA
} CellType;

typedef struct {
    CellType type;
    int bananas;
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