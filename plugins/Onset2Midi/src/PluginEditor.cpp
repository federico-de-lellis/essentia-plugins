#include "Onset2Midi/PluginEditor.h"

//==============================================================================
// ModernLookAndFeel
//==============================================================================
ModernLookAndFeel::ModernLookAndFeel()
{
    setColour(juce::Slider::trackColourId, juce::Colour(0xff2d3748));
    setColour(juce::Slider::thumbColourId, juce::Colour(0xff4299e1));
    setColour(juce::Label::textColourId, juce::Colours::white);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2d3748));
    setColour(juce::ComboBox::textColourId, juce::Colours::white);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff4a5568));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff4299e1));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff2a3441));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff4299e1));
}

void ModernLookAndFeel::drawLinearSlider(juce::Graphics&                 g,
                                         int                             x,
                                         int                             y,
                                         int                             width,
                                         int                             height,
                                         float                           sliderPos,
                                         float                           minSliderPos,
                                         float                           maxSliderPos,
                                         const juce::Slider::SliderStyle style,
                                         juce::Slider&                   slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos, style, slider);

    const float trackY      = y + height * 0.5f - 3.0f;
    const float trackHeight = 6.0f;

    g.setColour(juce::Colour(0xff2d3748));
    g.fillRoundedRectangle((float) x, trackY, (float) width, trackHeight, 3.0f);

    const float filledWidth = sliderPos - (float) x;
    if (filledWidth > 0)
    {
        g.setColour(juce::Colour(0xff4299e1));
        g.fillRoundedRectangle((float) x, trackY, filledWidth, trackHeight, 3.0f);
    }

    drawLinearSliderThumb(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
}

void ModernLookAndFeel::drawLinearSliderThumb(juce::Graphics&                 g,
                                              int                             x,
                                              int                             y,
                                              int                             width,
                                              int                             height,
                                              float                           sliderPos,
                                              float                           minSliderPos,
                                              float                           maxSliderPos,
                                              const juce::Slider::SliderStyle style,
                                              juce::Slider&                   slider)
{
    juce::ignoreUnused(x, width, minSliderPos, maxSliderPos, style, slider);

    const float thumbSize = 20.0f;
    const float thumbX    = sliderPos - thumbSize * 0.5f;
    const float thumbY    = y + height * 0.5f - thumbSize * 0.5f;

    g.setColour(juce::Colour(0xff4299e1));
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

    g.setColour(juce::Colour(0xff3182ce));
    g.drawEllipse(thumbX, thumbY, thumbSize, thumbSize, 2.0f);
}

//==============================================================================
// Editor
//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p)
{
    setLookAndFeel(&modernLF);

    // Header
    titleLabel.setText("Onset2Midi", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4299e1));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("Real-time Onset Detection -> MIDI Trigger", juce::dontSendNotification);
    subtitleLabel.setFont(juce::FontOptions(14.0f));
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
    subtitleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(subtitleLabel);

    setupGroupLabel(detectionGroupLabel, "ONSET DETECTION");
    setupGroupLabel(midiGroupLabel, "MIDI OUTPUT");

    // Method selector
    methodLabel.setText("Method", juce::dontSendNotification);
    methodLabel.setFont(juce::FontOptions(13.0f));
    methodLabel.setColour(juce::Label::textColourId, juce::Colour(0xffccd7e5));
    methodLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(methodLabel);

    methodCombo.addItemList(AudioPluginAudioProcessor::getMethodChoices(), 1);
    addAndMakeVisible(methodCombo);

    // Sliders
    setupSlider(sensitivitySlider, sensitivityLabel, sensitivityValueLabel, "Sensitivity");
    setupSlider(minInterOnsetSlider, minInterOnsetLabel, minInterOnsetValueLabel, "Min Inter-Onset");
    setupSlider(midiNoteSlider, midiNoteLabel, midiNoteValueLabel, "MIDI Note");
    setupSlider(midiChannelSlider, midiChannelLabel, midiChannelValueLabel, "MIDI Channel");
    setupSlider(velocitySlider, velocityLabel, velocityValueLabel, "Velocity Sensitivity");
    setupSlider(noteHoldSlider, noteHoldLabel, noteHoldValueLabel, "Note Hold");

    // Attachments
    auto& apvts             = processorRef.getAPVTS();
    methodAttachment        = std::make_unique<ComboBoxAttachment>(apvts, "onsetMethod", methodCombo);
    sensitivityAttachment   = std::make_unique<SliderAttachment>(apvts, "sensitivity", sensitivitySlider);
    minInterOnsetAttachment = std::make_unique<SliderAttachment>(apvts, "minInterOnsetMs", minInterOnsetSlider);
    midiNoteAttachment      = std::make_unique<SliderAttachment>(apvts, "midiNote", midiNoteSlider);
    midiChannelAttachment   = std::make_unique<SliderAttachment>(apvts, "midiChannel", midiChannelSlider);
    velocityAttachment      = std::make_unique<SliderAttachment>(apvts, "velocitySensitivity", velocitySlider);
    noteHoldAttachment      = std::make_unique<SliderAttachment>(apvts, "noteHoldMs", noteHoldSlider);

    // Logos
    upfLogo      = juce::ImageCache::getFromMemory(BinaryData::upflogo_png, BinaryData::upflogo_pngSize);
    essentiaLogo = juce::ImageCache::getFromMemory(BinaryData::essentia_logo_png, BinaryData::essentia_logo_pngSize);

    poweredByLabel.setText("powered by", juce::dontSendNotification);
    poweredByLabel.setFont(juce::FontOptions(8.30437f));
    poweredByLabel.setColour(juce::Label::textColourId, juce::Colour(0xff718096));
    poweredByLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(poweredByLabel);

    setSize(600, 700);
    setResizable(false, false);

    startTimerHz(30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void AudioPluginAudioProcessorEditor::setupGroupLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colour(0xff4299e1));
    label.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(label);
}

