#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "maze.h"
#include "config.h"

Maze maze;

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
    
    int total_cells = maze.rows * maze.cols;
    int obstacle_count = (total_cells * config.obstacle_percentage) / 100;
    
    for (int i = 0; i < obstacle_count; i++) {
        int row, col;
        do {
            row = rand() % maze.rows;
            col = rand() % maze.cols;
        } while (maze.cells[row][col].type == CELL_OBSTACLE);
        
        maze.cells[row][col].type = CELL_OBSTACLE;
    }
    
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
    printf("Obstacles: %d (%.1f%%)\n", obstacle_count, 
           (obstacle_count * 100.0) / total_cells);
    printf("Total Bananas: %d\n", count_total_bananas());
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
    do {
        *row = rand() % maze.rows;
        *col = rand() % maze.cols;
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