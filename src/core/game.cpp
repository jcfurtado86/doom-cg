#include "audio/AudioTuning.h"
#include "core/game.h"
#include "utils/assets.h"
#include "level/level.h"
#include "core/config.h"
#include "graphics/skybox.h"

#include "audio/AudioEngine.h"
#include "input/keystate.h"

#include "core/camera.h"
#include "input/input.h"
#include "graphics/drawlevel.h"
#include "core/movement.h"
#include "core/entities.h"

#include <GL/glut.h>
#include "core/window.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>

// --- VARIÁVEIS GLOBAIS ---
GLuint texChao;
GLuint texParede;
GLuint texSangue;
GLuint texLava;
GLuint texChaoInterno;
GLuint texParedeInterna;
GLuint texTeto;
GLuint texSkydome;

// Texturas de Entidades
GLuint texEnemies[5];
GLuint texEnemiesRage[5];
GLuint texEnemiesDamage[5];
GLuint texHealth;
GLuint texAmmo;

GLuint texGunDefault, texGunFire1, texGunFire2;
GLuint texGunReload1, texGunReload2;

GLuint texDamage;
float damageAlpha = 0.0f; // Começa invisível

GLuint texHealthOverlay;
float healthAlpha = 0.0f;

GLuint progSangue;
GLuint progLava;

float tempo = 0.0f;

int playerHealth = 100; // Vida do Jogador

// SISTEMA DE MUNIÇÃO
const int MAX_MAGAZINE = 12; // Capacidade do pente
int currentAmmo = 12;        // Balas na arma
int reserveAmmo = 25;        // Balas no bolso

// Estados da animação
enum WeaponState
{
    W_IDLE,
    W_FIRE_1,
    W_FIRE_2,
    W_RETURN,

    W_PUMP, // shotgun pump/cycle

    W_RELOAD_1,
    W_RELOAD_2,
    W_RELOAD_3
};

WeaponState weaponState = W_IDLE;
float weaponTimer = 0.0f;

// --- Assets / Level ---
static GameAssets gAssets;
Level gLevel;

// Configurações da IA
const float ENEMY_SPEED = 2.5f;
const float ENEMY_VIEW_DIST = 15.0f;
const float ENEMY_ATTACK_DIST = 1.5f;

// ======================== AUDIO (OpenAL) =========================
static AudioEngine gAudio;
static bool gAudioOk = false;

// Buffers comuns
static ALuint bufAmbient = 0;
static ALuint bufShot = 0;
static ALuint bufStep = 0;
// ====== novos SFX ======
static ALuint bufReload = 0;
static ALuint bufHurt = 0;
static ALuint bufClickReload = 0;
static ALuint bufKill = 0;

// Sources comuns
static ALuint srcAmbient = 0;
static ALuint srcShot = 0;
static ALuint srcStep = 0;
// ====== novos SFX ======
static ALuint srcReload = 0;
static ALuint srcHurt = 0;
static ALuint srcClickReload = 0; // 2D "na cabeça" (cooldown)
static ALuint srcKill = 0;        // 3D one-shot (reutiliza)

static bool stepPlaying = false;

// ===== NOVOS AUDIOS =====
// Lava (posicional, loop quando perto)
static ALuint bufLava = 0;
static ALuint srcLava = 0;
static bool lavaPlaying = false;

// Respiração (2D/na cabeça, loop sempre)
static ALuint bufBreath = 0;
static ALuint srcBreath = 0;

// Grunhido (one-shot no player, a cada X ataques)
static ALuint bufGrunt = 0;
static ALuint srcGrunt = 0;
static int shotsSinceGrunt = 0;

// ------------------ AUDIO (INIMIGOS 3D) ------------------
// Um buffer único (WAV mono) e 1 source por inimigo
static ALuint bufEnemy = 0;
static std::vector<ALuint> srcEnemies; // 1 source por inimigo
static ALuint bufEnemyScream = 0;
static std::vector<ALuint> srcEnemyScreams; // 1 source de scream por inimigo (3D, one-shot)

// ===== Controle de scream aleatório =====
static std::vector<float> enemyScreamTimer; // tempo até poder gritar de novo

// ===== Detectar "morte agora" =====
static std::vector<int> enemyPrevState;

// Ajusta aqui no include/audio/AudioTuning.h (painel de volumes/distâncias)
// Mantemos aliases locais para o resto do game.cpp continuar simples.
static constexpr float ENEMY_REF_DIST = AudioTuning::ENEMY_REF_DIST;
static constexpr float ENEMY_ROLLOFF = AudioTuning::ENEMY_ROLLOFF;
static constexpr float ENEMY_MAX_DIST = AudioTuning::ENEMY_MAX_DIST;
static constexpr float ENEMY_START_DIST = AudioTuning::ENEMY_START_DIST;
static constexpr float ENEMY_STOP_DIST = AudioTuning::ENEMY_STOP_DIST;
static constexpr float ENEMY_BASE_GAIN = AudioTuning::ENEMY_BASE_GAIN;

// helper: tocar tiro sem depender do input
static void audioPlayShot()
{
    if (!gAudioOk || srcShot == 0)
        return;

    // “shot” no ouvido do player (som 2D)
    alSourcei(srcShot, AL_SOURCE_RELATIVE, AL_TRUE);
    gAudio.setSourcePos(srcShot, {0.0f, 0.0f, 0.0f});
    gAudio.setSourceDistance(srcShot, 1.0f, 0.0f, 1000.0f);

    gAudio.stop(srcShot); // garante restart se spam
    gAudio.play(srcShot);
}

// helper: toca recarregamento da arma
static void audioPlayReload()
{
    if (!gAudioOk || srcReload == 0)
        return;
    gAudio.stop(srcReload); // garante que recomeça do início
    gAudio.play(srcReload);
}