void AudioPluginAudioProcessorEditor::setupSlider(juce::Slider&       slider,
                                                  juce::Label&        label,
                                                  juce::Label&        valueLabel,
                                                  const juce::String& labelText)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::FontOptions(13.0f));
    label.setColour(juce::Label::textColourId, juce::Colour(0xffccd7e5));
    label.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(label);

    valueLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    valueLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4299e1));
    valueLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(valueLabel);
}

juce::String AudioPluginAudioProcessorEditor::formatValue(const juce::String& parameterID, float value) const
{
    if (parameterID == "sensitivity")
        return juce::String(value, 2);
    if (parameterID == "velocitySensitivity")
        return juce::String(value, 2) + "x";
    if (parameterID == "minInterOnsetMs" || parameterID == "noteHoldMs")
        return juce::String(juce::roundToInt(value)) + " ms";
    if (parameterID == "midiNote")
        return juce::MidiMessage::getMidiNoteName(juce::roundToInt(value), true, true, 3) + " (" +
               juce::String(juce::roundToInt(value)) + ")";
    if (parameterID == "midiChannel")
        return juce::String(juce::roundToInt(value));
    return juce::String(value, 2);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    sensitivityValueLabel.setText(formatValue("sensitivity", (float) sensitivitySlider.getValue()),
                                  juce::dontSendNotification);
    minInterOnsetValueLabel.setText(formatValue("minInterOnsetMs", (float) minInterOnsetSlider.getValue()),
                                    juce::dontSendNotification);
    midiNoteValueLabel.setText(formatValue("midiNote", (float) midiNoteSlider.getValue()),
                               juce::dontSendNotification);
    midiChannelValueLabel.setText(formatValue("midiChannel", (float) midiChannelSlider.getValue()),
                                  juce::dontSendNotification);
    velocityValueLabel.setText(formatValue("velocitySensitivity", (float) velocitySlider.getValue()),
                               juce::dontSendNotification);
    noteHoldValueLabel.setText(formatValue("noteHoldMs", (float) noteHoldSlider.getValue()),
                               juce::dontSendNotification);

    // Onset activity LED flash on each new detected onset, then decay.
    const int c = processorRef.getOnsetCounter();
    if (c != lastOnsetCount)
    {
        lastOnsetCount = c;
        ledLevel       = 1.0f;
    }
    else
    {
        ledLevel *= 0.80f;
    }
    repaint();
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff1a1f2e));
    g.fillAll();

    const auto containerBounds = getLocalBounds().reduced(20).toFloat();
    g.setColour(juce::Colour(0xff2a3441));
    g.fillRoundedRectangle(containerBounds, 16.0f);
    g.setColour(juce::Colour(0xff3a4553));
    g.drawRoundedRectangle(containerBounds, 16.0f, 1.0f);

    // Onset activity LED (top-right of the container).
    const float ledSize = 16.0f;
    const float ledX    = containerBounds.getRight() - 40.0f;
    const float ledY    = containerBounds.getY() + 24.0f;

    g.setColour(juce::Colour(0xff1a202c));
    g.fillEllipse(ledX, ledY, ledSize, ledSize);
    g.setColour(juce::Colour(0xff48bb78).withAlpha(juce::jlimit(0.0f, 1.0f, ledLevel)));
    g.fillEllipse(ledX, ledY, ledSize, ledSize);
    g.setColour(juce::Colour(0xff4a5568));
    g.drawEllipse(ledX, ledY, ledSize, ledSize, 1.0f);

    g.setColour(juce::Colour(0xff718096));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("ONSET", (int) (ledX - 44.0f), (int) ledY, 40, (int) ledSize, juce::Justification::centredRight);

    // Footer logos.
    const float footerBaseline     = 40.0f;
    const float footerSideMargin   = 20.0f;
    const float standardLogoHeight = 34.0f;

    if (upfLogo.isValid())
    {
        const float w = upfLogo.getWidth() * (standardLogoHeight / upfLogo.getHeight());
        const int   lx = (int) (containerBounds.getX() + footerSideMargin);
        const int   ly = (int) (containerBounds.getBottom() - footerBaseline - standardLogoHeight);
        g.drawImage(upfLogo, lx, ly, (int) w, (int) standardLogoHeight, 0, 0, upfLogo.getWidth(),
                    upfLogo.getHeight());
    }

    if (essentiaLogo.isValid())
    {
        const float w  = essentiaLogo.getWidth() * (standardLogoHeight / essentiaLogo.getHeight());
        const int   lx = (int) (containerBounds.getRight() - footerSideMargin - w);
        const int   ly = (int) (containerBounds.getBottom() - footerBaseline - standardLogoHeight);
        g.drawImage(essentiaLogo, lx, ly, (int) w, (int) standardLogoHeight, 0, 0, essentiaLogo.getWidth(),
                    essentiaLogo.getHeight());
    }
}

