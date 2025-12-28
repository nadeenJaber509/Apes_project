#ifndef UTILS_H
#define UTILS_H

int random_int(int min, int max);
float random_float(float min, float max);

void sleep_seconds(int seconds);
void sleep_milliseconds(int milliseconds);

void log_event(const char *format, ...);

#endif