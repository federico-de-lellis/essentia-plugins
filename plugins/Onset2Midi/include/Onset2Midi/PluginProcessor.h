#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <essentia/algorithmfactory.h>
#include <essentia/essentiamath.h>

#include <complex>
#include <deque>
#include <vector>

//==============================================================================
// Onset2Midi
//
// Detects percussive onsets in the incoming audio with an Essentia spectral
// onset-detection function and emits a fixed MIDI note (velocity derived from
// hit strength) to trigger a sampler on the next track.
//
// DSP chain (per 1024-sample frame, hop 512):
//   mono frame -> Windowing(hann) -> FFT -> CartesianToPolar -> {magnitude, phase}
//     -> OnsetDetection(method) -> odf scalar -> adaptive peak-pick -> MIDI
//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool                        hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool   acceptsMidi() const override;
    bool   producesMidi() const override;
    bool   isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int                getNumPrograms() override;
    int                getCurrentProgram() override;
    void               setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void               changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Monotonic counter bumped on every detected onset; the editor polls it to
    // flash the activity LED. Read-only from the UI thread.
    int getOnsetCounter() const { return onsetCounter.load(); }

    // List of onset-detection method labels, kept in sync with the APVTS choice
    // parameter and the Essentia method strings.
    static juce::StringArray getMethodChoices();
    static const char*       getEssentiaMethod(int choiceIndex);

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState                  apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters() const;

    bool createAlgorithms();
    void destroyAlgorithms();
    void updateOnsetMethodIfNeeded();

    // Process one accumulated frame through the Essentia chain, returning the
    // onset-detection function value for that frame.
    float computeOnsetFunction();

    //==============================================================================
    // Essentia algorithms (the onset chain).
    essentia::standard::Algorithm* windowing{nullptr};
    essentia::standard::Algorithm* fft{nullptr};
    essentia::standard::Algorithm* cartToPolar{nullptr};
    essentia::standard::Algorithm* onsetDetection{nullptr};

    // Wiring buffers (pre-allocated, reused every frame on the audio thread).
    std::vector<essentia::Real>               frame;         // raw mono frame (frameSize)
    std::vector<essentia::Real>               windowedFrame; // windowed frame
    std::vector<std::complex<essentia::Real>> fftOut;        // complex spectrum
    std::vector<essentia::Real>               magnitude;     // |spectrum|
    std::vector<essentia::Real>               phase;         // phase (radians)
    essentia::Real                            odf{0.f};      // onset-detection value

    // Mono accumulation buffer fed by processBlock, drained in frame/hop steps.
    std::vector<float> accumBuffer;

    // Peak-picking state.
    std::deque<float> odfHistory;       // recent odf values for adaptive threshold
    float             prevOdf{0.f};     // previous frame's odf (local-max test)
    int               samplesSinceOnset{0}; // debounce counter (in samples)

    // Pending note-offs scheduled after a gate; counted down across blocks.
    struct PendingNoteOff
    {
        int note;
        int channel;
        int samplesRemaining;
    };
    std::vector<PendingNoteOff> pendingNoteOffs;

    // Engine configuration.
    static constexpr int frameSize = 1024;
    static constexpr int hopSize   = 512;
    double               mSampleRate{44100.0};
    int                  currentMethodIndex{0};

    std::atomic<int> onsetCounter{0};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
