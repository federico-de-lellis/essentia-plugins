#include "Onset2Midi/PluginProcessor.h"
#include "Onset2Midi/PluginEditor.h"

#include <cmath>

//==============================================================================
namespace
{
// Number of recent onset-function values used for the adaptive threshold mean
// (~0.2 s at frameRate = sampleRate/hopSize ~= 86 fps).
constexpr int   kOdfHistoryLength = 20;
constexpr float kVelocityFloorDb  = -60.0f; // RMS below this maps to velocity ~1
} // namespace

//==============================================================================
juce::StringArray AudioPluginAudioProcessor::getMethodChoices()
{
    return {"HFC", "Complex", "Flux", "MelFlux", "RMS", "Complex Phase"};
}

const char* AudioPluginAudioProcessor::getEssentiaMethod(int choiceIndex)
{
    switch (choiceIndex)
    {
        case 0: return "hfc";
        case 1: return "complex";
        case 2: return "flux";
        case 3: return "melflux";
        case 4: return "rms";
        case 5: return "complex_phase";
        default: return "hfc";
    }
}

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         )
    , apvts(*this, nullptr, "Parameters", createParameters())
{}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    releaseResources();
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameters() const
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>("onsetMethod",
                                                            "Onset Method",
                                                            getMethodChoices(),
                                                            0));
    layout.add(std::make_unique<juce::AudioParameterFloat>("sensitivity",
                                                           "Sensitivity",
                                                           juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
                                                           0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("minInterOnsetMs",
                                                           "Min Inter-Onset",
                                                           juce::NormalisableRange<float>(5.0f, 500.0f, 1.0f),
                                                           50.0f,
                                                           "ms"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("noteHoldMs",
                                                           "Note Hold",
                                                           juce::NormalisableRange<float>(5.0f, 1000.0f, 1.0f),
                                                           50.0f,
                                                           "ms"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("velocitySensitivity",
                                                           "Velocity Sensitivity",
                                                           juce::NormalisableRange<float>(0.1f, 4.0f, 0.01f),
                                                           1.0f));
    layout.add(std::make_unique<juce::AudioParameterInt>("midiNote", "MIDI Note", 0, 127, 60));
    layout.add(std::make_unique<juce::AudioParameterInt>("midiChannel", "MIDI Channel", 1, 16, 10));

    return layout;
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
bool AudioPluginAudioProcessor::createAlgorithms()
{
    auto& factory = essentia::standard::AlgorithmFactory::instance();

    try
    {
        windowing      = factory.create("Windowing", "type", "hann", "size", frameSize);
        fft            = factory.create("FFT", "size", frameSize);
        cartToPolar    = factory.create("CartesianToPolar");
        onsetDetection = factory.create("OnsetDetection",
                                        "method",
                                        getEssentiaMethod(currentMethodIndex),
                                        "sampleRate",
                                        mSampleRate);

        // Wiring of the chain, buffers are reused for every frame.
        windowing->input("frame").set(frame);
        windowing->output("frame").set(windowedFrame);

        fft->input("frame").set(windowedFrame);
        fft->output("fft").set(fftOut);

        cartToPolar->input("complex").set(fftOut);
        cartToPolar->output("magnitude").set(magnitude);
        cartToPolar->output("phase").set(phase);

        onsetDetection->input("spectrum").set(magnitude);
        onsetDetection->input("phase").set(phase);
        onsetDetection->output("onsetDetection").set(odf);

        // Silent frame so any internal allocations happen here
        std::fill(frame.begin(), frame.end(), 0.0f);
        computeOnsetFunction();

        return true;
    }
    catch (const std::exception& e)
    {
        DBG("Failed to create Onset2Midi algorithms: " + juce::String(e.what()));
        destroyAlgorithms();
        return false;
    }
    catch (...)
    {
        DBG("Failed to create Onset2Midi algorithms: unknown exception");
        destroyAlgorithms();
        return false;
    }
}

void AudioPluginAudioProcessor::destroyAlgorithms()
{
    delete onsetDetection;
    onsetDetection = nullptr;
    delete cartToPolar;
    cartToPolar = nullptr;
    delete fft;
    fft = nullptr;
    delete windowing;
    windowing = nullptr;
}

float AudioPluginAudioProcessor::computeOnsetFunction()
{
    windowing->compute();
    fft->compute();
    cartToPolar->compute();
    onsetDetection->compute();
    return static_cast<float>(odf);
}

void AudioPluginAudioProcessor::updateOnsetMethodIfNeeded()
{
    const int idx = static_cast<int>(std::lround(*apvts.getRawParameterValue("onsetMethod")));
    if (idx == currentMethodIndex && onsetDetection != nullptr)
        return;

    currentMethodIndex = idx;

    // Only the OnsetDetection node depends on the method (a configure-time
    // parameter) so we can recreate and rewire just that node
    auto& factory = essentia::standard::AlgorithmFactory::instance();
    try
    {
        delete onsetDetection;
        onsetDetection = factory.create("OnsetDetection",
                                        "method",
                                        getEssentiaMethod(currentMethodIndex),
                                        "sampleRate",
                                        mSampleRate);
        onsetDetection->input("spectrum").set(magnitude);
        onsetDetection->input("phase").set(phase);
        onsetDetection->output("onsetDetection").set(odf);
    }
    catch (...)
    {
        DBG("Failed to switch Onset2Midi method");
        onsetDetection = nullptr;
    }

    // The detection function and adaptive threshold restart from scratch
    odfHistory.clear();
    prevOdf = 0.0f;
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    essentia::init();

    if (sampleRate <= 0)
        return;

    mSampleRate        = sampleRate;
    currentMethodIndex = static_cast<int>(std::lround(*apvts.getRawParameterValue("onsetMethod")));

    // Pre-allocate the buffers used on the audio thread
    frame.assign(frameSize, 0.0f);
    windowedFrame.assign(frameSize, 0.0f);
    magnitude.assign(frameSize / 2 + 1, 0.0f);
    phase.assign(frameSize / 2 + 1, 0.0f);
    accumBuffer.clear();
    accumBuffer.reserve(static_cast<std::size_t>(frameSize * 2));

    odfHistory.clear();
    prevOdf           = 0.0f;
    samplesSinceOnset = 0;
    pendingNoteOffs.clear();

    destroyAlgorithms();
    if (!createAlgorithms())
        DBG("Onset2Midi: algorithm initialization failed in prepareToPlay");
}

void AudioPluginAudioProcessor::releaseResources()
{
    destroyAlgorithms();
    essentia::shutdown();
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

//==============================================================================
void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    updateOnsetMethodIfNeeded();

    // MIDI-source effect so any data on the input MIDI bus is irrelevant.
    midiMessages.clear();

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    // --- Emit note-offs whose gate elapsed during (or before) this block ------
    for (auto it = pendingNoteOffs.begin(); it != pendingNoteOffs.end();)
    {
        if (it->samplesRemaining < numSamples)
        {
            const int offset = juce::jlimit(0, numSamples - 1, it->samplesRemaining);
            midiMessages.addEvent(juce::MidiMessage::noteOff(it->channel, it->note), offset);
            it = pendingNoteOffs.erase(it);
        }
        else
        {
            it->samplesRemaining -= numSamples;
            ++it;
        }
    }

    if (onsetDetection == nullptr)
        return;

    // --- Read parameters (cheap, once per block) ------------------------------
    const float sensitivity      = *apvts.getRawParameterValue("sensitivity");
    const float minInterOnsetMs  = *apvts.getRawParameterValue("minInterOnsetMs");
    const float noteHoldMs       = *apvts.getRawParameterValue("noteHoldMs");
    const float velocitySens     = *apvts.getRawParameterValue("velocitySensitivity");
    const int   midiNote         = static_cast<int>(std::lround(*apvts.getRawParameterValue("midiNote")));
    const int   midiChannel      = static_cast<int>(std::lround(*apvts.getRawParameterValue("midiChannel")));

    const int minInterOnsetSamples = static_cast<int>(minInterOnsetMs * 0.001 * mSampleRate);
    const int noteHoldSamples      = juce::jmax(1, static_cast<int>(noteHoldMs * 0.001 * mSampleRate));

    // Higher sensitivity -> lower threshold multiplier -> more triggers.
    const float lambda = juce::jmap(sensitivity, 0.0f, 1.0f, 3.0f, 1.0f);

    // --- Mix to mono and accumulate -------------------------------------------
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += buffer.getReadPointer(ch)[i];
        accumBuffer.push_back(mono / static_cast<float>(numChannels));
    }

    // Index in accumBuffer where this block's audio begins (>= 0).
    const int blockStartInAccum = static_cast<int>(accumBuffer.size()) - numSamples;

    // --- Frame loop: overlapping frames, hopSize apart ------------------------
    std::size_t readPos = 0;
    while (accumBuffer.size() - readPos >= static_cast<std::size_t>(frameSize))
    {
        std::copy(accumBuffer.begin() + static_cast<std::ptrdiff_t>(readPos),
                  accumBuffer.begin() + static_cast<std::ptrdiff_t>(readPos) + frameSize,
                  frame.begin());

        float odfValue = 0.0f;
        try
        {
            odfValue = computeOnsetFunction();
        }
        catch (...)
        {
            odfValue = 0.0f;
        }

        // Adaptive threshold from the running mean of recent odf values.
        odfHistory.push_back(odfValue);
        if (static_cast<int>(odfHistory.size()) > kOdfHistoryLength)
            odfHistory.pop_front();

        float mean = 0.0f;
        for (float v : odfHistory)
            mean += v;
        mean /= static_cast<float>(odfHistory.size());

        const float threshold = mean * lambda + 1.0e-9f;

        samplesSinceOnset += hopSize;

        const bool crossing   = (odfValue > threshold) && (prevOdf <= threshold);
        const bool debounceOk = samplesSinceOnset >= minInterOnsetSamples;

        if (crossing && debounceOk)
        {
            samplesSinceOnset = 0;

            // Velocity from the frame's RMS level.
            double energy = 0.0;
            for (int s = 0; s < frameSize; ++s)
                energy += static_cast<double>(frame[(std::size_t) s]) * frame[(std::size_t) s];
            const float rms = std::sqrt(static_cast<float>(energy / frameSize));
            const float db  = 20.0f * std::log10(rms + 1.0e-9f);

            float velNorm = (db - kVelocityFloorDb) / (0.0f - kVelocityFloorDb);
            velNorm       = juce::jlimit(0.0f, 1.0f, velNorm) * velocitySens;
            const int velocity =
                juce::jlimit(1, 127, static_cast<int>(std::lround(velNorm * 127.0f)));

            // Block-relative sample offset (approximate: where this frame ends).
            const int frameEndInAccum = static_cast<int>(readPos) + frameSize;
            const int offset = juce::jlimit(0, numSamples - 1, frameEndInAccum - blockStartInAccum);

            // Clean retrigger: if the same note is still gated, close it first.
            for (auto it = pendingNoteOffs.begin(); it != pendingNoteOffs.end();)
            {
                if (it->note == midiNote && it->channel == midiChannel)
                {
                    midiMessages.addEvent(juce::MidiMessage::noteOff(it->channel, it->note), offset);
                    it = pendingNoteOffs.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            midiMessages.addEvent(
                juce::MidiMessage::noteOn(midiChannel, midiNote, (juce::uint8) velocity), offset);
            pendingNoteOffs.push_back({midiNote, midiChannel, offset + noteHoldSamples});

            onsetCounter.fetch_add(1);
        }

        prevOdf = odfValue;
        readPos += static_cast<std::size_t>(hopSize);
    }

    // Drop consumed samples and keep the overlap tail for the next block.
    if (readPos > 0)
        accumBuffer.erase(accumBuffer.begin(), accumBuffer.begin() + static_cast<std::ptrdiff_t>(readPos));
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
