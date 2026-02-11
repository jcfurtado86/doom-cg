#pragma once
#include <string>

void menuUpdateFire(float dt, int janelaW, int janelaH); // atualiza partículas
void menuDrawScreen(const std::string& title, const std::string& subtitle, int janelaW, int janelaH, float time);
void menuDrawPause(int janelaW, int janelaH, float time);
