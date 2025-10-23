/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

void add_slider(NewProjectAudioProcessorEditor& editor, juce::Slider& slider, juce::Label& label)
{
    slider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

    label.attachToComponent(&slider, false);
    label.setFont(FontOptions(14.0f));

    editor.addAndMakeVisible(slider);
    editor.addAndMakeVisible(label);
}

//==============================================================================
NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor(NewProjectAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , audioProcessor(p)
    , label_cutoff{{}, "Cutoff:"}
    , label_resonance{{}, "Resonance:"}
    , att_cutoff(p.state, "cutoff", slider_cutoff)
    , att_resonance(p.state, "resonance", slider_resonance)

{
    add_slider(*this, slider_cutoff, label_cutoff);
    add_slider(*this, slider_resonance, label_resonance);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(800, 300);
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor() {}

//==============================================================================

void NewProjectAudioProcessorEditor::resized()
{
    int totalHeight = getHeight();
    // space around layout
    int padding    = 40;
    int num_params = 2;

    int slider_height = (totalHeight - (2 + num_params - 1) * padding) / num_params;

    juce::Rectangle<int> bounds = {padding, padding, 200, slider_height};

    slider_cutoff.setBounds(bounds);
    bounds.translate(0, slider_height + padding);

    slider_resonance.setBounds(bounds);
}

void NewProjectAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // g.setColour(juce::Colours::white);
    // g.setFont(juce::FontOptions(15.0f));
    // g.drawFittedText("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}