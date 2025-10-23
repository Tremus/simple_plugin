/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

//==============================================================================
/**
 */
struct NewProjectAudioProcessorEditor
    : public juce::AudioProcessorEditor
    , protected juce::Timer
{
public:
    NewProjectAudioProcessor& audioProcessor;

    Slider                                         slider_cutoff, slider_resonance;
    Label                                          label_cutoff, label_resonance;
    AudioProcessorValueTreeState::SliderAttachment att_cutoff, att_resonance;

    double peak_ms = 16;
    // Reuse the same buffers. Required framework types by juce::Graphics
    juce::String spare_string;
    juce::Path   spare_path;

    juce::Rectangle<int> graph_area = {0, 0, 0, 0};

    NewProjectAudioProcessorEditor(NewProjectAudioProcessor&);
    ~NewProjectAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewProjectAudioProcessorEditor)
};
