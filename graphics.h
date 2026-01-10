#ifndef GRAPHICS_H
#define GRAPHICS_H

void init_graphics(int argc, char **argv);
void start_graphics(void);

// Statistics tracking functions
void graphics_add_female_fight(void);
void graphics_add_male_fight(void);
void graphics_add_baby_steal(void);
void graphics_add_collected(int amount);

// Notification function
void add_notification(float x, float y, const char *text, float r, float g, float b);
void add_notification_pixel(float px, float py, const char *text, float r, float g, float b);

// Screen position helpers for notifications
void get_male_screen_position(int family_id, float *px, float *py);
void get_baby_screen_position(int family_id, int baby_index, float *px, float *py);

// Event log function
void graphics_log_event(const char *event);

#endif
