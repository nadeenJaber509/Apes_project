// graphics.c - Ape Banana Collection Simulation Display
// Styled to match reference with dark background, sidebar stats, and family legend

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "graphics.h"
#include "simulation.h"
#include "maze.h"
#include "config.h"
#include "family.h"

// Window dimensions
#define WINDOW_WIDTH 1100
#define WINDOW_HEIGHT 750
#define SIDEBAR_WIDTH 220

// Statistics tracking
static int female_fights = 0;
static int male_fights = 0;
static int baby_steals = 0;
static int total_collected = 0;
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// Animation
static int animation_frame = 0;
static int frame_count = 0;

// Floating text notifications
#define MAX_NOTIFICATIONS 30
typedef struct {
    float x, y;
    char text[32];
    float r, g, b;
    int frames_remaining;
    bool active;
} Notification;

static Notification notifications[MAX_NOTIFICATIONS];
static pthread_mutex_t notif_mutex = PTHREAD_MUTEX_INITIALIZER;

// Event log for sidebar display
#define MAX_LOG_LINES 12
#define MAX_LOG_LENGTH 40
static char event_log[MAX_LOG_LINES][MAX_LOG_LENGTH];
static int log_index = 0;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Add event to the log display
void graphics_log_event(const char *event) {
    pthread_mutex_lock(&log_mutex);
    strncpy(event_log[log_index], event, MAX_LOG_LENGTH - 1);
    event_log[log_index][MAX_LOG_LENGTH - 1] = '\0';
    log_index = (log_index + 1) % MAX_LOG_LINES;
    pthread_mutex_unlock(&log_mutex);
}

// Family colors (matching reference)
static float family_colors[][3] = {
    {0.8f, 0.0f, 0.0f},    // Family 0 - Red
    {0.0f, 0.0f, 0.9f},    // Family 1 - Blue
    {0.0f, 0.7f, 0.7f},    // Family 2 - Cyan
    {0.9f, 0.0f, 0.9f},    // Family 3 - Magenta
    {0.0f, 0.8f, 0.0f},    // Family 4 - Green
    {1.0f, 0.5f, 0.0f},    // Family 5 - Orange
    {1.0f, 1.0f, 0.0f},    // Family 6 - Yellow
    {0.5f, 0.0f, 0.5f},    // Family 7 - Purple
    {0.0f, 0.5f, 0.0f},    // Family 8 - Dark Green
    {0.5f, 0.5f, 0.5f},    // Family 9 - Gray
};

// Public functions to update stats (called from other files)
void graphics_add_female_fight(void) {
    pthread_mutex_lock(&stats_mutex);
    female_fights++;
    pthread_mutex_unlock(&stats_mutex);
}

void graphics_add_male_fight(void) {
    pthread_mutex_lock(&stats_mutex);
    male_fights++;
    pthread_mutex_unlock(&stats_mutex);
}

void graphics_add_baby_steal(void) {
    pthread_mutex_lock(&stats_mutex);
    baby_steals++;
    pthread_mutex_unlock(&stats_mutex);
}

void graphics_add_collected(int amount) {
    pthread_mutex_lock(&stats_mutex);
    total_collected += amount;
    pthread_mutex_unlock(&stats_mutex);
}

// Add floating notification
void add_notification(float x, float y, const char *text, float r, float g, float b) {
    pthread_mutex_lock(&notif_mutex);
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (!notifications[i].active) {
            notifications[i].x = x;
            notifications[i].y = y;
            strncpy(notifications[i].text, text, 31);
            notifications[i].text[31] = '\0';
            notifications[i].r = r;
            notifications[i].g = g;
            notifications[i].b = b;
            notifications[i].frames_remaining = 90; // 3 seconds
            notifications[i].active = true;
            break;
        }
    }
    pthread_mutex_unlock(&notif_mutex);
}

// Draw text at position
static void draw_text(float x, float y, const char *text, void *font) {
    glRasterPos2f(x, y);
    for (const char *c = text; *c; c++) {
        glutBitmapCharacter(font, *c);
    }
}

