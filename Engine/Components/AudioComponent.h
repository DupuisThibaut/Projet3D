#ifndef AUDIOCOMPONENT_H
#define AUDIOCOMPONENT_H
#include <string>
#include <common/miniaudio.h>

enum class AudioType {
    NONE,
    MUSIC,
    SFX,
    SPATIAL
};
struct AudioComponent {
    AudioType type = AudioType::NONE;
    std::string audioFilePath;
    ma_sound* sound = nullptr;
    float volume = 1.0f;
    bool loop = false;
    bool playOnStart = false;
    bool isPlaying = false;
};
#endif // AUDIOCOMPONENT_H