// helper: toca intervalo de disparo a disparo
static void audioPlayClickReload()
{
    if (!gAudioOk || srcClickReload == 0)
        return;
    gAudio.stop(srcClickReload); // reinicia do começo sempre
    gAudio.play(srcClickReload);
}

// helper: toca morte do inimigo
static void audioPlayKillAt(float x, float z)
{
    if (!gAudioOk || srcKill == 0)
        return;
    gAudio.setSourcePos(srcKill, {x, 0.0f, z});
    gAudio.stop(srcKill);
    gAudio.play(srcKill);
}

// helper: toca efeitos extras para inimigos
static void ensureEnemyExtraSourcesCreated()
{
    if (!gAudioOk)
        return;

    const size_t n = gLevel.enemies.size();

    if (srcEnemyScreams.size() != n)
    {
        // para tudo antigo
        for (ALuint s : srcEnemyScreams)
            if (s)
                gAudio.stop(s);
        srcEnemyScreams.clear();
        srcEnemyScreams.resize(n, 0);

        enemyScreamTimer.clear();
        enemyScreamTimer.resize(n, 0.0f);
    }

    if (enemyPrevState.size() != n)
    {
        enemyPrevState.assign(n, 0);
    }
}

// helper: toca dano proprio
static void audioPlayHurt()
{
    if (!gAudioOk || srcHurt == 0)
        return;
    gAudio.stop(srcHurt);
    gAudio.play(srcHurt);
}

static void audioUpdateStep(bool moving)
{
    if (!gAudioOk || srcStep == 0)
        return;

    if (moving && !stepPlaying)
    {
        // Passo “no player” (2D)
        alSourcei(srcStep, AL_SOURCE_RELATIVE, AL_TRUE);
        gAudio.setSourcePos(srcStep, {0.0f, 0.0f, 0.0f});
        gAudio.setSourceDistance(srcStep, 1.0f, 0.0f, 1000.0f);

        gAudio.play(srcStep);
        stepPlaying = true;
    }
    else if (!moving && stepPlaying)
    {
        gAudio.stop(srcStep);
        stepPlaying = false;
    }
}

static void ensureEnemySourcesCreated()
{
    if (!gAudioOk || bufEnemy == 0)
        return;

    const size_t need = gLevel.enemies.size();
    if (srcEnemies.size() == need)
        return;

    // Se mudou quantidade (mapa reload etc), recria tudo simples
    for (ALuint s : srcEnemies)
    {
        if (s)
            gAudio.stop(s);
    }
    srcEnemies.clear();
    srcEnemies.resize(need, 0);

    for (size_t i = 0; i < need; ++i)
    {
        ALuint s = gAudio.createSource(bufEnemy, true); // loop
        if (!s)
            continue;

        // ESSENCIAL: som 3D real (não relativo ao listener)
        alSourcei(s, AL_SOURCE_RELATIVE, AL_FALSE);

        // Atenuação por distância
        gAudio.setSourceDistance(s, ENEMY_REF_DIST, ENEMY_ROLLOFF, ENEMY_MAX_DIST);

        // Gain base (deixa baixo, porque o rolloff já é agressivo)
        gAudio.setSourceGain(s, AudioTuning::MASTER * ENEMY_BASE_GAIN);

        // não toca já: vamos ligar/desligar por proximidade
        gAudio.stop(s);

        srcEnemies[i] = s;
    }

    std::printf("[Audio] Enemy sources: %zu\n", srcEnemies.size());
}

static void updateEnemyAudio(float listenerX, float listenerY, float listenerZ, float dt)
{
    if (!gAudioOk || bufEnemy == 0)
        return;
    ensureEnemySourcesCreated();

    for (size_t i = 0; i < gLevel.enemies.size() && i < srcEnemies.size(); ++i)
    {
        ALuint s = srcEnemies[i];
        if (!s)
            continue;

        auto &en = gLevel.enemies[i];

        // morto -> para
        if (en.state == STATE_DEAD)
        {
            gAudio.stop(s);
            continue;
        }

        // posição 3D do inimigo
        gAudio.setSourcePos(s, {en.x, 0.0f, en.z});

        float dx = en.x - listenerX;
        float dz = en.z - listenerZ;
        float dist = std::sqrt(dx * dx + dz * dz);

        // liga/desliga por proximidade (histerese)
        ALint state = 0;
        alGetSourcei(s, AL_SOURCE_STATE, &state);
        const bool isPlaying = (state == AL_PLAYING);

        if (!isPlaying && dist <= ENEMY_START_DIST)
        {
            gAudio.play(s);
        }
        else if (isPlaying && dist >= ENEMY_STOP_DIST)
        {
            gAudio.stop(s);
        }
    }

    // ================= ENEMY SCREAM (random / positional) =================
    // Timer por inimigo + chance pra não ficar irritante.
    if (bufEnemyScream && !srcEnemyScreams.empty() && enemyScreamTimer.size() >= gLevel.enemies.size())
    {
        for (size_t i = 0; i < gLevel.enemies.size() && i < srcEnemyScreams.size(); ++i)
        {
            auto &en = gLevel.enemies[i];
            if (en.state == STATE_DEAD)
                continue;

            float dxs = en.x - listenerX;
            float dzs = en.z - listenerZ;
            float ds = std::sqrt(dxs * dxs + dzs * dzs);

            // Fora do raio, só vai "andando" o timer mais devagar
            if (ds > AudioTuning::SCREAM_MAX_AUDIBLE_DIST)
            {
                enemyScreamTimer[i] -= dt * 0.25f;
                continue;
            }

            enemyScreamTimer[i] -= dt;
            if (enemyScreamTimer[i] > 0.0f)
                continue;

            // Sorteio pra tocar
            float r = (float)std::rand() / (float)RAND_MAX;
            if (r <= AudioTuning::SCREAM_CHANCE)
            {
                ALuint s = srcEnemyScreams[i];
                if (s)
                {
                    gAudio.setSourcePos(s, {en.x, 0.0f, en.z});
                    gAudio.stop(s); // reinicia do começo
                    gAudio.setSourceGain(s, AudioTuning::MASTER * AudioTuning::ENEMY_SCREAM_GAIN);
                    gAudio.play(s);
                }
            }

            // Próximo intervalo aleatório
            float r2 = (float)std::rand() / (float)RAND_MAX;
            float tmin = AudioTuning::ENEMY_SCREAM_MIN_INTERVAL;
            float tmax = AudioTuning::ENEMY_SCREAM_MAX_INTERVAL;
            enemyScreamTimer[i] = tmin + (tmax - tmin) * r2;
        }
    }
}

