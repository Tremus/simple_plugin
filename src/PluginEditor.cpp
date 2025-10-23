/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <xhl/time.h>

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

    startTimerHz(30);
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor() {}

//==============================================================================

void NewProjectAudioProcessorEditor::resized()
{
    int totalHeight = getHeight();
    int totalWidth  = getWidth();
    // space around layout
    int padding    = 40;
    int num_params = 2;

    int slider_height = (totalHeight - (2 + num_params - 1) * padding) / num_params;

    juce::Rectangle<int> bounds = {padding, padding, 200, slider_height};

    slider_cutoff.setBounds(bounds);
    bounds.translate(0, slider_height + padding);

    slider_resonance.setBounds(bounds);

    graph_area.setX(bounds.getRight() + padding);
    graph_area.setRight(totalWidth - 20);
    graph_area.setY(padding);
    graph_area.setBottom(totalHeight - padding);
}

void NewProjectAudioProcessorEditor::timerCallback()
{
    if (graph_area.getX() == 0) // not initialised
        return;

    // Repaint the background to save on compositing CPU/GPU cost, which juce::Components can quickly consume a lot of
    repaint(graph_area);
}

void NewProjectAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::black);
    g.fillRect(graph_area);

    enum
    {
        GRAPH_RESOLUTION = 64,
    };
    static_assert(GRAPH_RESOLUTION < ARRLEN(audioProcessor.time_delta_history), "");

    juce::Path path;
    path.preallocateSpace(GRAPH_RESOLUTION);

    const uint64_t mask      = ARRLEN(audioProcessor.time_delta_history) - 1;
    const uint64_t start_idx = audioProcessor.time_graph_write_idx - 1;

    g.setColour(juce::Colours::white);
    for (int i = 0; i < GRAPH_RESOLUTION; i++)
    {
        float x = xm_mapf(i, 0, GRAPH_RESOLUTION - 1, graph_area.getRight(), graph_area.getX());

        uint64_t idx  = start_idx - i;
        idx          &= mask;

        xassert(idx < ARRLEN(audioProcessor.time_delta_history));
        uint64_t time_delta_ns = audioProcessor.time_delta_history[idx];

        double ms = xtime_convert_ns_to_ms(time_delta_ns);

        float y = xm_mapf(ms, 0, 16, graph_area.getBottom(), graph_area.getY());

        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    g.strokePath(path, juce::PathStrokeType(2.0f));
}