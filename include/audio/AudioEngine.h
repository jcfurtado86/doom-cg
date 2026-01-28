#pragma once
#include <string>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

struct Vec3 {
    float x=0, y=0, z=0;
};

class AudioEngine {
public:
    bool init();
    void shutdown();

    // Carrega WAV PCM 16-bit (mono ou stereo). Retorna buffer OpenAL (0 se falhar)
    ALuint loadWav(const std::string& path);

    // Cria source e anexa buffer
    ALuint createSource(ALuint buffer, bool loop);

    // Playback
    void play(ALuint source);
    void stop(ALuint source);

    // Params
    void setSourcePos(ALuint source, const Vec3& p);
    void setSourceVel(ALuint source, const Vec3& v);
    void setSourceGain(ALuint source, float gain);
    void setSourcePitch(ALuint source, float pitch);

    void setListener(const Vec3& pos, const Vec3& vel, const Vec3& forward, const Vec3& up);

    // Distância (atenuação)
    void setDistanceModel();
    void setSourceDistance(ALuint source, float refDist, float rolloff, float maxDist);


private:
    ALCdevice*  mDevice  = nullptr;
    ALCcontext* mContext = nullptr;

    static bool readFile(const std::string& path, std::vector<unsigned char>& out);
};