static void audioInitOnce()
{
    if (gAudioOk)
        return; // já inicializado

    gAudioOk = gAudio.init();
    if (!gAudioOk)
    {
        std::printf("[Audio] Falha ao iniciar OpenAL (seguindo sem audio)\n");
        return;
    }

    // Modelo de distância (importante pro 3D)
    gAudio.setDistanceModel();

    // Carrega buffers globais (mono primeiro, fallback stereo)
    bufAmbient = gAudio.loadWav("assets/audio/ambient_mono.wav");
    if (!bufAmbient)
        bufAmbient = gAudio.loadWav("assets/audio/ambient.wav");

    bufShot = gAudio.loadWav("assets/audio/shot_mono.wav");
    if (!bufShot)
        bufShot = gAudio.loadWav("assets/audio/shot.wav");

    bufStep = gAudio.loadWav("assets/audio/step_mono.wav");
    if (!bufStep)
        bufStep = gAudio.loadWav("assets/audio/step.wav");

    bufEnemy = gAudio.loadWav("assets/audio/enemy_mono.wav");
    if (!bufEnemy)
        bufEnemy = gAudio.loadWav("assets/audio/enemy.wav");

    // Ambient: som "global" (não-posicional)
    if (bufAmbient)
    {
        srcAmbient = gAudio.createSource(bufAmbient, true);
        // Som relativo ao listener => não muda com distância
        alSourcei(srcAmbient, AL_SOURCE_RELATIVE, AL_TRUE);
        gAudio.setSourcePos(srcAmbient, {0.0f, 0.0f, 0.0f});
        // Rolloff 0 => sem atenuação
        gAudio.setSourceDistance(srcAmbient, 1.0f, 0.0f, 1000.0f);

            gAudio.setSourceGain(srcAmbient, AudioTuning::MASTER * AudioTuning::AMBIENT_GAIN);

        gAudio.play(srcAmbient);
        std::printf("[Audio] Ambient OK (assets/audio/ambient*_mono.wav)\n");
    }
    else
    {
        std::printf("[Audio] Ambient WAV nao encontrado (ambient_mono.wav/ambient.wav)\n");
    }

    // Shot:
    if (bufShot)
    {
        srcShot = gAudio.createSource(bufShot, false);
        alSourcei(srcShot, AL_SOURCE_RELATIVE, AL_TRUE);
        gAudio.setSourceGain(srcShot, AudioTuning::MASTER * AudioTuning::SHOT_GAIN);
        std::printf("[Audio] Shot OK\n");
    }
    else
    {
        std::printf("[Audio] Shot WAV nao encontrado\n");
    }

    // Step: loop enquanto anda
    if (bufStep)
    {
        srcStep = gAudio.createSource(bufStep, true);
        alSourcei(srcStep, AL_SOURCE_RELATIVE, AL_TRUE);
        gAudio.setSourceGain(srcStep, AudioTuning::MASTER * AudioTuning::STEP_GAIN);
        std::printf("[Audio] Step OK\n");
    }
    else
    {
        std::printf("[Audio] Step WAV nao encontrado\n");
    }

    // Enemy buffer (mono + 1 source por inimigo)
    if (bufEnemy)
    {
        std::printf("[Audio] Enemy WAV OK\n");
        ensureEnemySourcesCreated();
    }
    else
    {
        std::printf("[Audio] Enemy WAV nao encontrado\n");
    }

    // --- RELOAD ---
    bufReload = gAudio.loadWav("assets/audio/reload_mono.wav");
    if (!bufReload)
        bufReload = gAudio.loadWav("assets/audio/reload.wav");
    if (bufReload)
    {
        srcReload = gAudio.createSource(bufReload, false);
        // som 2D (sempre no player)
        alSourcei(srcReload, AL_SOURCE_RELATIVE, AL_TRUE);
        gAudio.setSourcePos(srcReload, {0.0f, 0.0f, 0.0f});
        gAudio.setSourceGain(srcReload, AudioTuning::MASTER * AudioTuning::RELOAD_GAIN);
        std::printf("[Audio] Reload OK.\n");
    }
    else
    {
        std::printf("[Audio] Reload NAO carregou.\n");
    }

    // ===== CLICK (cooldown entre tiros) =====
    bufClickReload = gAudio.loadWav("assets/audio/click_reload_mono.wav");
    if (bufClickReload)
    {
        srcClickReload = gAudio.createSource(bufClickReload, false);
        if (srcClickReload)
        {
            alSourcei(srcClickReload, AL_SOURCE_RELATIVE, AL_TRUE); // 2D na cabeça
            alSource3f(srcClickReload, AL_POSITION, 0, 0, 0);
            gAudio.setSourceGain(srcClickReload, AudioTuning::MASTER * AudioTuning::PUMP_GAIN);
        }
    }

    // ===== ENEMY SCREAM =====
    bufEnemyScream = gAudio.loadWav("assets/audio/enemy_scream_mono.wav");

    // ===== KILL =====
    bufKill = gAudio.loadWav("assets/audio/kill_mono.wav");
    if (bufKill)
    {
        srcKill = gAudio.createSource(bufKill, false);
        if (srcKill)
        {
            alSourcei(srcKill, AL_SOURCE_RELATIVE, AL_FALSE); // 3D de verdade
            gAudio.setSourceGain(srcKill, AudioTuning::MASTER * AudioTuning::KILL_GAIN);

            // distância do kill (usa algo parecido com inimigo, ou um pouco menor)
            gAudio.setSourceDistance(srcKill,
                                     AudioTuning::ENEMY_REF_DIST,
                                     AudioTuning::ENEMY_ROLLOFF,
                                     AudioTuning::ENEMY_MAX_DIST);
        }
    }

    // garante vetores de inimigos
    ensureEnemyExtraSourcesCreated();

    if (bufEnemyScream)
    {
        for (size_t i = 0; i < gLevel.enemies.size(); i++)
        {
            ALuint s = gAudio.createSource(bufEnemyScream, false);
            if (!s)
                continue;

            alSourcei(s, AL_SOURCE_RELATIVE, AL_FALSE); // 3D real
            gAudio.setSourceGain(s, AudioTuning::MASTER * AudioTuning::ENEMY_SCREAM_GAIN);

            // Usa o MESMO “perfil de distância” do áudio base do inimigo
            gAudio.setSourceDistance(s,
                                     AudioTuning::ENEMY_REF_DIST,
                                     AudioTuning::ENEMY_ROLLOFF * 0.8f,
                                     AudioTuning::SCREAM_MAX_AUDIBLE_DIST);

            srcEnemyScreams[i] = s;

            // timer inicial random (pra não gritarem todos ao mesmo tempo)
            float r = (float)rand() / (float)RAND_MAX;
            enemyScreamTimer[i] = AudioTuning::ENEMY_SCREAM_MIN_INTERVAL +
                                  r * (AudioTuning::ENEMY_SCREAM_MAX_INTERVAL - AudioTuning::ENEMY_SCREAM_MIN_INTERVAL);
        }
    }

    // --- HURT (dano no player) ---
    bufHurt = gAudio.loadWav("assets/audio/hurt_mono.wav");
    if (!bufHurt)
        bufHurt = gAudio.loadWav("assets/audio/hurt.wav");
    if (bufHurt)
    {
        srcHurt = gAudio.createSource(bufHurt, false);
        // som 2D (sempre no player)
        alSourcei(srcHurt, AL_SOURCE_RELATIVE, AL_TRUE);
        gAudio.setSourcePos(srcHurt, {0.0f, 0.0f, 0.0f});
        gAudio.setSourceGain(srcHurt, AudioTuning::MASTER * AudioTuning::DAMAGE_GAIN);
        std::printf("[Audio] Hurt OK.\n");
    }
    else
    {
        std::printf("[Audio] Hurt NAO carregou.\n");
    }

    // --------- Lava (loop posicional) ----------
    bufLava = gAudio.loadWav("assets/audio/lava_mono.wav");
    if (!bufLava)
        bufLava = gAudio.loadWav("assets/audio/lava.wav"); // fallback
    if (bufLava)
    {
        srcLava = gAudio.createSource(bufLava, true); // loop
        if (srcLava)
        {
            // SOM 3D REAL
            alSourcei(srcLava, AL_SOURCE_RELATIVE, AL_FALSE);

            // Atenuação (ajuste fino aqui)
            // refDist = até onde fica “alto”
            // rolloff = quão rápido cai
            // maxDist = depois disso, praticamente não ouve
            gAudio.setSourceDistance(srcLava,
                                     6.0f, // refDist "cheio" até 6m
                                     0.8f, // rolloff: cai mais devagar (ou seja, escuta mais longe)
                                     25.0f // maxDist: dá pra ouvir até ~25m
            );

            gAudio.setSourceGain(srcLava, AudioTuning::MASTER * AudioTuning::LAVA_GAIN); // volume base da lava
            gAudio.stop(srcLava);                                                        // começa desligado, liga por proximidade
            lavaPlaying = false;

            std::printf("[Audio] Lava OK.\n");
        }
    }
    else
    {
        std::printf("[Audio] Lava WAV nao encontrado.\n");
    }

    // --------- Respiração (loop 2D / na cabeça) ----------
    bufBreath = gAudio.loadWav("assets/audio/breath_mono.wav"); // pode ser mono ou stereo
    if (bufBreath)
    {
        srcBreath = gAudio.createSource(bufBreath, true); // loop
        if (srcBreath)
        {
            // “na cabeça”: som relativo ao listener, sem distância
            alSourcei(srcBreath, AL_SOURCE_RELATIVE, AL_TRUE);
            alSource3f(srcBreath, AL_POSITION, 0.0f, 0.0f, 0.0f);

            // gain base (ajuste)
            gAudio.setSourceGain(srcBreath, 0.0f);
            gAudio.play(srcBreath);

            std::printf("[Audio] Breath OK.\n");
        }
    }
    else
    {
        std::printf("[Audio] Breath WAV nao encontrado.\n");
    }

    // --------- Grunhido (one-shot) ----------
    bufGrunt = gAudio.loadWav("assets/audio/grunt_mono.wav");
    if (!bufGrunt)
        bufGrunt = gAudio.loadWav("assets/audio/grunt.wav"); // fallback
    if (bufGrunt)
    {
        srcGrunt = gAudio.createSource(bufGrunt, false); // one-shot
        if (srcGrunt)
        {
            // pode ser 2D (relativo) pra não “viajar” no espaço
            alSourcei(srcGrunt, AL_SOURCE_RELATIVE, AL_TRUE);
            alSource3f(srcGrunt, AL_POSITION, 0.0f, 0.0f, 0.0f);

            gAudio.setSourceGain(srcGrunt, AudioTuning::MASTER * AudioTuning::GRUNT_GAIN); //<=1 baixo, 1-2 normal, >=2 alto
            std::printf("[Audio] Grunt OK.\n");
        }
    }
    else
    {
        std::printf("[Audio] Grunt WAV nao encontrado.\n");
    }
}

