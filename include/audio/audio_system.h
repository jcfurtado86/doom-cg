#pragma once

#include "audio/audio_engine.h"
#include "audio/audio_tuning.h"
#include "core/camera.h" // Vec3
#include <vector>

// Forward decl
struct Level;

// Listener (posição/orientação do player)
struct AudioListener {
    Vec3 pos;
    Vec3 vel;
    Vec3 forward;
    Vec3 up;
};

// Estado do sistema de áudio (tudo que era global no game.cpp)
struct AudioSystem {
    AudioEngine engine;
    bool ok = false;

    // Buffers
    ALuint bufAmbient = 0;
    ALuint bufShot = 0;
    ALuint bufStep = 0;
    ALuint bufReload = 0;
    ALuint bufHurt = 0;
    ALuint bufClickReload = 0;
    ALuint bufKill = 0;

    ALuint bufLava = 0;
    ALuint bufBreath = 0;
    ALuint bufGrunt = 0;

    ALuint bufEnemy = 0;
    ALuint bufEnemyScream = 0;

    // Sources
    ALuint srcAmbient = 0;
    ALuint srcShot = 0;
    ALuint srcStep = 0;
    ALuint srcReload = 0;
    ALuint srcHurt = 0;
    ALuint srcClickReload = 0;
    ALuint srcKill = 0;

    ALuint srcLava = 0;
    bool lavaPlaying = false;

    ALuint srcBreath = 0;

    ALuint srcGrunt = 0;
    int shotsSinceGrunt = 0;

    bool stepPlaying = false;

    // Inimigos
    std::vector<ALuint> srcEnemies;       // loop 3D por inimigo
    std::vector<ALuint> srcEnemyScreams;  // one-shot 3D por inimigo
    std::vector<float> enemyScreamTimer;  // cooldown randômico
    std::vector<int> enemyPrevState;      // detectar morte "agora"
};

// -------- API do módulo --------

// Inicializa OpenAL, carrega buffers, cria sources "globais" e prepara vetores por inimigo.
// Chame depois do level já estar carregado (para saber quantos inimigos existem).
void audioInit(AudioSystem& a, const Level& level);

// Atualiza listener, passos, loops de inimigos, lava, respiração, screams e kill-detect.
void audioUpdate(
    AudioSystem& a,
    const Level& level,
    const AudioListener& listener,
    float dt,
    bool playerMoving,
    int playerHealth
);

// SFX diretos disparados por gameplay
void audioPlayShot(AudioSystem& a);
void audioPlayReload(AudioSystem& a);
void audioPlayPumpClick(AudioSystem& a);
void audioPlayHurt(AudioSystem& a);
void audioPlayKillAt(AudioSystem& a, float x, float z);

// Utilitário (se você quiser tocar "grunhido" a cada N tiros)
void audioOnPlayerShot(AudioSystem& a);
