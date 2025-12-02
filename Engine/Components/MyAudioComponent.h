#ifndef MYAUDIOCOMPONENT_H
#define MYAUDIOCOMPONENT_H
#include <string>
#include <common/miniaudio.h>

enum class AudioType {
    NONE,
    MUSIC,
    SFX,
    SPATIAL
};
struct MyAudioComponent {
    AudioType type = AudioType::NONE;
    std::string audioFilePath;
    ma_sound* sound = nullptr;
    float volume = 1.0f;
    bool loop = false;
    bool playOnStart = false;
    bool isPlaying = false;

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId, const std::string& gameFolder) {
        if(entityData["entities"][entityId]["audio"].contains("type")){
            std::string typeStr = entityData["entities"][entityId]["audio"]["type"].get<std::string>();
            // lowercase for robust comparison
            std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), [](unsigned char c){ return std::tolower(c); });
            if(typeStr == "music") type = AudioType::MUSIC;
            else if(typeStr == "sfx") type = AudioType::SFX;
            else if(typeStr == "spatial") type = AudioType::SPATIAL;
            else type = AudioType::NONE;
        }
        if(entityData["entities"][entityId]["audio"].contains("path")){
            audioFilePath = gameFolder + "/" + entityData["entities"][entityId]["audio"]["path"].get<std::string>();
        }
        if(entityData["entities"][entityId]["audio"].contains("volume")){
            volume = entityData["entities"][entityId]["audio"]["volume"].get<float>();
        }
        loop = entityData["entities"][entityId]["audio"].value("loop", false);
        playOnStart = entityData["entities"][entityId]["audio"].value("play_on_start", false);
        isPlaying = playOnStart;
    }
};
#endif // MYAUDIOCOMPONENT_H