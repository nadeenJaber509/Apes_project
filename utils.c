#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"
#include "graphics.h"

int random_int(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

float random_float(float min, float max)
{
    float scale = rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

void sleep_seconds(int seconds)
{
    sleep(seconds);
}

void sleep_milliseconds(int milliseconds)
{
    usleep(milliseconds * 1000);
}

void log_event(const char *format, ...)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    // Print to terminal
    printf("[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
    
    // Also send to graphics log (without timestamp for brevity)
    char log_buf[128];
    va_list args2;
    va_start(args2, format);
    vsnprintf(log_buf, sizeof(log_buf), format, args2);
    va_end(args2);
    graphics_log_event(log_buf);
}