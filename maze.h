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

void init_maze(void);
void cleanup_maze(void);
void print_maze(void);
void print_maze_stats(void);

bool is_valid_position(int row, int col);
bool is_cell_accessible(int row, int col);
int collect_bananas_from_cell(int row, int col, int amount);
int get_cell_bananas(int row, int col);
void add_bananas_to_cell(int row, int col, int amount);

void get_random_empty_position(int *row, int *col);
int count_total_bananas(void);

#endif