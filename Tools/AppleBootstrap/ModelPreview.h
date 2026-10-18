#pragma once
#include <cstdint>
#include <string>
class CM2Scene;
class CM2Model;
class CCharacterComponent;

// Original Model2 scene owned by the native preview's Event/Services context.
// All game/Gx work runs on that context's thread; archive reads use its worker.
class ModelPreview {
public:
    ModelPreview(const char* name, uint32_t creatureDisplay = 0, const char* character = nullptr);
    ~ModelPreview();
    ModelPreview(const ModelPreview&) = delete;
    ModelPreview& operator=(const ModelPreview&) = delete;
    bool Draw(float elapsedSeconds, uint32_t width, uint32_t height);
private:
    void Shutdown();
    std::string m_name;
    CM2Scene* m_scene = nullptr;
    CM2Model* m_model = nullptr;
    CCharacterComponent* m_character = nullptr;
    bool m_componentsInitialized = false;
    bool m_initialized = false, m_effectsInitialized = false, m_blobLoaded = false;
    double m_fractionalMilliseconds = 0;
    float m_rotation = 0;
};