// --- FUNÇÕES AUXILIARES DE LUZ ---
static void setupIndoorLightOnce()
{
    glEnable(GL_LIGHT1);
    GLfloat lampDiffuse[] = {1.7f, 1.7f, 1.8f, 1.0f};
    GLfloat lampSpecular[] = {0, 0, 0, 1.0f};
    GLfloat lampAmbient[] = {0.98f, 0.99f, 1.41f, 1.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lampDiffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, lampSpecular);
    glLightfv(GL_LIGHT1, GL_AMBIENT, lampAmbient);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.6f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.06f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.02f);
    glDisable(GL_LIGHT1);
}

static void setupSunLightOnce()
{
    glEnable(GL_LIGHT0);
    GLfloat sceneAmbient[] = {0.45f, 0.30f, 0.25f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, sceneAmbient);
    GLfloat sunDiffuse[] = {1.4f, 0.55f, 0.30f, 1.0f};
    GLfloat sunSpecular[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpecular);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

static void setSunDirectionEachFrame()
{
    GLfloat sunDir[] = {0.3f, 1.0f, 0.2f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
}

void playerTryReload()
{
    if (weaponState != W_IDLE)
        return;

    if (currentAmmo >= MAX_MAGAZINE)
        return;

    if (reserveAmmo <= 0)
        return;

    std::printf("Recarregando...\n");

    // Inicia a sequência de reload
    weaponState = W_RELOAD_1;

    // TEMPO DO PRIMEIRO FRAME DO RELOAD (parte inicial)
    weaponTimer = 0.50f;

    audioPlayReload(); // toca o som de recarregamento (3.0s)
}

// --- ATAQUE ---
void playerTryAttack()
{
    std::printf("POW! Tentando atacar...\n");

    if (weaponState != W_IDLE)
        return;
    if (currentAmmo <= 0)
    {
        std::printf("CLICK! Sem municao! Aperte R!\n");
        return;
    }

    currentAmmo--;
    // grunhido a cada N tiros (esforço)
    shotsSinceGrunt++;
    if (shotsSinceGrunt >= AudioTuning::GRUNT_EVERY_N_SHOTS)
    {
        shotsSinceGrunt = 0;
        if (gAudioOk && srcGrunt != 0)
        {
            gAudio.stop(srcGrunt); // garante recomeçar
            gAudio.play(srcGrunt);
        }
    }

    // Audio tiro
    audioPlayShot();

    weaponState = W_FIRE_1;
    weaponTimer = 0.08f;

    std::printf("POW! Tiro!\n");

    for (auto &en : gLevel.enemies)
    {
        if (en.state == STATE_DEAD)
            continue;

        float dx = en.x - camX;
        float dz = en.z - camZ;
        float dist = std::sqrt(dx * dx + dz * dz);

        if (dist < 17.0f)
        {
            float radYaw = yaw * 3.14159f / 180.0f;
            float camDirX = std::sin(radYaw);
            float camDirZ = -std::cos(radYaw);

            float toEnemyX = dx / dist;
            float toEnemyZ = dz / dist;

            float dot = camDirX * toEnemyX + camDirZ * toEnemyZ;

            if (dot > 0.6f)
            {
                en.hp -= 30;
                en.hurtTimer = 0.5f;
                std::printf("ACERTOU! HP Inimigo: %.0f\n", en.hp);

                if (en.hp <= 0)
                {
                    en.state = STATE_DEAD;
                    en.respawnTimer = 15.0f;

                    std::printf("Inimigo MORREU! Respawn em 15s.\n");

                    Item drop;
                    drop.type = ITEM_AMMO;
                    drop.x = en.x;
                    drop.z = en.z;
                    drop.active = true;
                    drop.respawnTimer = 0.0f;

                    gLevel.items.push_back(drop);
                    std::printf("DROP! Inimigo derrubou municao!\n");
                }
                return;
            }
        }
    }
}

// --- Walkable ---
bool isWalkable(float x, float z)
{
    float tile = gLevel.metrics.tile;
    float offX = gLevel.metrics.offsetX;
    float offZ = gLevel.metrics.offsetZ;

    int tx = (int)((x - offX) / tile);
    int tz = (int)((z - offZ) / tile);

    const auto &data = gLevel.map.data();

    if (tz < 0 || tz >= (int)data.size())
        return false;
    if (tx < 0 || tx >= (int)data[tz].size())
        return false;

    char c = data[tz][tx];
    if (c == '1' || c == '2')
        return false;

    return true;
}

// --- ENTIDADES ---
void updateEntities(float dt)
{
    // Inimigos
    for (auto &en : gLevel.enemies)
    {
        if (en.state == STATE_DEAD)
        {
            en.respawnTimer -= dt;
            if (en.respawnTimer <= 0.0f)
            {
                en.state = STATE_IDLE;
                en.hp = 100;
                en.x = en.startX;
                en.z = en.startZ;
                en.hurtTimer = 0.0f;
                std::printf("Inimigo RESPAWNOU!\n");
            }
            continue;
        }

        if (en.hurtTimer > 0.0f)
            en.hurtTimer -= dt;

        float dx = camX - en.x;
        float dz = camZ - en.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        switch (en.state)
        {
        case STATE_IDLE:
            if (dist < ENEMY_VIEW_DIST)
                en.state = STATE_CHASE;
            break;

        case STATE_CHASE:
            if (dist < ENEMY_ATTACK_DIST)
            {
                en.state = STATE_ATTACK;
                en.attackCooldown = 0.5f;
            }
            else if (dist > ENEMY_VIEW_DIST * 1.5f)
            {
                en.state = STATE_IDLE;
            }
            else
            {
                float dirX = dx / dist;
                float dirZ = dz / dist;

                float moveStep = ENEMY_SPEED * dt;

                float nextX = en.x + dirX * moveStep;
                if (isWalkable(nextX, en.z))
                    en.x = nextX;

                float nextZ = en.z + dirZ * moveStep;
                if (isWalkable(en.x, nextZ))
                    en.z = nextZ;
            }
            break;

        case STATE_ATTACK:
            if (dist > ENEMY_ATTACK_DIST)
            {
                en.state = STATE_CHASE;
            }
            else
            {
                en.attackCooldown -= dt;
                if (en.attackCooldown <= 0.0f)
                {
                    playerHealth -= 10;
                    en.attackCooldown = 1.0f;
                    std::printf("DANO RECEBIDO! Vida: %d\n", playerHealth);
                    damageAlpha = 1.0f;

                    audioPlayHurt();

                    if (playerHealth <= 0)
                    {
                        std::printf("=== GAME OVER ===\n");
                        playerHealth = 100;
                        loadLevel(gLevel, "maps/map1.txt", 4.0f);
                        applySpawn(gLevel, camX, camZ);
                        damageAlpha = 0.0f;
                    }
                }
            }
            break;

        default:
            break;
        }
    }

    // Itens
    for (auto &item : gLevel.items)
    {
        if (!item.active)
        {
            item.respawnTimer -= dt;
            if (item.respawnTimer <= 0.0f)
            {
                item.active = true;
                std::printf("Item de volta no mapa!\n");
            }
            continue;
        }

        float dx = camX - item.x;
        float dz = camZ - item.z;

        if (dx * dx + dz * dz < 1.0f)
        {
            item.active = false;

            if (item.type == ITEM_HEALTH)
            {
                item.respawnTimer = 15.0f;
                playerHealth += 50;
                if (playerHealth > 100)
                    playerHealth = 100;
                healthAlpha = 1.0f;
                std::printf("PEGOU CURA! Vida: %d\n", playerHealth);
            }
            else if (item.type == ITEM_AMMO)
            {
                item.respawnTimer = 999999.0f;
                reserveAmmo = 20;
                std::printf("PEGOU MUNICAO!\n");
            }
        }
    }
}

// --- INIT ---
bool gameInit(const char *mapPath)
{
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);

    setupSunLightOnce();
    setupIndoorLightOnce();

    if (!loadAssets(gAssets))
        return false;

    texChao = gAssets.texChao;
    texParede = gAssets.texParede;
    texSangue = gAssets.texSangue;
    texLava = gAssets.texLava;
    texChaoInterno = gAssets.texChaoInterno;
    texParedeInterna = gAssets.texParedeInterna;
    texTeto = gAssets.texTeto;

    texSkydome = gAssets.texSkydome;

    texGunDefault = gAssets.texGunDefault;
    texGunFire1 = gAssets.texGunFire1;
    texGunFire2 = gAssets.texGunFire2;
    texGunReload1 = gAssets.texGunReload1;
    texGunReload2 = gAssets.texGunReload2;

    texDamage = gAssets.texDamage;

    for (int i = 0; i < 5; i++)
    {
        texEnemies[i] = gAssets.texEnemies[i];
        texEnemiesRage[i] = gAssets.texEnemiesRage[i];
        texEnemiesDamage[i] = gAssets.texEnemiesDamage[i];
    }

    texHealthOverlay = gAssets.texHealthOverlay;
    texHealth = gAssets.texHealth;
    texAmmo = gAssets.texAmmo;

    progSangue = gAssets.progSangue;
    progLava = gAssets.progLava;

    if (!loadLevel(gLevel, mapPath, GameConfig::TILE_SIZE))
        return false;

    applySpawn(gLevel, camX, camZ);
    camY = GameConfig::PLAYER_EYE_Y;

    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMotion);
    glutSetCursor(GLUT_CURSOR_NONE);

    // Audio init + ambient + enemy sources
    audioInitOnce();

    return true;
}

