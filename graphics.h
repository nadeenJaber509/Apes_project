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

// Event log function
void graphics_log_event(const char *event);

#endif
