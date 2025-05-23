
#pragma once
#include <juce_animation/juce_animation.h>
#include <juce_audio_processors/juce_audio_processors.h>

class AnimationView : public juce::Component
{
public:
    AnimationView(juce::ValueAnimatorBuilder::EasingFn easingFunctionFactoryIn, juce::AudioProcessorValueTreeState& apvts) // when built, this will be juce::Easings::CreateEase()
        : easingFunctionFactory(std::move(easingFunctionFactoryIn)), orderAT(apvts, "order", order), overlapAT(apvts, "overlap", overlap), gainAT(apvts, "gain", gain), mixAT(apvts, "mix", mix)
    {
        jassert(easingFunctionFactory != nullptr);
        addAndMakeVisible(order);
        addAndMakeVisible(overlap);
        addAndMakeVisible(gain);
        addAndMakeVisible(mix);

        order.setName("Window Size");
        overlap.setName("Window Overlap");
        gain.setName("Gain");
        mix.setName("Mix");
    }

    void animateIn()
    {
        const auto valueChangedCallback = [&](float v)
        {
            animateFrame(v, 1.f, 1.f);
        };

        const auto animateIn = juce::ValueAnimatorBuilder{}.withEasing(easingFunctionFactory)
                                                           .withDurationMs(350)
                                                           .withValueChangedCallback(valueChangedCallback);

        const auto animateOut = animateIn.withValueChangedCallback([=](auto v)
                                                                   { valueChangedCallback(1.0f - v); });

        animator = std::make_unique<juce::Animator>(juce::AnimatorSetBuilder(animateIn.build())
                                                //   .followedBy(animateOut.build())
                                                  .build());

        updater.addAnimator(*animator);
        animator->start();
    }

    void animateOut()
    {
        const auto valueChangedCallback = [&](float v)
        {
            animateFrame(v, 1.f, 1.f);
        };

        const auto animateIn = juce::ValueAnimatorBuilder{}.withEasing(easingFunctionFactory).withDurationMs(500).withValueChangedCallback(valueChangedCallback);

        const auto animateOut = animateIn.withValueChangedCallback([=](auto v)
                                                                   { valueChangedCallback(1.0f - v); });

        animator = std::make_unique<juce::Animator>(juce::AnimatorSetBuilder(animateOut.build())
                                                        //   .followedBy(animateOut.build())
                                                        .build());

        updater.addAnimator(*animator);
        animator->start();
    }

    void animateFrame(float position, float size, float alpha)
    {
        const auto bounds = getLocalBounds();
        const auto center = bounds.getCentre();
        const auto yLimits = juce::makeAnimationLimits(bounds.getHeight(), -bounds.getHeight());
        setTransform(juce::AffineTransform{}.scaled(size, size, center.getX(), center.getY())
                                            .translated(0.0f, yLimits.lerp(position)));

        setAlpha(alpha);
    }

    void paint(juce::Graphics &g) override
    {
        g.setColour(juce::Colours::black);
        g.fillAll();
        g.setColour(juce::Colours::white);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds();
        auto topLeft = bounds.removeFromTop(bounds.getHeight() * .5);
        auto topRight = topLeft.removeFromRight(topLeft.getWidth()*.5);
        auto bottomLeft = bounds.removeFromLeft(bounds.getWidth() * .5);
        auto bottomRight = bounds;
        
        topLeft.reduce(20, 10);
        topRight.reduce(20, 10);
        bottomLeft.reduce(20, 10);
        bottomRight.reduce(20, 10);

        order.setBounds(topLeft);
        overlap.setBounds(topRight);
        gain.setBounds(bottomLeft);
        mix.setBounds(bottomRight);
    }

private:

    juce::ValueAnimatorBuilder::EasingFn easingFunctionFactory{};
    std::unique_ptr<juce::Animator> animator;
    juce::VBlankAnimatorUpdater updater{this};

    juce::Slider order{juce::Slider::SliderStyle::LinearBar, juce::Slider::TextEntryBoxPosition::NoTextBox},
                 overlap{juce::Slider::SliderStyle::LinearBar, juce::Slider::TextEntryBoxPosition::NoTextBox},
                 gain{juce::Slider::SliderStyle::LinearBar, juce::Slider::TextEntryBoxPosition::NoTextBox},
                 mix{juce::Slider::SliderStyle::LinearBar, juce::Slider::TextEntryBoxPosition::NoTextBox}; 

    juce::AudioProcessorValueTreeState::SliderAttachment orderAT, overlapAT, gainAT, mixAT;
};