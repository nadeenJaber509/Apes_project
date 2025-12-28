// graphics.c
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <pthread.h>
#include "graphics.h"
#include "simulation.h"
#include "maze.h"
#include "config.h"
#include "family.h"

/* رسم المتاهة (مبدئي وبسيط) */
void draw_scene(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw maze
    for (int r = 0; r < maze.rows; r++) {
        for (int c = 0; c < maze.cols; c++) {
            Cell cell = maze.cells[r][c];
            if (cell.type == CELL_OBSTACLE) {
                glColor3f(0.0, 0.0, 0.0); // black
            } else if (cell.type == CELL_BANANA) {
                glColor3f(1.0, 1.0, 0.0); // yellow
            } else {
                glColor3f(1.0, 1.0, 1.0); // white
            }
            glRectf(c, r, c + 1, r + 1);
        }
    }

    // Draw female apes
    glColor3f(1.0, 0.75, 0.8); // pink
    for (int i = 0; i < total_families; i++) {
        FemaleApe *female = families[i].female;
        if (female->in_maze && female->active) {
            glRectf(female->position_col, female->position_row,
                    female->position_col + 1, female->position_row + 1);
            // Draw number
            glColor3f(0.0, 0.0, 0.0);
            glRasterPos2f(female->position_col + 0.1, female->position_row + 0.5);
            char buf[10];
            sprintf(buf, "%d", female->bananas_collected);
            for (char *c = buf; *c; c++) {
                glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
            }
            glColor3f(1.0, 0.75, 0.8);
        }
    }

    // Draw male apes
    glColor3f(0.0, 0.0, 1.0); // blue
    for (int i = 0; i < total_families; i++) {
        MaleApe *male = families[i].male;
        if (male->active) {
            glRectf(male->position_col, male->position_row,
                    male->position_col + 1, male->position_row + 1);
            // Draw energy
            glColor3f(1.0, 1.0, 1.0);
            glRasterPos2f(male->position_col + 0.1, male->position_row + 0.5);
            char buf[10];
            sprintf(buf, "%d", male->energy);
            for (char *c = buf; *c; c++) {
                glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
            }
            glColor3f(0.0, 0.0, 1.0);
        }
    }

    // Draw baby apes
    glColor3f(0.6, 0.3, 0.0); // brown
    for (int i = 0; i < total_families; i++) {
        for (int j = 0; j < families[i].num_babies; j++) {
            BabyApe *baby = &families[i].babies[j];
            if (baby->active) {
                glRectf(baby->position_col, baby->position_row,
                        baby->position_col + 1, baby->position_row + 1);
                // Draw eaten
                glColor3f(1.0, 1.0, 1.0);
                glRasterPos2f(baby->position_col + 0.1, baby->position_row + 0.5);
                char buf[10];
                sprintf(buf, "%d", baby->bananas_eaten);
                for (char *c = buf; *c; c++) {
                    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
                }
                glColor3f(0.6, 0.3, 0.0);
            }
        }
    }

    glFlush();
}

void idle_func(void)
{
    glutPostRedisplay();
    if (simulation_done) {
        glutLeaveMainLoop();
    }
}

/* حلقة OpenGL */
void setup_graphics(void)
{
    glutDisplayFunc(draw_scene);
    glutIdleFunc(idle_func);
}

/* تهيئة الجرافيكس */
void init_graphics(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(600, 600);
    glutCreateWindow("Apes Simulation");

    glClearColor(1.0, 1.0, 1.0, 1.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, config.maze_cols, 0, config.maze_rows + 2);

    setup_graphics();
}

/* إيقاف الجرافيكس */
void stop_graphics(void)
{
    // GLUT ما بدعم إيقاف نظيف، فخليها فاضية
}
