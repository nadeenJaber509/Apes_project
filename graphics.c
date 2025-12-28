// graphics.c
#include <GLUT/glut.h>
#include <pthread.h>
#include "graphics.h"
#include "simulation.h"
#include "maze.h"

static pthread_t graphics_thread;

/* رسم المتاهة (مبدئي وبسيط) */
void draw_scene(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    // لاحقًا ترسمي المتاهة + الموز + القردة
    glFlush();
}

/* حلقة OpenGL */
void* graphics_loop(void *arg)
{
    (void)arg;

    glutDisplayFunc(draw_scene);
    glutMainLoop();

    return NULL;
}

/* تهيئة الجرافيكس */
void init_graphics(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(600, 600);
    glutCreateWindow("Apes Simulation");

    glClearColor(1.0, 1.0, 1.0, 1.0);

    pthread_create(&graphics_thread, NULL, graphics_loop, NULL);
}

/* إيقاف الجرافيكس */
void stop_graphics(void)
{
    // GLUT ما بدعم إيقاف نظيف، فخليها فاضية
}
