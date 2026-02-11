#include <GL/glut.h>
#include <cmath>
#include <cstdlib>

#include "input/input.h"
#include "input/keystate.h"
#include "core/window.h"
#include "core/game.h" // Adicione isso no topo

void keyboard(unsigned char key, int, int)
{
    // ESC sai do jogo imediatamente em qualquer tela
    if (key == 27) std::exit(0);

    // --- MENU INICIAL ---
    if (currentState == MENU_INICIAL) {
        if (key == 13) { // ENTER
            currentState = JOGANDO;
        }
        return;
    }

    // --- GAME OVER ---
    if (currentState == GAME_OVER) {
        if (key == 13) { // ENTER reinicia
            gameReset();
            currentState = JOGANDO;
        }
        return;
    }

    // --- PAUSE ---
    if (currentState == PAUSADO) {
        if (key == 'p' || key == 'P') {
            currentState = JOGANDO; // Despausa
        }
        return;
    }

    // --- JOGANDO ---
    if (currentState == JOGANDO) {
        if (key == 'p' || key == 'P') {
            currentState = PAUSADO;
            // Para o movimento ao pausar
            keyW = keyA = keyS = keyD = false; 
            return;
        }

        // Controles de Jogo (WASD + R)
        switch (key)
        {
        case 'w': case 'W': keyW = true; break;
        case 's': case 'S': keyS = true; break;
        case 'a': case 'A': keyA = true; break;
        case 'd': case 'D': keyD = true; break;
        case 'r': case 'R': playerTryReload(); break;
        }
    }
}

void keyboardUp(unsigned char key, int, int)
{
    switch (key)
    {
    case 'w': case 'W': keyW = false; break;
    case 's': case 'S': keyS = false; break;
    case 'a': case 'A': keyA = false; break;
    case 'd': case 'D': keyD = false; break;
    }

    if ((key == 13 || key == '\r') && (glutGetModifiers() & GLUT_ACTIVE_ALT))
    {
        altFullScreen();
    }
}
void mouseClick(int button, int state, int x, int y)
{
    // Se apertou o botão esquerdo
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        playerTryAttack();
    }
}