// Draw a filled rectangle
static void draw_rect(float x1, float y1, float x2, float y2) {
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

// Draw a rectangle outline
static void draw_rect_outline(float x1, float y1, float x2, float y2) {
    glBegin(GL_LINE_LOOP);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

// Draw banana icon
static void draw_banana(float cx, float cy, float size) {
    glColor3f(1.0f, 0.85f, 0.0f);
    // Simple oval banana
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 20; i++) {
        float angle = 2.0f * 3.14159f * i / 20.0f;
        float x = cx + size * 0.5f * cosf(angle);
        float y = cy + size * 0.3f * sinf(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

// Get family color
static void get_family_color(int family_id, float *r, float *g, float *b) {
    int idx = family_id % 10;
    *r = family_colors[idx][0];
    *g = family_colors[idx][1];
    *b = family_colors[idx][2];
}

// Draw the maze area
static void draw_maze(float offset_x, float offset_y, float cell_size) {
    if (maze.cells == NULL) return;
    
    // Draw maze background (dark border)
    glColor3f(0.08f, 0.15f, 0.08f);
    draw_rect(offset_x - 8, offset_y - 8, 
              offset_x + maze.cols * cell_size + 8, 
              offset_y + maze.rows * cell_size + 8);
    
    for (int r = 0; r < maze.rows; r++) {
        for (int c = 0; c < maze.cols; c++) {
            float x = offset_x + c * cell_size;
            float y = offset_y + (maze.rows - 1 - r) * cell_size;
            Cell cell = maze.cells[r][c];
            
            if (cell.type == CELL_OBSTACLE) {
                // Dark green walls (like in reference)
                glColor3f(0.1f, 0.2f, 0.1f);
                draw_rect(x, y, x + cell_size, y + cell_size);
                // Darker border for depth
                glColor3f(0.05f, 0.1f, 0.05f);
                glLineWidth(1.0f);
                draw_rect_outline(x, y, x + cell_size, y + cell_size);
            } else {
                // Light green path
                glColor3f(0.35f, 0.55f, 0.35f);
                draw_rect(x, y, x + cell_size, y + cell_size);
                
                // Grid lines
                glColor3f(0.25f, 0.4f, 0.25f);
                glLineWidth(1.0f);
                draw_rect_outline(x, y, x + cell_size, y + cell_size);
                
                // Draw bananas (yellow dots like in reference)
                if (cell.type == CELL_BANANA && cell.bananas > 0) {
                    glColor3f(1.0f, 0.85f, 0.0f);
                    draw_banana(x + cell_size/2, y + cell_size/2, cell_size * 0.5f);
                    
                    // Show banana count
                    char count[8];
                    sprintf(count, "%d", cell.bananas);
                    glColor3f(0.0f, 0.0f, 0.0f);
                    draw_text(x + cell_size * 0.4f, y + cell_size * 0.35f, count, GLUT_BITMAP_HELVETICA_12);
                }
            }
        }
    }
}

// Draw apes on the maze
static void draw_apes(float offset_x, float offset_y, float cell_size) {
    if (families == NULL) return;
    
    // Draw Males (squares with colored borders) - positioned at bottom border
    for (int i = 0; i < total_families; i++) {
        MaleApe *male = families[i].male;
        if (male->active && !families[i].withdrawn) {
            float r, g, b;
            get_family_color(i, &r, &g, &b);
            
            // Males patrol outside maze - draw at bottom border area
            float x, y;
            if (male->position_row >= maze.rows) {
                // Outside maze - draw at bottom border
                x = offset_x + male->position_col * cell_size * 0.5f;
                y = offset_y - 55;  // Below maze
            } else {
                // Inside maze (shouldn't happen normally)
                x = offset_x + male->position_col * cell_size;
                y = offset_y + (maze.rows - 1 - male->position_row) * cell_size;
            }
            
            // Shake if fighting
            if (male->fighting) {
                float shake = sinf(animation_frame * 0.5f) * 3.0f;
                x += shake;
            }
            
            float size = cell_size * 1.2f;
            
            // Border (family color)
            glColor3f(r, g, b);
            glLineWidth(3.0f);
            draw_rect_outline(x, y, x + size, y + size);
            
            // Inner fill (darker)
            glColor3f(r * 0.4f, g * 0.4f, b * 0.4f);
            draw_rect(x + 2, y + 2, x + size - 2, y + size - 2);
            
            // Label
            char label[16];
            sprintf(label, "M%d", i);
            glColor3f(1.0f, 1.0f, 1.0f);
            draw_text(x + size * 0.3f, y + size * 0.55f, label, GLUT_BITMAP_HELVETICA_10);
            
            // Energy
            sprintf(label, "E:%d", male->energy);
            glColor3f(0.8f, 0.8f, 0.0f);
            draw_text(x + size * 0.15f, y + size * 0.2f, label, GLUT_BITMAP_HELVETICA_10);
        }
    }
    
    // Draw Females (similar style, slightly different)
    for (int i = 0; i < total_families; i++) {
        FemaleApe *female = families[i].female;
        if (female->active && female->in_maze && !families[i].withdrawn) {
            float x = offset_x + female->position_col * cell_size;
            float y = offset_y + (maze.rows - 1 - female->position_row) * cell_size;
            float r, g, b;
            get_family_color(i, &r, &g, &b);
            
            // Shake if fighting
            if (female->fighting) {
                float shake = sinf(animation_frame * 0.5f) * 3.0f;
                x += shake;
            }
            
            float pad = cell_size * 0.15f;
            
            // Border (family color, thinner)
            glColor3f(r, g, b);
            glLineWidth(2.0f);
            draw_rect_outline(x + pad, y + pad, x + cell_size - pad, y + cell_size - pad);
            
            // Inner fill (lighter than male)
            glColor3f(r * 0.6f + 0.2f, g * 0.6f + 0.2f, b * 0.6f + 0.2f);
            draw_rect(x + pad + 2, y + pad + 2, x + cell_size - pad - 2, y + cell_size - pad - 2);
            
            // Label
            char label[16];
            sprintf(label, "F%d", i);
            glColor3f(0.0f, 0.0f, 0.0f);
            draw_text(x + cell_size * 0.32f, y + cell_size * 0.45f, label, GLUT_BITMAP_HELVETICA_10);
            
            // Banana count
            if (female->bananas_collected > 0) {
                sprintf(label, "%d", female->bananas_collected);
                glColor3f(1.0f, 0.8f, 0.0f);
                draw_text(x + cell_size * 0.35f, y + cell_size * 0.2f, label, GLUT_BITMAP_HELVETICA_10);
            }
        }
    }
    
    // Draw Babies (smallest, with border) - positioned near their dad
    for (int i = 0; i < total_families; i++) {
        for (int j = 0; j < families[i].num_babies; j++) {
            BabyApe *baby = &families[i].babies[j];
            if (baby->active && !families[i].withdrawn) {
                float r, g, b;
                get_family_color(i, &r, &g, &b);
                
                // Babies patrol outside maze with dad - draw at bottom border
                float x, y;
                if (baby->position_row >= maze.rows) {
                    // Outside maze - draw at bottom border near dad
                    x = offset_x + baby->position_col * cell_size * 0.5f;
                    y = offset_y - 35;  // Slightly above dad
                } else {
                    // Inside maze (when stealing)
                    x = offset_x + baby->position_col * cell_size;
                    y = offset_y + (maze.rows - 1 - baby->position_row) * cell_size;
                }
                
                float size = cell_size * 0.7f;
                
                // Border
                glColor3f(r, g, b);
                glLineWidth(2.0f);
                draw_rect_outline(x, y, x + size, y + size);
                
                // Fill (lightest)
                glColor3f(r * 0.5f + 0.5f, g * 0.5f + 0.5f, b * 0.5f + 0.5f);
                draw_rect(x + 1, y + 1, x + size - 1, y + size - 1);
                
                // Label
                char label[16];
                sprintf(label, "B%d", i);
                glColor3f(0.0f, 0.0f, 0.0f);
                draw_text(x + size * 0.2f, y + size * 0.35f, label, GLUT_BITMAP_HELVETICA_10);
            }
        }
    }
}

// Draw family baskets around the maze border
static void draw_baskets(float offset_x, float offset_y, float cell_size) {
    if (families == NULL) return;
    
    float maze_width = maze.cols * cell_size;
    float maze_height = maze.rows * cell_size;
    
    for (int i = 0; i < total_families; i++) {
        if (families[i].withdrawn) continue;
        
        // Position baskets around the maze border
        float x, y;
        int per_side = (total_families + 3) / 4;
        int side = i / per_side;
        int pos = i % per_side;
        
        switch (side) {
            case 0: // Left
                x = offset_x - 40;
                y = offset_y + (pos + 0.5f) * (maze_height / per_side);
                break;
            case 1: // Top
                x = offset_x + (pos + 0.5f) * (maze_width / per_side);
                y = offset_y + maze_height + 10;
                break;
            case 2: // Right
                x = offset_x + maze_width + 10;
                y = offset_y + (pos + 0.5f) * (maze_height / per_side);
                break;
            default: // Bottom
                x = offset_x + (pos + 0.5f) * (maze_width / per_side);
                y = offset_y - 40;
                break;
        }
        
        float r, g, b;
        get_family_color(i, &r, &g, &b);
        
        // Basket border
        glColor3f(r, g, b);
        glLineWidth(3.0f);
        draw_rect_outline(x, y, x + 32, y + 28);
        
        // Basket fill
        glColor3f(r * 0.3f, g * 0.3f, b * 0.3f);
        draw_rect(x + 2, y + 2, x + 30, y + 26);
        
        // Basket label and count
        char label[16];
        sprintf(label, "%d", families[i].basket_bananas);
        glColor3f(1.0f, 1.0f, 1.0f);
        draw_text(x + 10, y + 8, label, GLUT_BITMAP_HELVETICA_12);
    }
}

// Draw notifications
static void draw_notifications(float offset_x, float offset_y, float cell_size) {
    pthread_mutex_lock(&notif_mutex);
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (notifications[i].active) {
            float alpha = notifications[i].frames_remaining / 90.0f;
            float rise = (90 - notifications[i].frames_remaining) * 0.3f;
            
            float x = offset_x + notifications[i].x * cell_size;
            float y = offset_y + (maze.rows - 1 - notifications[i].y) * cell_size + rise + cell_size;
            
            // Shadow
            glColor3f(0.0f, 0.0f, 0.0f);
            draw_text(x + 1, y - 1, notifications[i].text, GLUT_BITMAP_HELVETICA_12);
            
            // Text
            glColor3f(notifications[i].r * alpha + (1-alpha), 
                     notifications[i].g * alpha + (1-alpha), 
                     notifications[i].b * alpha + (1-alpha));
            draw_text(x, y, notifications[i].text, GLUT_BITMAP_HELVETICA_12);
            
            notifications[i].frames_remaining--;
            if (notifications[i].frames_remaining <= 0) {
                notifications[i].active = false;
            }
        }
    }
    pthread_mutex_unlock(&notif_mutex);
}

// Draw the sidebar with statistics
static void draw_sidebar(float x, float y, float width, float height) {
    // Background
    glColor3f(0.12f, 0.12f, 0.18f);
    draw_rect(x, y, x + width, y + height);
    
    // Border
    glColor3f(0.3f, 0.3f, 0.4f);
    glLineWidth(2.0f);
    draw_rect_outline(x, y, x + width, y + height);
    
    float text_x = x + 15;
    float text_y = y + height - 35;
    
    // STATISTICS header
    glColor3f(1.0f, 1.0f, 1.0f);
    draw_text(text_x + 50, text_y, "STATISTICS", GLUT_BITMAP_HELVETICA_18);
    text_y -= 25;
    
    // Horizontal line
    glColor3f(0.4f, 0.4f, 0.5f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(x + 10, text_y + 5);
    glVertex2f(x + width - 10, text_y + 5);
    glEnd();
    text_y -= 15;
    
    // Stats
    char buf[64];
    glColor3f(0.85f, 0.85f, 0.85f);
    
    sprintf(buf, "Time: %d / %d sec", get_elapsed_time(), config.max_simulation_time);
    draw_text(text_x, text_y, buf, GLUT_BITMAP_HELVETICA_12);
    text_y -= 18;
    
    pthread_mutex_lock(&stats_mutex);
    sprintf(buf, "Female Fights: %d", female_fights);
    draw_text(text_x, text_y, buf, GLUT_BITMAP_HELVETICA_12);
    text_y -= 18;
    
    sprintf(buf, "Male Fights: %d", male_fights);
    draw_text(text_x, text_y, buf, GLUT_BITMAP_HELVETICA_12);
    text_y -= 18;
    
    sprintf(buf, "Baby Steals: %d", baby_steals);
    draw_text(text_x, text_y, buf, GLUT_BITMAP_HELVETICA_12);
    text_y -= 18;
    
    sprintf(buf, "Total Collected: %d", total_collected);
    draw_text(text_x, text_y, buf, GLUT_BITMAP_HELVETICA_12);
    text_y -= 18;
    pthread_mutex_unlock(&stats_mutex);
    
    // Count withdrawn
    int withdrawn = 0;
    for (int i = 0; i < total_families; i++) {
        if (families[i].withdrawn) withdrawn++;
    }
    sprintf(buf, "Withdrawn: %d / %d", withdrawn, config.max_withdrawn_families);
    draw_text(text_x, text_y, buf, GLUT_BITMAP_HELVETICA_12);
    text_y -= 30;
    
    // FAMILY STANDINGS header
    glColor3f(1.0f, 1.0f, 0.8f);
    draw_text(text_x + 25, text_y, "FAMILY STANDINGS", GLUT_BITMAP_HELVETICA_12);
    text_y -= 20;
    
    // Family progress bars
    for (int i = 0; i < total_families && i < 10; i++) {
        float r, g, b;
        get_family_color(i, &r, &g, &b);
        
        // Rank number
        glColor3f(0.6f, 0.6f, 0.6f);
        sprintf(buf, "#%d", i + 1);
        draw_text(text_x, text_y, buf, GLUT_BITMAP_HELVETICA_10);
        
        // Family color box
        glColor3f(r, g, b);
        draw_rect(text_x + 22, text_y - 2, text_x + 34, text_y + 9);
        
        // Family label
        sprintf(buf, "F%d:", i);
        glColor3f(0.8f, 0.8f, 0.8f);
        draw_text(text_x + 38, text_y, buf, GLUT_BITMAP_HELVETICA_10);
        
        // Progress bar background
        float bar_x = text_x + 60;
        float bar_width = width - 95;
        glColor3f(0.2f, 0.2f, 0.25f);
        draw_rect(bar_x, text_y - 2, bar_x + bar_width, text_y + 9);
        
        // Progress bar fill
        int bananas = families[i].basket_bananas;
        float fill = (float)bananas / (float)config.family_max_bananas;
        if (fill > 1.0f) fill = 1.0f;
        
        glColor3f(r, g, b);
        draw_rect(bar_x, text_y - 2, bar_x + bar_width * fill, text_y + 9);
        
        // Banana count
        sprintf(buf, "%d", bananas);
        glColor3f(1.0f, 1.0f, 1.0f);
        draw_text(bar_x + bar_width + 5, text_y, buf, GLUT_BITMAP_HELVETICA_10);
        
        // Withdrawn indicator
        if (families[i].withdrawn) {
            glColor3f(1.0f, 0.3f, 0.3f);
            draw_text(bar_x + bar_width - 15, text_y, "(W)", GLUT_BITMAP_HELVETICA_10);
        }
        
        text_y -= 16;
    }
    
    text_y -= 10;
    
    // LEGEND header (simplified - just symbols)
    glColor3f(1.0f, 1.0f, 1.0f);
    draw_text(text_x + 65, text_y, "LEGEND", GLUT_BITMAP_HELVETICA_12);
    text_y -= 16;
    
    // Simplified legend
    glColor3f(0.7f, 0.7f, 0.7f);
    draw_text(text_x, text_y, "M=Male F=Female B=Baby", GLUT_BITMAP_HELVETICA_10);
    text_y -= 20;
    
    // EVENT LOG header
    glColor3f(0.3f, 0.8f, 1.0f);
    draw_text(text_x + 45, text_y, "EVENT LOG", GLUT_BITMAP_HELVETICA_12);
    text_y -= 5;
    
    // Draw log background
    glColor3f(0.05f, 0.08f, 0.1f);
    draw_rect(x + 5, y + 10, x + width - 5, text_y);
    
    // Draw log border
    glColor3f(0.2f, 0.4f, 0.5f);
    glLineWidth(1.0f);
    draw_rect_outline(x + 5, y + 10, x + width - 5, text_y);
    
    text_y -= 12;
    
    // Draw event log lines
    pthread_mutex_lock(&log_mutex);
    for (int i = 0; i < MAX_LOG_LINES; i++) {
        int idx = (log_index - 1 - i + MAX_LOG_LINES) % MAX_LOG_LINES;
        if (event_log[idx][0] != '\0') {
            // Color based on event type
            if (strstr(event_log[idx], "fight") || strstr(event_log[idx], "Fight") || 
                strstr(event_log[idx], "WON") || strstr(event_log[idx], "LOST")) {
                glColor3f(1.0f, 0.5f, 0.5f);  // Red for fights
            } else if (strstr(event_log[idx], "stole") || strstr(event_log[idx], "Steal")) {
                glColor3f(1.0f, 0.8f, 0.3f);  // Yellow for steals
            } else if (strstr(event_log[idx], "delivered") || strstr(event_log[idx], "collected")) {
                glColor3f(0.5f, 1.0f, 0.5f);  // Green for collection
            } else if (strstr(event_log[idx], "Withdrawn") || strstr(event_log[idx], "stopped")) {
                glColor3f(1.0f, 0.3f, 0.3f);  // Bright red for withdrawals
            } else {
                glColor3f(0.6f, 0.7f, 0.8f);  // Light blue for other events
            }
            draw_text(text_x - 5, text_y, event_log[idx], GLUT_BITMAP_HELVETICA_10);
            text_y -= 11;
            if (text_y < y + 15) break;
        }
    }
    pthread_mutex_unlock(&log_mutex);
}

// Draw title bar
static void draw_title_bar(float width) {
    float height = 30;
    float y = WINDOW_HEIGHT - height;
    
    // Background
    glColor3f(0.15f, 0.08f, 0.08f);
    draw_rect(0, y, width, y + height);
    
    // Bottom border
    glColor3f(0.4f, 0.2f, 0.2f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(0, y);
    glVertex2f(width, y);
    glEnd();
    
    // Title
    glColor3f(1.0f, 0.9f, 0.3f);
    draw_text(width / 2 - 130, y + 8, "Ape Banana Collection Simulation", GLUT_BITMAP_HELVETICA_18);
    
    // Fight notification (if any fights happening)
    bool any_male_fight = false;
    int fighter1 = -1, fighter2 = -1;
    for (int i = 0; i < total_families && !any_male_fight; i++) {
        if (families[i].male->fighting) {
            fighter1 = i;
            for (int j = i + 1; j < total_families; j++) {
                if (families[j].male->fighting) {
                    fighter2 = j;
                    any_male_fight = true;
                    break;
                }
            }
        }
    }
    
    if (any_male_fight && fighter1 >= 0 && fighter2 >= 0) {
        char fight_text[64];
        sprintf(fight_text, "FAMILY %d vs FAMILY %d", fighter1, fighter2);
        glColor3f(1.0f, 0.2f, 0.2f);
        draw_text(15, y + 8, fight_text, GLUT_BITMAP_HELVETICA_12);
    }
}

// Main draw function
void draw_scene(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (maze.cells == NULL || families == NULL) {
        glFlush();
        return;
    }
    
    // Calculate maze dimensions
    float maze_area_width = WINDOW_WIDTH - SIDEBAR_WIDTH - 100;
    float maze_area_height = WINDOW_HEIGHT - 100;
    float cell_size = fminf(maze_area_width / maze.cols, maze_area_height / maze.rows);
    
    float offset_x = 55;
    float offset_y = 40;
    
    // Draw title bar
    draw_title_bar(WINDOW_WIDTH);
    
    // Draw maze
    draw_maze(offset_x, offset_y, cell_size);
    
    // Draw baskets
    draw_baskets(offset_x, offset_y, cell_size);
    
    // Draw apes
    draw_apes(offset_x, offset_y, cell_size);
    
    // Draw floating notifications
    draw_notifications(offset_x, offset_y, cell_size);
    
    // Draw sidebar
    draw_sidebar(WINDOW_WIDTH - SIDEBAR_WIDTH - 15, 15, SIDEBAR_WIDTH, WINDOW_HEIGHT - 60);
    
    animation_frame++;
    glFlush();
}

// Keyboard handler
static void request_clean_exit(const char *reason) {
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

void keyboard_func(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    
    if (key == 27 || key == 'q' || key == 'Q') {
        request_clean_exit("User requested exit via keyboard");
    }
}

// Timer function
void timer_func(int value) {
    (void)value;
    
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

// Initialize graphics
void init_graphics(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Ape Banana Collection Simulation");
    
    // Dark background (like reference)
    glClearColor(0.08f, 0.1f, 0.12f, 1.0f);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Initialize notifications
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        notifications[i].active = false;
    }
    
    glutDisplayFunc(draw_scene);
    glutKeyboardFunc(keyboard_func);
    glutTimerFunc(33, timer_func, 0);
}

// Start graphics loop
void start_graphics(void) {
    glutMainLoop();
}
