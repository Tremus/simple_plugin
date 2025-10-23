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
    , att_cutoff(p.APVTS, "cutoff", slider_cutoff)
    , att_resonance(p.APVTS, "resonance", slider_resonance)

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
    static_assert(GRAPH_RESOLUTION < ARRLEN(audioProcessor.time.delta_history), "");
    static_assert(ARRLEN(audioProcessor.time.delta_history) == 128, "");

    spare_path.clear();
    spare_path.preallocateSpace(GRAPH_RESOLUTION);

    // TODO write a ring buffer
    const uint64_t mask      = ARRLEN(audioProcessor.time.delta_history) - 1;
    const uint64_t start_idx = audioProcessor.time.graph_write_idx - 1;

    g.setColour(juce::Colours::white);

    // Allocating stack memory is fast
    double ms_vals[GRAPH_RESOLUTION];

    for (int i = 0; i < ARRLEN(ms_vals); i++)
    {
        uint64_t idx  = start_idx - i;
        idx          &= mask;

        xassert(idx < ARRLEN(audioProcessor.time.delta_history));
        uint64_t time_delta_ns = audioProcessor.time.delta_history[idx];

        // Fast nanoseconds (int) to ms (double)
        double ms  = (time_delta_ns >> 10) * 1024e-6;
        ms_vals[i] = ms;

        if (ms > this->peak_ms)
        {
            this->peak_ms = ms;
        }
    }

    for (int i = 0; i < ARRLEN(ms_vals); i++)
    {
        int x = (int)xm_mapf(i, 0, GRAPH_RESOLUTION - 1, graph_area.getRight(), graph_area.getX());
        int y = (int)xm_mapf(ms_vals[i], 0, this->peak_ms, graph_area.getBottom(), graph_area.getY());

        if (i == 0)
            spare_path.startNewSubPath(x, y);
        else
            spare_path.lineTo(x, y);
    }
    g.strokePath(spare_path, juce::PathStrokeType(2.0f));

    spare_string.clear();
    spare_string += "0ms";

    g.drawText(
        spare_string,
        graph_area.getX() + 10,
        graph_area.getBottom() - 30,
        100,
        20,
        juce::Justification::bottomLeft);

    char label[32];
    snprintf(label, sizeof(label), "%.2lfms", this->peak_ms);

    spare_string.clear();
    spare_string += label;

    g.drawText(spare_string, graph_area.getX() + 10, graph_area.getY() + 10, 100, 20, juce::Justification::topLeft);

    uint64_t avg_ns  = audioProcessor.time.graph_running_sum;
    avg_ns          /= ARRLEN(audioProcessor.time.delta_history);

    double avg_ms = (avg_ns >> 10) * 1024e-6;
    snprintf(label, sizeof(label), "Avg: %.3lfms", avg_ms);
    spare_string.clear();
    spare_string += label;

    g.drawText(
        spare_string,
        graph_area.getRight() - 110,
        graph_area.getY() + 10,
        100,
        20,
        juce::Justification::topRight);
}