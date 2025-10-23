/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#define XHL_MATHS_IMPL
#define XHL_TIME_IMPL

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp.h"

#include <xhl/time.h>

//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         )
    ,
#endif
    APVTS(
        *this,
        nullptr,
        "state",
        {std::make_unique<AudioParameterFloat>(
             ParameterID{"cutoff", 1},
             "Cutoff",
             NormalisableRange<float>(0.0f, 1.0f),
             0.5f),
         std::make_unique<AudioParameterFloat>(
             ParameterID{"resonance", 1},
             "Resonance",
             NormalisableRange<float>(0.0f, 1.0f),
             0.5f)})
{
    xtime_init();

    this->param_cutoff    = APVTS.getRawParameterValue("cutoff");
    this->param_resonance = APVTS.getRawParameterValue("resonance");

    memset(&this->time, 0, sizeof(this->time));

    const int numVoices = 8;
    // Add some voices...
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice(new SawWaveVoice(*this));

    // ..and give the synth a sound to play
    synth.addSound(new SineWaveSound());
}

NewProjectAudioProcessor::~NewProjectAudioProcessor() {}

//==============================================================================
const juce::String NewProjectAudioProcessor::getName() const { return JucePlugin_Name; }

bool NewProjectAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool NewProjectAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool NewProjectAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double NewProjectAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int NewProjectAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int NewProjectAudioProcessor::getCurrentProgram() { return 0; }

void NewProjectAudioProcessor::setCurrentProgram(int index) {}

const juce::String NewProjectAudioProcessor::getProgramName(int index) { return {}; }

void NewProjectAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

void NewProjectAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    synth.setCurrentPlaybackSampleRate(sampleRate);

    memset(&this->time, 0, sizeof(this->time));
}

void NewProjectAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto                    totalNumInputChannels  = getTotalNumInputChannels();
    auto                    totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    int numSamples = buffer.getNumSamples();
    synth.renderNextBlock(buffer, midiMessages, 0, numSamples);

    xassert(time.graph_write_idx < juce::numElementsInArray(time.delta_history));
    uint64_t time_now   = xtime_now_ns();
    uint64_t time_delta = time_now - time.last_process_call;
    // after program initialises or does ::prepareToPlay, 'time_last_process_call' will be set to zero
    // To avoid getting some really large delta, we just say the delta is zero
    if (time.last_process_call == 0)
        time_delta = 0;
    uint64_t prev_delta = time.delta_history[time.graph_write_idx];

    time.graph_running_sum = time.graph_running_sum + time_delta - prev_delta;
    time.last_process_call = time_now;

    time.delta_history[time.graph_write_idx] = time_delta;
    if (++time.graph_write_idx >= juce::numElementsInArray(time.delta_history))
        time.graph_write_idx = 0;
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor(*this);
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void NewProjectAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NewProjectAudioProcessor(); }

//==============================================================================

Coeffs make_filter_coeffs(float cutoff, float resonance, double sample_rate_inv)
{
    cutoff    = xm_clampf(cutoff, 0, 1);
    resonance = xm_clampf(resonance, 0, 1);

    float cutoff_Hz = xm_fast_denomalise_Hz(cutoff);
    float lp_Q      = xm_lerpf(resonance, 0.2, 2);

    return filter_LP(cutoff_Hz, lp_Q, sample_rate_inv);
}

void SawWaveVoice::startNote(
    int   midiNoteNumber,
    float velocity,
    SynthesiserSound* /*sound*/,
    int /*currentPitchWheelPosition*/)
{
    memset(&state, 0, sizeof(state));

    state.level = velocity * 0.25; // -12dB

    state.sample_rate_inv = 1.0 / getSampleRate();

    float Hz        = MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    state.phase_inc = Hz * state.sample_rate_inv;

    adsr_set_params(&state.adsr, 0.05, 1, 1, 0.05, getSampleRate());
    adsr_set_stage(&state.adsr, ADSR_ATTACK);

    smoothvalue_reset(&state.cutoff, processor.param_cutoff->load());
    smoothvalue_reset(&state.resonance, processor.param_resonance->load());

    state.coeffs = make_filter_coeffs(state.cutoff.current, state.resonance.current, state.sample_rate_inv);
}

void SawWaveVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr_set_stage(&state.adsr, ADSR_RELEASE);
    }
    else
    {
        adsr_set_stage(&state.adsr, ADSR_IDLE);

        clearCurrentNote();
        state.phase_inc = 0.0;
    }
}

float voice_tick(SawWaveVoice& voice)
{
    // Saw wave
    // TODO: make band limited wavetable to prevent aliasing
    float osc_sample = juce::jmap<float>(voice.state.phase, -1.0f, 1.0f);

    voice.state.phase += voice.state.phase_inc;
    voice.state.phase -= (int)voice.state.phase;

    // Filter:
    bool is_smoothing = !!voice.state.cutoff.remaining || !!voice.state.resonance.remaining;

    if (is_smoothing)
    {
        smoothvalue_tick(&voice.state.cutoff);
        smoothvalue_tick(&voice.state.resonance);
        voice.state.coeffs = voice.state.coeffs =
            make_filter_coeffs(voice.state.cutoff.current, voice.state.resonance.current, voice.state.sample_rate_inv);
    }

    float filter_sample = filter_process(osc_sample, &voice.state.coeffs, voice.state.filter_state);

    float env_amt = adsr_tick(&voice.state.adsr);

    filter_sample *= env_amt;

    return filter_sample;
}

void SawWaveVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    float  cutoff            = processor.param_cutoff->load();
    float  res               = processor.param_resonance->load();
    double smothing_time_sec = 0.05; // 50ms
    smoothvalue_set_target(&state.cutoff, cutoff, smothing_time_sec * getSampleRate());
    smoothvalue_set_target(&state.resonance, res, smothing_time_sec * getSampleRate());

    if (state.adsr.current_stage != ADSR_IDLE)
    {
        while (--numSamples >= 0)
        {
            float currentSample = (float)(voice_tick(*this) * state.level);

            for (int i = outputBuffer.getNumChannels(); --i >= 0;)
                outputBuffer.addSample(i, startSample, currentSample);

            ++startSample;

            if (state.adsr.current_stage == ADSR_IDLE)
            {
                // tells the synth that this voice has stopped
                clearCurrentNote();
                break;
            }
        }
    }
}