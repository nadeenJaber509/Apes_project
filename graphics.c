// graphics.c
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include "graphics.h"
#include "simulation.h"
#include "maze.h"
#include "config.h"
#include "family.h"

static int frame_count = 0;
static int animation_frame = 0;

// Fight effect structure
#define MAX_FIGHT_EFFECTS 20
typedef struct {
    float x, y;
    int frames_remaining;
    bool active;
} FightEffect;

static FightEffect fight_effects[MAX_FIGHT_EFFECTS];
static pthread_mutex_t effects_mutex = PTHREAD_MUTEX_INITIALIZER;

// Helper function to draw a circle (for apes)
void draw_circle(float cx, float cy, float radius, int segments)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159f * i / segments;
        float x = cx + radius * cos(angle);
        float y = cy + radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

// Add a new fight effect at position
void add_fight_effect(float x, float y)
{
    pthread_mutex_lock(&effects_mutex);
    for (int i = 0; i < MAX_FIGHT_EFFECTS; i++) {
        if (!fight_effects[i].active) {
            fight_effects[i].x = x;
            fight_effects[i].y = y;
            fight_effects[i].frames_remaining = 60; // ~2 seconds at 30fps
            fight_effects[i].active = true;
            break;
        }
    }
    pthread_mutex_unlock(&effects_mutex);
}

