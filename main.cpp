#include <GL/glew.h>
#include <GL/glut.h>
#include <cstdio>

#include "core/app.h"
#include "core/scene.h"
#include "core/game.h"

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(janelaW, janelaH);
    glutCreateWindow("Um dia vai ser DOOM");

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::printf("Erro GLEW: %s\n", glewGetErrorString(err));
        return 1;
    }

    appInit();

    if (!gameInit("maps/map2.txt"))
        return 1;

    glutMainLoop();
    return 0;
}
