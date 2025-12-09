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

    void renderEditor() {
        if(ImGui::CollapsingHeader("Audio Component", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
            ImGui::Checkbox("Loop", &loop);
            ImGui::Checkbox("Play on Start", &playOnStart);
            ImGui::Checkbox("Is Playing", &isPlaying);

            // Zone de drag & drop
            ImGui::Text("Audio File:");
            ImGui::BeginChild("AudioDropZone", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::TextWrapped("%s", audioFilePath.empty() ? "Drag & drop an audio file here" : audioFilePath.c_str());
            ImGui::EndChild();

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                    const char* droppedPath = (const char*)payload->Data;
                    std::string ext = droppedPath;
                    ext = ext.substr(ext.find_last_of('.') + 1);
                    // Vérifie si c'est un fichier audio
                    if (ext == "wav" || ext == "mp3" || ext == "ogg" || ext == "WAV" || ext == "MP3" || ext == "OGG") {
                        audioFilePath = std::string(droppedPath);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            char audioPathBuffer[256];
            strncpy(audioPathBuffer, audioFilePath.c_str(), sizeof(audioPathBuffer));
            audioPathBuffer[sizeof(audioPathBuffer) - 1] = '\0';
            ImGui::InputText("Audio File Path", audioPathBuffer, sizeof(audioPathBuffer));
            audioFilePath = std::string(audioPathBuffer);

            const char* audioTypes[] = { "NONE", "MUSIC", "SFX", "SPATIAL" };
            int currentType = static_cast<int>(type);
            if (ImGui::Combo("Audio Type", &currentType, audioTypes, IM_ARRAYSIZE(audioTypes))) {
                type = static_cast<AudioType>(currentType);
            }
        }
    }
};
#endif // MYAUDIOCOMPONENT_H