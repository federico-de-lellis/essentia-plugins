#pragma once

#include "Onset2Midi/PluginProcessor.h"
#include "BinaryData.h"

//==============================================================================
// Custom Look and Feel for modern styling (shared style with the other plugins).
class ModernLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ModernLookAndFeel();

    void drawLinearSlider(juce::Graphics&                 g,
                          int                             x,
                          int                             y,
                          int                             width,
                          int                             height,
                          float                           sliderPos,
                          float                           minSliderPos,
                          float                           maxSliderPos,
                          const juce::Slider::SliderStyle style,
                          juce::Slider&                   slider) override;

    void drawLinearSliderThumb(juce::Graphics&                 g,
                               int                             x,
                               int                             y,
                               int                             width,
                               int                             height,
                               float                           sliderPos,
                               float                           minSliderPos,
                               float                           maxSliderPos,
                               const juce::Slider::SliderStyle style,
                               juce::Slider&                   slider) override;
};

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void timerCallback() override;

    void setupSlider(juce::Slider&       slider,
                     juce::Label&        label,
                     juce::Label&        valueLabel,
                     const juce::String& labelText);
    void setupGroupLabel(juce::Label& label, const juce::String& text);
    juce::String formatValue(const juce::String& parameterID, float value) const;

    AudioPluginAudioProcessor& processorRef;

    // Header
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label poweredByLabel;

    // Group titles
    juce::Label detectionGroupLabel;
    juce::Label midiGroupLabel;

    // Detection controls
    juce::Label    methodLabel;
    juce::ComboBox methodCombo;

    juce::Slider sensitivitySlider;
    juce::Label  sensitivityLabel;
    juce::Label  sensitivityValueLabel;

    juce::Slider minInterOnsetSlider;
    juce::Label  minInterOnsetLabel;
    juce::Label  minInterOnsetValueLabel;

    // MIDI controls
    juce::Slider midiNoteSlider;
    juce::Label  midiNoteLabel;
    juce::Label  midiNoteValueLabel;

    juce::Slider midiChannelSlider;
    juce::Label  midiChannelLabel;
    juce::Label  midiChannelValueLabel;

    juce::Slider velocitySlider;
    juce::Label  velocityLabel;
    juce::Label  velocityValueLabel;

    juce::Slider noteHoldSlider;
    juce::Label  noteHoldLabel;
    juce::Label  noteHoldValueLabel;

    // Parameter attachments
    std::unique_ptr<ComboBoxAttachment> methodAttachment;
    std::unique_ptr<SliderAttachment>   sensitivityAttachment;
    std::unique_ptr<SliderAttachment>   minInterOnsetAttachment;
    std::unique_ptr<SliderAttachment>   midiNoteAttachment;
    std::unique_ptr<SliderAttachment>   midiChannelAttachment;
    std::unique_ptr<SliderAttachment>   velocityAttachment;
    std::unique_ptr<SliderAttachment>   noteHoldAttachment;

    // Onset activity LED
    int   lastOnsetCount{0};
    float ledLevel{0.0f};

    ModernLookAndFeel modernLF;

    juce::Image upfLogo;
    juce::Image essentiaLogo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
