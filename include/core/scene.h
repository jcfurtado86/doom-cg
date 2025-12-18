#ifndef SCENE_H
#define SCENE_H

extern float tempo;

extern float camX, camY, camZ;
extern float yaw, pitch;

extern int janelaW;
extern int janelaH;

extern int centerX;
extern int centerY;

extern bool fullScreen;

extern bool ignoreWarp;
extern bool firstMouse;

extern bool keyW;
extern bool keyA;
extern bool keyS;
extern bool keyD;

void altFullScreen();
void atualizaCentroJanela(int w, int h);

void desenhaChao(void);
void desenhaTorresELosangos(void);
void desenhaPiramideDegraus(void);
void setCamera(float x, float y, float z, float newYaw, float newPitch);

#endif