// Draw explosion/collision effect
void draw_explosion(float cx, float cy, float progress)
{
    float size = 0.3f + progress * 0.7f;
    float alpha = 1.0f - progress;

    // Draw expanding rings
    for (int ring = 0; ring < 3; ring++) {
        float ring_size = size * (1.0f + ring * 0.3f);
        int segments = 16;

        if (ring == 0) glColor3f(1.0f * alpha, 0.2f * alpha, 0.0f);
        else if (ring == 1) glColor3f(1.0f * alpha, 0.5f * alpha, 0.0f);
        else glColor3f(1.0f * alpha, 0.8f * alpha, 0.0f);

        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * 3.14159f * i / segments;
            float x = cx + ring_size * cos(angle);
            float y = cy + ring_size * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
    }

    // Draw spikes/rays
    int num_spikes = 8;
    for (int i = 0; i < num_spikes; i++) {
        float angle = 2.0f * 3.14159f * i / num_spikes + progress * 2.0f;
        float inner = 0.1f;
        float outer = size * 1.2f;

        glColor3f(1.0f * alpha, 0.3f * alpha, 0.0f);
        glBegin(GL_LINES);
        glVertex2f(cx + inner * cos(angle), cy + inner * sin(angle));
        glVertex2f(cx + outer * cos(angle), cy + outer * sin(angle));
        glEnd();
    }

    // Draw center flash
    glColor3f(1.0f * alpha, 1.0f * alpha, 0.5f * alpha);
    draw_circle(cx, cy, 0.15f * (1.0f - progress * 0.5f), 12);

    // Draw "POW"
    if (progress < 0.5f) {
        glColor3f(1.0f, 0.0f, 0.0f);
        glRasterPos2f(cx - 0.25f, cy + size + 0.1f);
        const char *text = "POW!";
        for (const char *c = text; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
    }

    // Draw particles
    int num_particles = 6;
    for (int i = 0; i < num_particles; i++) {
        float angle = 2.0f * 3.14159f * i / num_particles + animation_frame * 0.1f;
        float dist = size * 0.8f * progress;
        float px = cx + dist * cos(angle);
        float py = cy + dist * sin(angle);

        glColor3f(1.0f * alpha, 0.6f * alpha, 0.0f);
        draw_circle(px, py, 0.08f * (1.0f - progress), 8);
    }
}

// Helper function to draw a rounded rectangle effect with border
void draw_cell_with_border(float x, float y, float r, float g, float b,
                           float border_r, float border_g, float border_b)
{
    glColor3f(border_r, border_g, border_b);
    glRectf(x, y, x + 1, y + 1);

    glColor3f(r, g, b);
    glRectf(x + 0.06f, y + 0.06f, x + 0.94f, y + 0.94f);

    glColor3f(r + 0.15f, g + 0.15f, b + 0.15f);
    glRectf(x + 0.06f, y + 0.55f, x + 0.94f, y + 0.94f);
}

// Helper function to draw text on two lines (centered)
void draw_two_line_text(float x, float y, const char *line1, const char *line2,
                        float r, float g, float b)
{
    glColor3f(r, g, b);

    int len1 = 0;
    for (const char *c = line1; *c; c++) len1++;
    float offset1 = len1 * 0.035f;
    glRasterPos2f(x + 0.5f - offset1, y + 0.65f);
    for (const char *c = line1; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
    }

    int len2 = 0;
    for (const char *c = line2; *c; c++) len2++;
    float offset2 = len2 * 0.045f;
    glRasterPos2f(x + 0.5f - offset2, y + 0.25f);
    for (const char *c = line2; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

// Helper function to draw single line text centered
void draw_text_centered(float x, float y, const char *text, float r, float g, float b)
{
    glColor3f(r, g, b);
    int len = 0;
    for (const char *c = text; *c; c++) len++;
    float offset = len * 0.04f;
    glRasterPos2f(x + 0.5f - offset, y + 0.4f);
    for (const char *c = text; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

/* رسم المتاهة */
void draw_scene(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (maze.cells == NULL || families == NULL) {
        glFlush();
        return;
    }

    // Draw maze
    for (int r = 0; r < maze.rows; r++) {
        for (int c = 0; c < maze.cols; c++) {
            Cell cell = maze.cells[r][c];
            if (cell.type == CELL_OBSTACLE) {
                draw_cell_with_border(c, r, 0.25f, 0.25f, 0.3f, 0.1f, 0.1f, 0.15f);
            } else if (cell.type == CELL_BANANA) {
                draw_cell_with_border(c, r, 1.0f, 0.85f, 0.0f, 0.8f, 0.5f, 0.0f);

                if (cell.bananas > 0) {
                    glColor3f(1.0f, 0.95f, 0.0f);
                    draw_circle(c + 0.5f, r + 0.65f, 0.15f, 16);

                    char buf[10];
                    sprintf(buf, "%d", cell.bananas);
                    glColor3f(0.4f, 0.2f, 0.0f);
                    int len = 0;
                    for (const char *ch = buf; *ch; ch++) len++;
                    glRasterPos2f(c + 0.5f - len * 0.05f, r + 0.25f);
                    for (const char *ch = buf; *ch; ch++) {
                        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *ch);
                    }
                }
            } else {
                draw_cell_with_border(c, r, 0.92f, 0.94f, 0.96f, 0.75f, 0.78f, 0.82f);
            }
        }
    }

    // Females
    for (int i = 0; i < total_families; i++) {
        FemaleApe *female = families[i].female;
        if (female->in_maze && female->active) {
            float x = female->position_col;
            float y = female->position_row;

            if (female->fighting) {
                float shake = sinf(animation_frame * 0.5f) * 0.05f;
                x += shake;

                glColor3f(0.9f, 0.0f, 0.4f);
                draw_circle(x + 0.5f, y + 0.5f, 0.6f, 16);

                glColor3f(0.5f, 0.0f, 0.2f);
                draw_circle(x + 0.53f, y + 0.47f, 0.42f, 24);

                glColor3f(1.0f, 0.0f, 0.3f);
                draw_circle(x + 0.5f, y + 0.5f, 0.42f, 24);

                glColor3f(1.0f, 0.4f, 0.5f);
                draw_circle(x + 0.4f, y + 0.6f, 0.15f, 16);
            } else {
                glColor3f(0.4f, 0.1f, 0.25f);
                draw_circle(x + 0.53f, y + 0.47f, 0.42f, 24);

                glColor3f(1.0f, 0.3f, 0.6f);
                draw_circle(x + 0.5f, y + 0.5f, 0.42f, 24);

                glColor3f(1.0f, 0.6f, 0.8f);
                draw_circle(x + 0.4f, y + 0.6f, 0.15f, 16);
            }

            char line1[8], line2[8];
            sprintf(line1, "F%d", female->family_id);
            sprintf(line2, "%d", female->bananas_collected);
            draw_two_line_text(x, y, line1, line2, 1.0f, 1.0f, 1.0f);
        }
    }

    // Males
    for (int i = 0; i < total_families; i++) {
        MaleApe *male = families[i].male;
        if (male->active) {
            float x = male->position_col;
            float y = male->position_row;

            if (male->fighting) {
                float shake = sinf(animation_frame * 0.5f) * 0.05f;
                x += shake;

                glColor3f(0.8f, 0.0f, 0.0f);
                draw_circle(x + 0.5f, y + 0.5f, 0.6f, 16);

                glColor3f(0.4f, 0.0f, 0.0f);
                glRectf(x + 0.12f, y + 0.08f, x + 0.92f, y + 0.88f);

                glColor3f(0.9f, 0.1f, 0.1f);
                glRectf(x + 0.1f, y + 0.1f, x + 0.9f, y + 0.9f);

                glColor3f(1.0f, 0.3f, 0.3f);
                glRectf(x + 0.1f, y + 0.5f, x + 0.9f, y + 0.9f);
            } else {
                glColor3f(0.0f, 0.15f, 0.4f);
                glRectf(x + 0.12f, y + 0.08f, x + 0.92f, y + 0.88f);

                glColor3f(0.1f, 0.4f, 0.9f);
                glRectf(x + 0.1f, y + 0.1f, x + 0.9f, y + 0.9f);

                glColor3f(0.3f, 0.6f, 1.0f);
                glRectf(x + 0.1f, y + 0.5f, x + 0.9f, y + 0.9f);
            }

            char line1[8], line2[8];
            sprintf(line1, "M%d", male->family_id);
            sprintf(line2, "%d", families[i].basket_bananas);
            draw_two_line_text(x, y, line1, line2, 1.0f, 1.0f, male->fighting ? 1.0f : 0.0f);
        }
    }

    // Babies
    for (int i = 0; i < total_families; i++) {
        for (int j = 0; j < families[i].num_babies; j++) {
            BabyApe *baby = &families[i].babies[j];
            if (baby->active) {
                float x = baby->position_col;
                float y = baby->position_row;

                glColor3f(0.4f, 0.15f, 0.0f);
                draw_circle(x + 0.53f, y + 0.47f, 0.38f, 24);

                glColor3f(1.0f, 0.5f, 0.1f);
                draw_circle(x + 0.5f, y + 0.5f, 0.38f, 24);

                glColor3f(1.0f, 0.7f, 0.4f);
                draw_circle(x + 0.4f, y + 0.6f, 0.12f, 16);

                char line1[8], line2[8];
                sprintf(line1, "B%d", baby->family_id);
                sprintf(line2, "%d", baby->bananas_eaten);
                draw_two_line_text(x, y, line1, line2, 1.0f, 1.0f, 1.0f);
            }
        }
    }

    // Add fight effects
    for (int i = 0; i < total_families; i++) {
        MaleApe *male = families[i].male;
        if (male->active && male->fighting) {
            add_fight_effect(male->position_col + 0.5f, male->position_row + 0.5f);
        }
    }
    for (int i = 0; i < total_families; i++) {
        FemaleApe *female = families[i].female;
        if (female->active && female->in_maze && female->fighting) {
            add_fight_effect(female->position_col + 0.5f, female->position_row + 0.5f);
        }
    }

    pthread_mutex_lock(&effects_mutex);
    for (int i = 0; i < MAX_FIGHT_EFFECTS; i++) {
        if (fight_effects[i].active) {
            float progress = 1.0f - (fight_effects[i].frames_remaining / 60.0f);
            draw_explosion(fight_effects[i].x, fight_effects[i].y, progress);

            fight_effects[i].frames_remaining--;
            if (fight_effects[i].frames_remaining <= 0) {
                fight_effects[i].active = false;
            }
        }
    }
    pthread_mutex_unlock(&effects_mutex);

    animation_frame++;
    glFlush();
}

static void request_clean_exit(const char *reason)
{
    if (reason) {
        printf("\n%s\n", reason);
    }
    stop_simulation();
#ifndef __APPLE__
    glutLeaveMainLoop();
#else
    exit(0);
#endif
}

void keyboard_func(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;

    if (key == 27 || key == 'q' || key == 'Q') {
        request_clean_exit("User requested exit via keyboard");
    }
}

void timer_func(int value)
{
    (void)value;

    // Every ~3 seconds
    frame_count++;
    if (frame_count >= 100) {
        frame_count = 0;
        if (check_termination_conditions()) {
            request_clean_exit("Termination condition reached");
            return;
        }
        print_simulation_stats();
    }

    if (simulation_done || !is_simulation_running()) {
        request_clean_exit("Simulation stopped");
        return;
    }

    glutPostRedisplay();
    glutTimerFunc(33, timer_func, 0);
}

/* تهيئة الجرافيكس */
void init_graphics(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(800, 820);
    glutCreateWindow("🍌 Apes Banana Collection Simulation 🐵");

    glClearColor(0.9f, 0.95f, 1.0f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, config.maze_cols, 0, config.maze_rows + 2);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glutDisplayFunc(draw_scene);
    glutKeyboardFunc(keyboard_func);
    glutTimerFunc(33, timer_func, 0);
}

/* Start graphics loop */
void start_graphics(void)
{
    glutMainLoop();
}
