#include "audio/AudioEngine.h"
#include <cstdio>
#include <cstring>
#include <fstream>

static bool read_u16(const unsigned char* p, size_t sz, size_t& off, unsigned short& out) {
    if (off + 2 > sz) return false;
    out = (unsigned short)(p[off] | (p[off+1] << 8));
    off += 2;
    return true;
}
static bool read_u32(const unsigned char* p, size_t sz, size_t& off, unsigned int& out) {
    if (off + 4 > sz) return false;
    out = (unsigned int)(p[off] | (p[off+1] << 8) | (p[off+2] << 16) | (p[off+3] << 24));
    off += 4;
    return true;
}

bool AudioEngine::readFile(const std::string& path, std::vector<unsigned char>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    size_t sz = (size_t)f.tellg();
    f.seekg(0, std::ios::beg);
    out.resize(sz);
    f.read((char*)out.data(), (std::streamsize)sz);
    return true;
}

bool AudioEngine::init() {
    mDevice = alcOpenDevice(nullptr);
    if (!mDevice) return false;

    mContext = alcCreateContext(mDevice, nullptr);
    if (!mContext) return false;

    alcMakeContextCurrent(mContext);

    // 🔥 MODELO DE DISTÂNCIA CORRETO
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

    // Listener padrão
    alListener3f(AL_POSITION, 0, 0, 0);
    alListener3f(AL_VELOCITY, 0, 0, 0);

    float ori[] = { 0,0,-1, 0,1,0 };
    alListenerfv(AL_ORIENTATION, ori);

    return true;
}


void AudioEngine::shutdown() {
    if (mContext) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(mContext);
        mContext = nullptr;
    }
    if (mDevice) {
        alcCloseDevice(mDevice);
        mDevice = nullptr;
    }
}

ALuint AudioEngine::loadWav(const std::string& path) {
    std::vector<unsigned char> data;
    if (!readFile(path, data)) {
        std::printf("[Audio] Falha ao abrir WAV: %s\n", path.c_str());
        return 0;
    }
    if (data.size() < 44) {
        std::printf("[Audio] WAV pequeno demais: %s\n", path.c_str());
        return 0;
    }

    const unsigned char* p = data.data();
    size_t sz = data.size();
    size_t off = 0;

    // RIFF
    if (std::memcmp(p, "RIFF", 4) != 0) return 0;
    off += 4;
    unsigned int riffSize=0; if (!read_u32(p, sz, off, riffSize)) return 0;
    if (std::memcmp(p+off, "WAVE", 4) != 0) return 0;
    off += 4;

    // Procurar chunks fmt / data
    unsigned short audioFormat=0, numChannels=0, bitsPerSample=0;
    unsigned int sampleRate=0;
    const unsigned char* pcmPtr=nullptr;
    unsigned int pcmSize=0;

    while (off + 8 <= sz) {
        char id[5] = {0,0,0,0,0};
        std::memcpy(id, p+off, 4);
        off += 4;
        unsigned int chunkSize=0; if (!read_u32(p, sz, off, chunkSize)) return 0;

        if (off + chunkSize > sz) break;

        if (std::memcmp(id, "fmt ", 4) == 0) {
            size_t fmtOff = off;
            unsigned short tmp16=0; unsigned int tmp32=0;

            // fmt
            read_u16(p, sz, fmtOff, audioFormat);
            read_u16(p, sz, fmtOff, numChannels);
            read_u32(p, sz, fmtOff, sampleRate);
            read_u32(p, sz, fmtOff, tmp32); // byteRate ignore
            read_u16(p, sz, fmtOff, tmp16); // blockAlign ignore
            read_u16(p, sz, fmtOff, bitsPerSample);

        } else if (std::memcmp(id, "data", 4) == 0) {
            pcmPtr  = p + off;
            pcmSize = chunkSize;
        }

        off += chunkSize;
        // padding
        if (chunkSize & 1) off++;
    }

    if (!pcmPtr || pcmSize == 0) {
        std::printf("[Audio] WAV sem chunk data: %s\n", path.c_str());
        return 0;
    }
    if (audioFormat != 1) {
        std::printf("[Audio] WAV nao PCM (format=%u): %s\n", (unsigned)audioFormat, path.c_str());
        return 0;
    }
    if (bitsPerSample != 16) {
        std::printf("[Audio] WAV precisa ser 16-bit (bits=%u): %s\n", (unsigned)bitsPerSample, path.c_str());
        return 0;
    }

    ALenum format = 0;
    if (numChannels == 1) format = AL_FORMAT_MONO16;
    else if (numChannels == 2) format = AL_FORMAT_STEREO16;
    else {
        std::printf("[Audio] WAV canais inesperados (%u): %s\n", (unsigned)numChannels, path.c_str());
        return 0;
    }

    ALuint buffer=0;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, pcmPtr, (ALsizei)pcmSize, (ALsizei)sampleRate);

    ALenum err = alGetError();
    if (err != AL_NO_ERROR) {
        std::printf("[Audio] alBufferData erro (%d) em %s\n", (int)err, path.c_str());
        alDeleteBuffers(1, &buffer);
        return 0;
    }

    // IMPORTANTE: som posicional no OpenAL funciona “de verdade” melhor com MONO.
    // Se for STEREO, geralmente ele fica “colado”/não atenua como esperado.
    if (numChannels == 2) {
        std::printf("[Audio] Aviso: %s eh STEREO. Para som 3D/atenuacao, converta para MONO.\n", path.c_str());
    }

    return buffer;
}

ALuint AudioEngine::createSource(ALuint buffer, bool loop) {
    ALuint s=0;
    alGenSources(1, &s);
    alSourcei(s, AL_BUFFER, (ALint)buffer);
    alSourcei(s, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);

    // defaults bons
    alSourcef(s, AL_GAIN, 1.0f);
    alSourcef(s, AL_PITCH, 1.0f);
    alSource3f(s, AL_POSITION, 0,0,0);
    alSource3f(s, AL_VELOCITY, 0,0,0);

    return s;
}

void AudioEngine::play(ALuint source) { alSourcePlay(source); }
void AudioEngine::stop(ALuint source) { alSourceStop(source); }

void AudioEngine::setSourcePos(ALuint source, const Vec3& p) { alSource3f(source, AL_POSITION, p.x,p.y,p.z); }
void AudioEngine::setSourceVel(ALuint source, const Vec3& v) { alSource3f(source, AL_VELOCITY, v.x,v.y,v.z); }
void AudioEngine::setSourceGain(ALuint source, float gain)    { alSourcef(source, AL_GAIN, gain); }
void AudioEngine::setSourcePitch(ALuint source, float pitch)  { alSourcef(source, AL_PITCH, pitch); }

void AudioEngine::setListener(const Vec3& pos, const Vec3& vel, const Vec3& forward, const Vec3& up) {
    alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);
    float ori[6] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
    alListenerfv(AL_ORIENTATION, ori);
}

void AudioEngine::setDistanceModel() {
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
}

void AudioEngine::setSourceDistance(ALuint src, float refDist, float rolloff, float maxDist) {
    alSourcef(src, AL_REFERENCE_DISTANCE, refDist);
    alSourcef(src, AL_ROLLOFF_FACTOR, rolloff);
    alSourcef(src, AL_MAX_DISTANCE, maxDist);
}