// --- WEAPON ANIM ---
void updateWeaponAnim(float dt)
{
    // Tempos do tiro (ajuste livre)
    const float TIME_FRAME_1 = 0.05f;
    const float TIME_FRAME_2 = 0.12f;

    // Tempos do RELOAD (SOM = ~3.0s)
    const float RELOAD_T2 = 0.85f; // W_RELOAD_2 (principal)
    const float RELOAD_T3 = 0.25f; // W_RELOAD_3
    // Total: 3.00s
    // Se teu som for, por exemplo, 2.6s ou 3.4s, tu só ajusta esses 3 números:
    // const float RELOAD_T1 = 0.50f;
    // const float RELOAD_T2 = 2.00f;
    // const float RELOAD_T3 = 0.50f;

    if (weaponState == W_IDLE)
        return;

    weaponTimer -= dt;

    if (weaponTimer > 0.0f)
        return;

    // ---------------- TIRO ----------------
    if (weaponState == W_FIRE_1)
    {
        weaponState = W_FIRE_2;
        weaponTimer = TIME_FRAME_2;
    }
    else if (weaponState == W_FIRE_2)
    {
        // Depois do disparo, entra no "pump" (shotgun cycle)
        weaponState = W_PUMP;
        weaponTimer = AudioTuning::PUMP_TIME;

        // Som automático do pump/click
        audioPlayClickReload();
    }
    else if (weaponState == W_RETURN)
    {
        weaponState = W_IDLE;
        weaponTimer = 0.0f;
    }

    else if (weaponState == W_PUMP)
    {
        weaponState = W_IDLE;
        weaponTimer = 0.0f;
    }

    // ---------------- RELOAD ----------------
    else if (weaponState == W_RELOAD_1)
    {
        weaponState = W_RELOAD_2;
        weaponTimer = RELOAD_T2;
    }
    else if (weaponState == W_RELOAD_2)
    {
        weaponState = W_RELOAD_3;
        weaponTimer = RELOAD_T3;
    }
    else if (weaponState == W_RELOAD_3)
    {
        weaponState = W_IDLE;
        weaponTimer = 0.0f;

        // MATEMÁTICA DA RECARGA (só aplica no final, sincronizado com o áudio)
        int needed = MAX_MAGAZINE - currentAmmo;
        if (needed > reserveAmmo)
            needed = reserveAmmo;

        currentAmmo += needed;
        reserveAmmo -= needed;

        std::printf("Arma carregada!\n");
    }
}