void AudioPluginAudioProcessorEditor::resized()
{
    const int totalWidth   = getWidth();
    const int margin       = 60;
    const int labelHeight  = 20;
    const int sliderHeight = 25;
    const int valueWidth   = 110;
    const int controlSpace = 6;
    const int rowSpace     = 14;
    const int sectionSpace = 22;
    const int contentWidth = totalWidth - 2 * margin;

    auto layoutSlider = [&](int& y, juce::Label& label, juce::Label& valueLabel, juce::Slider& slider) {
        label.setBounds(margin, y, 200, labelHeight);
        valueLabel.setBounds(totalWidth - margin - valueWidth, y, valueWidth, labelHeight);
        slider.setBounds(margin, y + labelHeight + controlSpace, contentWidth, sliderHeight);
        y += labelHeight + controlSpace + sliderHeight + rowSpace;
    };

    titleLabel.setBounds(0, 25, totalWidth, 30);
    subtitleLabel.setBounds(0, 55, totalWidth, 20);

    int currentY = 100;

    // Section: Onset Detection
    detectionGroupLabel.setBounds(margin, currentY, 250, labelHeight);
    currentY += labelHeight + 10;

    methodLabel.setBounds(margin, currentY, 120, labelHeight);
    methodCombo.setBounds(margin + 120, currentY, contentWidth - 120, labelHeight + 6);
    currentY += labelHeight + 6 + rowSpace;

    layoutSlider(currentY, sensitivityLabel, sensitivityValueLabel, sensitivitySlider);
    layoutSlider(currentY, minInterOnsetLabel, minInterOnsetValueLabel, minInterOnsetSlider);

    currentY += sectionSpace - rowSpace;

    // Section: MIDI Output
    midiGroupLabel.setBounds(margin, currentY, 250, labelHeight);
    currentY += labelHeight + 10;

    layoutSlider(currentY, midiNoteLabel, midiNoteValueLabel, midiNoteSlider);
    layoutSlider(currentY, midiChannelLabel, midiChannelValueLabel, midiChannelSlider);
    layoutSlider(currentY, velocityLabel, velocityValueLabel, velocitySlider);
    layoutSlider(currentY, noteHoldLabel, noteHoldValueLabel, noteHoldSlider);

    // "powered by"  above the Essentia logo (logo top == height - 94,
    // check paint()) to aligned over the right-side logo.
    const int logoTop = getHeight() - 94;
    poweredByLabel.setBounds(totalWidth - 40 - 140, logoTop - 14, 140, 12);
}
