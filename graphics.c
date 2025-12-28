// graphics.c
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
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

// Helper function to draw a circle (for apes) - MOVED UP
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
void add_fight_effect(float x, float y) {
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
void draw_explosion(float cx, float cy, float progress) {
    float size = 0.3 + progress * 0.7;
    float alpha = 1.0 - progress;
    
    // Draw expanding rings
    for (int ring = 0; ring < 3; ring++) {
        float ring_size = size * (1.0 + ring * 0.3);
        int segments = 16;
        
        // Outer ring (red/orange)
        if (ring == 0) glColor3f(1.0 * alpha, 0.2 * alpha, 0.0);
        else if (ring == 1) glColor3f(1.0 * alpha, 0.5 * alpha, 0.0);
        else glColor3f(1.0 * alpha, 0.8 * alpha, 0.0);
        
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
        float angle = 2.0f * 3.14159f * i / num_spikes + progress * 2.0;
        float inner = 0.1;
        float outer = size * 1.2;
        
        glColor3f(1.0 * alpha, 0.3 * alpha, 0.0);
        glBegin(GL_LINES);
        glVertex2f(cx + inner * cos(angle), cy + inner * sin(angle));
        glVertex2f(cx + outer * cos(angle), cy + outer * sin(angle));
        glEnd();
    }
    
    // Draw center flash
    glColor3f(1.0 * alpha, 1.0 * alpha, 0.5 * alpha);
    draw_circle(cx, cy, 0.15 * (1.0 - progress * 0.5), 12);
    
    // Draw "POW" or "BANG" text
    if (progress < 0.5) {
        glColor3f(1.0, 0.0, 0.0);
        glRasterPos2f(cx - 0.25, cy + size + 0.1);
        const char *text = "POW!";
        for (const char *c = text; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
    }
    
    // Draw particles
    int num_particles = 6;
    for (int i = 0; i < num_particles; i++) {
        float angle = 2.0f * 3.14159f * i / num_particles + animation_frame * 0.1;
        float dist = size * 0.8 * progress;
        float px = cx + dist * cos(angle);
        float py = cy + dist * sin(angle);
        
        glColor3f(1.0 * alpha, 0.6 * alpha, 0.0);
        draw_circle(px, py, 0.08 * (1.0 - progress), 8);
    }
}

// Helper function to draw a rounded rectangle effect with border
void draw_cell_with_border(float x, float y, float r, float g, float b, float border_r, float border_g, float border_b)
{
    // Draw border
    glColor3f(border_r, border_g, border_b);
    glRectf(x, y, x + 1, y + 1);
    
    // Draw inner cell with rounded effect
    glColor3f(r, g, b);
    glRectf(x + 0.06, y + 0.06, x + 0.94, y + 0.94);
    
    // Add highlight for 3D effect
    glColor3f(r + 0.15, g + 0.15, b + 0.15);
    glRectf(x + 0.06, y + 0.55, x + 0.94, y + 0.94);
}

// Helper function to draw text on two lines (centered)
void draw_two_line_text(float x, float y, const char *line1, const char *line2, float r, float g, float b)
{
    glColor3f(r, g, b);
    
    // Line 1 (top) - smaller font
    int len1 = 0;
    for (const char *c = line1; *c; c++) len1++;
    float offset1 = len1 * 0.035;
    glRasterPos2f(x + 0.5 - offset1, y + 0.65);
    for (const char *c = line1; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
    }
    
    // Line 2 (bottom) - larger font for number
    int len2 = 0;
    for (const char *c = line2; *c; c++) len2++;
    float offset2 = len2 * 0.045;
    glRasterPos2f(x + 0.5 - offset2, y + 0.25);
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
    float offset = len * 0.04;
    glRasterPos2f(x + 0.5 - offset, y + 0.4);
    for (const char *c = text; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

/* رسم المتاهة */
void draw_scene(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Don't draw if simulation not initialized
    if (maze.cells == NULL || families == NULL) {
        glFlush();
        return;
    }

    // Draw maze with enhanced visuals
    for (int r = 0; r < maze.rows; r++) {
        for (int c = 0; c < maze.cols; c++) {
            Cell cell = maze.cells[r][c];
            if (cell.type == CELL_OBSTACLE) {
                // Dark obstacles with 3D effect
                draw_cell_with_border(c, r, 0.25, 0.25, 0.3, 0.1, 0.1, 0.15);
            } else if (cell.type == CELL_BANANA) {
                // Golden yellow bananas
                draw_cell_with_border(c, r, 1.0, 0.85, 0.0, 0.8, 0.5, 0.0);
                
                // Draw banana emoji-style icon and count
                if (cell.bananas > 0) {
                    // Draw small banana icon
                    glColor3f(1.0, 0.95, 0.0);
                    draw_circle(c + 0.5, r + 0.65, 0.15, 16);
                    
                    // Draw count below
                    char buf[10];
                    sprintf(buf, "%d", cell.bananas);
                    glColor3f(0.4, 0.2, 0.0);
                    int len = 0;
                    for (const char *ch = buf; *ch; ch++) len++;
                    glRasterPos2f(c + 0.5 - len * 0.05, r + 0.25);
                    for (const char *ch = buf; *ch; ch++) {
                        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *ch);
                    }
                }
            } else {
                // Light cells with subtle pattern
                draw_cell_with_border(c, r, 0.92, 0.94, 0.96, 0.75, 0.78, 0.82);
            }
        }
    }

    // Draw female apes (pink circles)
    for (int i = 0; i < total_families; i++) {
        FemaleApe *female = families[i].female;
        if (female->in_maze && female->active) {
            float x = female->position_col;
            float y = female->position_row;
            
            if (female->fighting) {
                // Fighting effect - red/magenta glow and shake
                float shake = sin(animation_frame * 0.5) * 0.05;
                x += shake;
                
                // Angry glow
                glColor3f(0.9, 0.0, 0.4);
                draw_circle(x + 0.5, y + 0.5, 0.6, 16);
                
                // Shadow
                glColor3f(0.5, 0.0, 0.2);
                draw_circle(x + 0.53, y + 0.47, 0.42, 24);
                
                // Main circle (red/magenta when fighting)
                glColor3f(1.0, 0.0, 0.3);
                draw_circle(x + 0.5, y + 0.5, 0.42, 24);
                
                // Highlight
                glColor3f(1.0, 0.4, 0.5);
                draw_circle(x + 0.4, y + 0.6, 0.15, 16);
            } else {
                // Shadow
                glColor3f(0.4, 0.1, 0.25);
                draw_circle(x + 0.53, y + 0.47, 0.42, 24);
                
                // Main circle (pink)
                glColor3f(1.0, 0.3, 0.6);
                draw_circle(x + 0.5, y + 0.5, 0.42, 24);
                
                // Highlight
                glColor3f(1.0, 0.6, 0.8);
                draw_circle(x + 0.4, y + 0.6, 0.15, 16);
            }
            
            // Text on two lines
            char line1[8], line2[8];
            sprintf(line1, "F%d", female->family_id);
            sprintf(line2, "%d", female->bananas_collected);
            draw_two_line_text(x, y, line1, line2, 1.0, 1.0, 1.0);
        }
    }

    // Draw male apes (blue squares with rounded look)
    for (int i = 0; i < total_families; i++) {
        MaleApe *male = families[i].male;
        if (male->active) {
            float x = male->position_col;
            float y = male->position_row;
            
            if (male->fighting) {
                // Fighting effect - red glow and shake
                float shake = sin(animation_frame * 0.5) * 0.05;
                x += shake;
                
                // Red angry glow
                glColor3f(0.8, 0.0, 0.0);
                draw_circle(x + 0.5, y + 0.5, 0.6, 16);
                
                // Shadow
                glColor3f(0.4, 0.0, 0.0);
                glRectf(x + 0.12, y + 0.08, x + 0.92, y + 0.88);
                
                // Main square (red when fighting)
                glColor3f(0.9, 0.1, 0.1);
                glRectf(x + 0.1, y + 0.1, x + 0.9, y + 0.9);
                
                // Highlight
                glColor3f(1.0, 0.3, 0.3);
                glRectf(x + 0.1, y + 0.5, x + 0.9, y + 0.9);
            } else {
                // Normal blue appearance
                // Shadow
                glColor3f(0.0, 0.15, 0.4);
                glRectf(x + 0.12, y + 0.08, x + 0.92, y + 0.88);
                
                // Main square (blue)
                glColor3f(0.1, 0.4, 0.9);
                glRectf(x + 0.1, y + 0.1, x + 0.9, y + 0.9);
                
                // Highlight
                glColor3f(0.3, 0.6, 1.0);
                glRectf(x + 0.1, y + 0.5, x + 0.9, y + 0.9);
            }
            
            // Text on two lines
            char line1[8], line2[8];
            sprintf(line1, "M%d", male->family_id);
            sprintf(line2, "%d", families[i].basket_bananas);
            draw_two_line_text(x, y, line1, line2, 1.0, 1.0, male->fighting ? 1.0 : 0.0);
        }
    }

    // Draw baby apes (small orange diamonds)
    for (int i = 0; i < total_families; i++) {
        for (int j = 0; j < families[i].num_babies; j++) {
            BabyApe *baby = &families[i].babies[j];
            if (baby->active) {
                float x = baby->position_col;
                float y = baby->position_row;
                
                // Shadow
                glColor3f(0.4, 0.15, 0.0);
                draw_circle(x + 0.53, y + 0.47, 0.38, 24);
                
                // Main circle (orange)
                glColor3f(1.0, 0.5, 0.1);
                draw_circle(x + 0.5, y + 0.5, 0.38, 24);
                
                // Highlight
                glColor3f(1.0, 0.7, 0.4);
                draw_circle(x + 0.4, y + 0.6, 0.12, 16);
                
                // Text on two lines
                char line1[8], line2[8];
                sprintf(line1, "B%d", baby->family_id);
                sprintf(line2, "%d", baby->bananas_eaten);
                draw_two_line_text(x, y, line1, line2, 1.0, 1.0, 1.0);
            }
        }
    }
    
    // Check for fighting males and add effects
    for (int i = 0; i < total_families; i++) {
        MaleApe *male = families[i].male;
        if (male->active && male->fighting) {
            // Add fight effect at male's position
            add_fight_effect(male->position_col + 0.5, male->position_row + 0.5);
        }
    }
    
    // Check for fighting females and add effects
    for (int i = 0; i < total_families; i++) {
        FemaleApe *female = families[i].female;
        if (female->active && female->in_maze && female->fighting) {
            // Add fight effect at female's position
            add_fight_effect(female->position_col + 0.5, female->position_row + 0.5);
        }
    }
    
    // Draw fight effects
    pthread_mutex_lock(&effects_mutex);
    for (int i = 0; i < MAX_FIGHT_EFFECTS; i++) {
        if (fight_effects[i].active) {
            float progress = 1.0 - (fight_effects[i].frames_remaining / 60.0);
            draw_explosion(fight_effects[i].x, fight_effects[i].y, progress);
            
            fight_effects[i].frames_remaining--;
            if (fight_effects[i].frames_remaining <= 0) {
                fight_effects[i].active = false;
            }
        }
    }
    pthread_mutex_unlock(&effects_mutex);
    
    // Update animation frame
    animation_frame++;

    glFlush();
}

void keyboard_func(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;
    if (key == 27 || key == 'q' || key == 'Q') { // ESC or q
        printf("\nUser requested exit via keyboard\n");
        stop_simulation();
        exit(0);
    }
}

void timer_func(int value)
{
    (void)value;
    
    // Check termination every ~100 frames (about 3 seconds at 30fps)
    frame_count++;
    if (frame_count >= 100) {
        frame_count = 0;
        if (check_termination_conditions()) {
            stop_simulation();
            exit(0);
        }
        print_simulation_stats();
    }
    
    if (simulation_done || !is_simulation_running()) {
        exit(0);
    }
    
    glutPostRedisplay();
    glutTimerFunc(33, timer_func, 0); // ~30 FPS
}

/* حلقة OpenGL */
void setup_graphics(void)
{
    glutDisplayFunc(draw_scene);
    glutKeyboardFunc(keyboard_func);
    glutTimerFunc(33, timer_func, 0); // Start timer
}

/* تهيئة الجرافيكس */
void init_graphics(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(800, 820);
    glutCreateWindow("🍌 Apes Banana Collection Simulation 🐵");

    // Smooth background gradient color
    glClearColor(0.9, 0.95, 1.0, 1.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, config.maze_cols, 0, config.maze_rows + 2);
    
    // Enable antialiasing for smoother graphics
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    setup_graphics();
}

/* Start graphics loop (call this from main thread) */
void start_graphics(void)
{
    glutMainLoop();
}

/* إيقاف الجرافيكس */
void stop_graphics(void)
{
    // GLUT ما بدعم إيقاف نظيف، فخليها فاضية
}
