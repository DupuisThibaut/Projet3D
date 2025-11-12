#ifndef AUDIOSYSTEM_H
#define AUDIOSYSTEM_H

#define MINIAUDIO_IMPLEMENTATION

#include <unordered_map>
#include <string>
#include <cstdint>
#include <iostream>

#include "../Components/AudioComponent.h"
#include "../Components/TransformComponent.h"
#include "../common/miniaudio.h"

class AudioSystem {
public:
    EntityManager* entityManager;

    AudioSystem(EntityManager* em) : entityManager(em) {
        ma_engine_config engineConfig = ma_engine_config_init();
        engineConfig.gainSmoothTimeInFrames = 256;
        ma_result r = ma_engine_init(&engineConfig, &engine);
        if (r != MA_SUCCESS) {
            std::cerr << "AudioSystem: ma_engine_init failed (" << r << ")\n";
            initialized = false;
        } else {
            std::cout << "AudioSystem: engine initialized successfully\n";
            ma_engine_set_volume(&engine, 1.0f);
            initialized = true;
        }
    }

    ~AudioSystem() {
        for (auto &kv : audioComponents) {
            AudioComponent &comp = kv.second;
            if (comp.sound) {
                ma_sound_stop(comp.sound);
                ma_sound_uninit(comp.sound);
                delete comp.sound;
                comp.sound = nullptr;
            }
        }
        if (initialized) ma_engine_uninit(&engine);
    }

    void addAudio(uint32_t id, AudioComponent& audio) {
        if (!initialized) return;
        AudioComponent comp = audio;
        comp.sound = new ma_sound();
        unsigned int flags = MA_SOUND_FLAG_DECODE;
        if (comp.type == AudioType::MUSIC) flags = MA_SOUND_FLAG_STREAM;
        std::cout << "AudioSystem: loading '" << comp.audioFilePath << "' for entity " << id << "...\n";
        ma_result r = ma_sound_init_from_file(&engine, comp.audioFilePath.c_str(), flags, NULL, NULL, comp.sound);
        if (r != MA_SUCCESS) {
            std::cerr << "AudioSystem: failed to load '" << comp.audioFilePath << "' (error " << r << ")\n";
            delete comp.sound;
            comp.sound = nullptr;
            return;
        }
        ma_sound_set_looping(comp.sound, comp.loop ? MA_TRUE : MA_FALSE);
        float safeVolume = std::min(1.0f, std::max(0.0f, comp.volume)) * 0.7f;
        ma_sound_set_volume(comp.sound, safeVolume);
        if (comp.type == AudioType::MUSIC || comp.type == AudioType::SFX) {
            ma_sound_set_spatialization_enabled(comp.sound, MA_FALSE);
        } else if (comp.type == AudioType::SPATIAL) {
            ma_sound_set_spatialization_enabled(comp.sound, MA_TRUE);
            ma_sound_set_min_distance(comp.sound, 0.01f);
            ma_sound_set_max_distance(comp.sound, 5.0f);
            ma_sound_set_rolloff(comp.sound, 1.0f);
            ma_sound_set_attenuation_model(comp.sound, ma_attenuation_model_inverse);
        }
        if (comp.playOnStart) {
            ma_sound_start(comp.sound);
            comp.isPlaying = true;
            std::cout << "AudioSystem: started playback for entity " << id << "\n";
        }
        audioComponents[id] = std::move(comp);
    }

    void play(uint32_t id) {
        auto it = audioComponents.find(id);
        if (it == audioComponents.end()) return;
        if (it->second.sound) {
            ma_sound_start(it->second.sound);
            it->second.isPlaying = true;
        }
    }

    void stop(uint32_t id) {
        auto it = audioComponents.find(id);
        if (it == audioComponents.end()) return;
        if (it->second.sound) {
            ma_sound_stop(it->second.sound);
            it->second.isPlaying = false;
        }
    }

    void update() {
        if (!initialized) return;
        for (auto &kv : audioComponents) {
            uint32_t id = kv.first;
            AudioComponent &comp = kv.second;
            if (comp.type == AudioType::SPATIAL && comp.sound) {
                auto it = entityManager->GetComponents<TransformComponent>().find(id);
                if (it != entityManager->GetComponents<TransformComponent>().end()) {
                    const TransformComponent &t = it->second;
                    ma_sound_set_position(comp.sound, t.position.x, t.position.y, t.position.z);
                }
            }
            if (comp.sound) {
                ma_bool32 playing = ma_sound_is_playing(comp.sound);
                comp.isPlaying = (playing != MA_FALSE);
            }
        }

    }


    void updateListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) {
        if (!initialized) return;
        #if defined(MA_ENGINE_API_VERSION) || 1
         ma_engine_listener_set_position(&engine, 0, position.x, position.y, position.z);
        ma_engine_listener_set_direction(&engine, 0, forward.x, forward.y, forward.z);
        ma_engine_listener_set_world_up(&engine, 0, up.x, up.y, up.z);
        #else
        (void)position; (void)forward; (void)up;
        #endif
    }

    void clear(){
        for(auto& [id, comp] : audioComponents){
            ma_sound_uninit(comp.sound);
            delete comp.sound;
        }
        audioComponents.clear();
    }

private:
    ma_engine engine;
    bool initialized = false;
    std::unordered_map<uint32_t, AudioComponent> audioComponents;
};

#endif // AUDIOSYSTEM_H