// --- Texto ---
void drawText(float x, float y, const char *text, float escala)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(escala, escala, 1.0f);
    glLineWidth(2.0f);

    for (const char *c = text; *c != '\0'; c++)
    {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    }

    glPopMatrix();
}

void drawAmmoHUD()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glDisable(GL_TEXTURE_2D);

    glColor3f(1.0f, 0.8f, 0.0f);

    std::string ammoStr = std::to_string(currentAmmo) + " / " + std::to_string(reserveAmmo);
    drawText(20, janelaH - 50, ammoStr.c_str(), 0.3f);

    glEnable(GL_TEXTURE_2D);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawHealthHUD()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    if (playerHealth > 60)
        glColor3f(0.2f, 1.0f, 0.2f);
    else if (playerHealth > 30)
        glColor3f(1.0f, 1.0f, 0.0f);
    else
        glColor3f(1.0f, 0.0f, 0.0f);

    std::string hpStr = "VIDA: " + std::to_string(playerHealth);

    float x = janelaW - 250.0f;
    float y = janelaH - 50.0f;

    drawText(x, y, hpStr.c_str(), 0.3f);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawWeaponHUD()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint currentTex = texGunDefault;
    if (weaponState == W_FIRE_1 || weaponState == W_RETURN)
        currentTex = texGunFire1;
    else if (weaponState == W_FIRE_2)
        currentTex = texGunFire2;
    else if (weaponState == W_RELOAD_1 || weaponState == W_RELOAD_3)
        currentTex = texGunReload1;
    else if (weaponState == W_RELOAD_2)
        currentTex = texGunReload2;

    if (currentTex == 0)
    {
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        return;
    }

    glBindTexture(GL_TEXTURE_2D, currentTex);
    glColor4f(1, 1, 1, 1);

    float gunH = janelaH * 0.5f;
    float gunW = gunH;
    float x = (janelaW - gunW) / 2.0f;
    float y = 0.0f;

    if (weaponState != W_IDLE)
    {
        y -= 20.0f;
        x += (rand() % 10 - 5);
    }

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x, y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + gunW, y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + gunW, y + gunH);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x, y + gunH);
    glEnd();

    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawDamageOverlay()
{
    if (damageAlpha <= 0.0f)
        return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindTexture(GL_TEXTURE_2D, texDamage);
    glColor4f(1.0f, 1.0f, 1.0f, damageAlpha);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(janelaW, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(janelaW, janelaH);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0, janelaH);
    glEnd();

    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawHealthOverlay()
{
    if (healthAlpha <= 0.0f)
        return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, janelaW, 0, janelaH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindTexture(GL_TEXTURE_2D, texHealthOverlay);
    glColor4f(1.0f, 1.0f, 1.0f, healthAlpha);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(janelaW, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(janelaW, janelaH);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0, janelaH);
    glEnd();

    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

static bool isLavaTile(int tx, int tz)
{
    const auto &data = gLevel.map.data();
    if (tz < 0 || tz >= (int)data.size())
        return false;
    if (tx < 0 || tx >= (int)data[tz].size())
        return false;
    return data[tz][tx] == 'L'; // ajuste se sua lava for outro char
}

// Procura a lava mais próxima em um raio (em tiles) e retorna:
// - found: achou ou não
// - outX/outZ: posição do centro do tile mais próximo (mundo)
// - outDist: distância do player até essa lava
static bool nearestLava(float px, float pz, float &outX, float &outZ, float &outDist)
{
    float tile = gLevel.metrics.tile;
    float offX = gLevel.metrics.offsetX;
    float offZ = gLevel.metrics.offsetZ;

    int ptx = (int)((px - offX) / tile);
    int ptz = (int)((pz - offZ) / tile);

    const int R = 10; // raio de busca em tiles (aumenta se quiser)
    bool found = false;
    float bestD2 = 1e30f;
    float bestX = 0, bestZ = 0;

    for (int dz = -R; dz <= R; ++dz)
    {
        for (int dx = -R; dx <= R; ++dx)
        {
            int tx = ptx + dx;
            int tz = ptz + dz;
            if (!isLavaTile(tx, tz))
                continue;

            // centro do tile no mundo
            float cx = offX + (tx + 0.5f) * tile;
            float cz = offZ + (tz + 0.5f) * tile;

            float ddx = cx - px;
            float ddz = cz - pz;
            float d2 = ddx * ddx + ddz * ddz;

            if (d2 < bestD2)
            {
                bestD2 = d2;
                bestX = cx;
                bestZ = cz;
                found = true;
            }
        }
    }

    if (!found)
        return false;

    outX = bestX;
    outZ = bestZ;
    outDist = std::sqrt(bestD2);
    return true;
}

// --- UPDATE ---
void gameUpdate(float dt)
{
    // LISTENER (OpenAL)
    if (gAudioOk)
    {
        Vec3 listenerPos = {camX, camY, camZ};

        float ry = yaw * 3.14159f / 180.0f;
        float rp = pitch * 3.14159f / 180.0f;

        Vec3 forward = {
            cosf(rp) * sinf(ry),
            sinf(rp),
            -cosf(rp) * cosf(ry)};

        Vec3 up = {0.0f, 1.0f, 0.0f};
        Vec3 vel = {0.0f, 0.0f, 0.0f};

        gAudio.setListener(listenerPos, vel, forward, up);
    }

    tempo += dt;

    atualizaMovimento();
    // passos (loop enquanto anda)
    audioUpdateStep(keyW || keyA || keyS || keyD);

    if (damageAlpha > 0.0f)
    {
        damageAlpha -= dt * 0.5f;
        if (damageAlpha < 0.0f)
            damageAlpha = 0.0f;
    }

    if (healthAlpha > 0.0f)
    {
        healthAlpha -= dt * 0.9f;
        if (healthAlpha < 0.0f)
            healthAlpha = 0.0f;
    }

    updateEntities(dt);
    updateWeaponAnim(dt);

    ensureEnemyExtraSourcesCreated();

    for (size_t i = 0; i < gLevel.enemies.size(); i++)
    {
        auto &en = gLevel.enemies[i];

        // ----- KILL: tocou no momento em que virou DEAD -----
        if (enemyPrevState[i] != STATE_DEAD && en.state == STATE_DEAD)
        {
            audioPlayKillAt(en.x, en.z);
        }
        enemyPrevState[i] = (int)en.state;

        // ----- SCREAM: aleatório, só se vivo -----
        if (en.state == STATE_DEAD)
            continue;
        if (srcEnemyScreams.empty() || srcEnemyScreams[i] == 0)
            continue;

        enemyScreamTimer[i] -= dt;
        if (enemyScreamTimer[i] <= 0.0f)
        {
            // Só toca se o player estiver relativamente perto
            float dx = en.x - camX;
            float dz = en.z - camZ;
            float dist = std::sqrt(dx * dx + dz * dz);

            if (dist <= AudioTuning::SCREAM_MAX_AUDIBLE_DIST)
            {
                // chance de gritar quando o timer vence
                float chance = (float)rand() / (float)RAND_MAX;
                if (chance <= AudioTuning::SCREAM_CHANCE)
                {
                    gAudio.setSourcePos(srcEnemyScreams[i], {en.x, 0.0f, en.z});
                    gAudio.stop(srcEnemyScreams[i]);
                    gAudio.play(srcEnemyScreams[i]);
                }
            }

            // sorteia próximo tempo (sempre)
            float r = (float)rand() / (float)RAND_MAX;
            enemyScreamTimer[i] =
                AudioTuning::ENEMY_SCREAM_MIN_INTERVAL +
                r * (AudioTuning::ENEMY_SCREAM_MAX_INTERVAL - AudioTuning::ENEMY_SCREAM_MIN_INTERVAL);
        }
    }

    // audio inimigos 3D (só toca perto)
    if (gAudioOk)
    {
        ensureEnemySourcesCreated();
        updateEnemyAudio(camX, camY, camZ, dt);
    }

    // ================= LAVA (proximidade) =================
    if (gAudioOk && srcLava != 0)
    {
        float lx, lz, dist;
        bool hasLava = nearestLava(camX, camZ, lx, lz, dist);

        // histerese (pra não ficar liga/desliga tremendo)
        const float LAVA_START = 8.0f; // começa a tocar quando ficar a <= 8m
        const float LAVA_STOP = 9.5f;  // para quando afastar pra >= 9.5m

        if (hasLava)
        {
            // posiciona o som na lava mais próxima
            gAudio.setSourcePos(srcLava, {lx, 0.0f, lz});

            if (!lavaPlaying && dist <= LAVA_START)
            {
                gAudio.play(srcLava);
                lavaPlaying = true;
            }
            else if (lavaPlaying && dist >= LAVA_STOP)
            {
                gAudio.stop(srcLava);
                lavaPlaying = false;
            }
        }
        else
        {
            // não tem lava perto (ou não existe) -> garante desligado
            if (lavaPlaying)
            {
                gAudio.stop(srcLava);
                lavaPlaying = false;
            }
        }
    }

    // ================= Respiração (low HP) =================
    if (srcBreath != 0)
    {
        // Só liga abaixo do threshold
        if (playerHealth > AudioTuning::LOW_HP_THRESHOLD)
        {
            gAudio.setSourceGain(srcBreath, 0.0f);
        }
        else
        {
            // 0..1 (quanto mais perto de 0 HP, mais alto)
            float t = (AudioTuning::LOW_HP_THRESHOLD - playerHealth) / (float)AudioTuning::LOW_HP_THRESHOLD;
            if (t < 0.0f)
                t = 0.0f;
            if (t > 1.0f)
                t = 1.0f;

            // volume cresce com t
            float gain = (AudioTuning::MASTER * AudioTuning::BREATH_GAIN) * (0.20f + t * 1.00f);
            gAudio.setSourceGain(srcBreath, gain);

            // garante que está tocando (loop)
            ALint st = 0;
            alGetSourcei(srcBreath, AL_SOURCE_STATE, &st);
            if (st != AL_PLAYING)
                gAudio.play(srcBreath);
        }
    }
}

// --- RENDER ---
void gameRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float radYaw = yaw * 3.14159265f / 180.0f;
    float radPitch = pitch * 3.14159265f / 180.0f;

    float dirX = cosf(radPitch) * sinf(radYaw);
    float dirY = sinf(radPitch);
    float dirZ = -cosf(radPitch) * cosf(radYaw);

    gluLookAt(
        camX, camY, camZ,
        camX + dirX, camY + dirY, camZ + dirZ,
        0.0f, 1.0f, 0.0f);

    setSunDirectionEachFrame();

    // céu
    drawSkydome(camX, camY, camZ);

    drawLevel(gLevel.map);

    // sprites
    drawEntities(gLevel.enemies, gLevel.items, camX, camZ);

    // HUD
    drawWeaponHUD();
    drawDamageOverlay();
    drawHealthOverlay();
    drawAmmoHUD();
    drawHealthHUD();

    glutSwapBuffers();
}