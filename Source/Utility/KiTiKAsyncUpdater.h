#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

struct KiTiKAsyncUpdater : public juce::AsyncUpdater
{
    KiTiKAsyncUpdater() {};

    void setCallback(std::function<void(void)> callback)
    {
        callback_ = std::move(callback);
    }

    void handleAsyncUpdate() override
    {
        callback_();
    }

private:
    std::function<void(void)> callback_;
};