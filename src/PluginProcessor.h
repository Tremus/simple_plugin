/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <xhl/debug.h>
#include <xhl/maths.h>

struct NewProjectAudioProcessor;

typedef enum MyADSRStage
{
    ADSR_IDLE,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE,
    ADSR_NUM_STAGES,
} MyADSRStage;

#define ADSR_MIN_LEVEL 0.0001

struct MyADSR
{
    MyADSRStage current_stage;
    float       current_level;
    float       multiplier;
    uint32_t    current_idx;
    uint32_t    next_stage_idx;

    uint32_t attack_length_samples;
    uint32_t decay_length_samples;
    uint32_t release_length_samples;
    float    sustain;
};

// https://www.martin-finke.de/articles/audio-plugins-011-envelopes/
static void adsr_set_params(MyADSR* env, float attack, float decay, float sustain, float release, float sample_rate)
{
    xassert(sample_rate > 0);
    env->attack_length_samples  = attack * sample_rate;
    env->decay_length_samples   = decay * sample_rate;
    env->release_length_samples = release * sample_rate;
    env->sustain                = sustain;
}

static void adsr_calc_multiplier(MyADSR* env, float startLevel, float endLevel, uint32_t lengthInSamples)
{
    float v = 0;
    if (lengthInSamples)
        v = 1.0f + (xm_fastlog(endLevel) - xm_fastlog(startLevel)) / (float)(lengthInSamples);
    xassert(v == v);
    env->multiplier = v;
}

static void adsr_set_stage(MyADSR* env, MyADSRStage new_stage)
{
    env->current_stage = new_stage;
    env->current_idx   = 0;

    switch (new_stage)
    {
    case ADSR_IDLE:
        env->current_level = 0.0f;
        env->multiplier    = 1.0f;
        break;
    case ADSR_ATTACK:
        env->next_stage_idx = env->attack_length_samples;
        if (ADSR_MIN_LEVEL > env->current_level) // Legato voice steal
            env->current_level = ADSR_MIN_LEVEL;
        adsr_calc_multiplier(env, env->current_level, 1.0f, env->next_stage_idx);
        break;
    case ADSR_DECAY:
        env->next_stage_idx = env->decay_length_samples;
        env->current_level  = 1.0f;
        adsr_calc_multiplier(env, env->current_level, xm_maxf(env->sustain, ADSR_MIN_LEVEL), env->next_stage_idx);
        break;
    case ADSR_SUSTAIN:
        env->current_level = env->sustain;
        env->multiplier    = 1.0f;
        break;
    case ADSR_RELEASE:
        env->next_stage_idx = env->release_length_samples;
        // We could go from ATTACK/DECAY to RELEASE,
        // so we're not changing current_level here.
        adsr_calc_multiplier(env, env->current_level, ADSR_MIN_LEVEL, env->next_stage_idx);
        break;
    default:
        break;
    }
}

static float adsr_tick(MyADSR* env)
{
    if (env->current_stage != ADSR_IDLE && env->current_stage != ADSR_SUSTAIN)
    {
        if (env->current_idx == env->next_stage_idx)
        {
            MyADSRStage new_stage = (MyADSRStage)(env->current_stage + 1);
            if (new_stage >= ADSR_NUM_STAGES)
                new_stage = ADSR_IDLE;
            adsr_set_stage(env, new_stage);
        }
        env->current_level *= env->multiplier;
        env->current_idx++;
    }
    // xassert(env->current_level >= 0 && env->current_level <= 1);
    return xm_clampf(env->current_level, 0, 1);
}

// NOTE: synth copy pasted from JUCE example code
//==============================================================================
/** A demo synth sound that's just a basic sine wave.. */
class SineWaveSound final : public SynthesiserSound
{
public:
    SineWaveSound() {}

    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};

//==============================================================================
/** A simple demo synth voice that just plays a sine wave.. */
struct SineWaveVoice final : public SynthesiserVoice
{
public:
    NewProjectAudioProcessor& processor;

    // TODO: add MyADSR to prevent pops on new notes
    MyADSR adsr;

    float cutoff    = 0;
    float resonance = 0;

    float filter_state[2] = {0, 0};

    double phase     = 0.0;
    double phase_inc = 0.0;
    double level     = 0.0;
    double tailOff   = 0.0;

    SineWaveVoice(NewProjectAudioProcessor& _p)
        : processor(_p)
    {
        memset(&adsr, 0, sizeof(adsr));
    }

    bool canPlaySound(SynthesiserSound* sound) override { return dynamic_cast<SineWaveSound*>(sound) != nullptr; }

    void startNote(int midiNoteNumber, float velocity, SynthesiserSound* /*sound*/, int /*currentPitchWheelPosition*/)
        override
    {
        phase   = 0.0;
        level   = velocity * 0.15;
        tailOff = 0.0;

        double Hz = MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        phase_inc = Hz / getSampleRate();

        adsr_set_params(&adsr, 0.05, 1, 1, 0.05, getSampleRate());
        adsr_set_stage(&adsr, ADSR_ATTACK);
    }

    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr_set_stage(&adsr, ADSR_RELEASE);

            if (approximatelyEqual(tailOff, 0.0)) // we only need to begin a tail-off if it's not already doing so - the
                                                  // stopNote method could be called more than once.
                tailOff = 1.0;
        }
        else
        {
            adsr_set_stage(&adsr, ADSR_IDLE);

            clearCurrentNote();
            phase_inc = 0.0;
        }
    }

    void pitchWheelMoved(int /*newValue*/) override
    {
        // not implemented for the purposes of this demo!
    }

    void controllerMoved(int /*controllerNumber*/, int /*newValue*/) override
    {
        // not implemented for the purposes of this demo!
    }

    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    using SynthesiserVoice::renderNextBlock;
};

// NOTE: Based off of generated & copy pasted code from JUCE projucer
struct NewProjectAudioProcessor : public juce::AudioProcessor
{
public:
    Synthesiser synth;
    // NOTE: APVTS is buggy and crash prone. If given more time, replace it with custom params & param attachments!!!
    AudioProcessorValueTreeState state;

    std::atomic<float>* param_cutoff    = 0;
    std::atomic<float>* param_resonance = 0;

    // Time values are in nanoseconds
    uint64_t time_delta_history[128];
    uint64_t time_graph_write_idx   = 0;
    uint64_t time_graph_running_sum = 0;
    uint64_t time_last_process_call = 0;

    //==============================================================================
    NewProjectAudioProcessor();
    ~NewProjectAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewProjectAudioProcessor)
};
