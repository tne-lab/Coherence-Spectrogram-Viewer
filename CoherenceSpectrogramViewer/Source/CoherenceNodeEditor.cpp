/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
Copyright (C) 2019 Translational NeuroEngineering Laboratory

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "CoherenceNodeEditor.h"

/************** editor *************/
CoherenceEditor::CoherenceEditor(CoherenceNode* p)
    : VisualizerEditor(p, 300, true)
{
    tabText = "TFR-Coherence & Spectrogram";
    processor = p;
    // Segment length
    int x = 0, y = 0, h = 0, w = 0;
    // Change to column/row setup
    segLabel = createLabel("segLabel", "Segment Length(s):", { x + 5, y + 25, w + 70, h + 27 });
    addAndMakeVisible(segLabel);

    segEditable = createEditable("segEditable", "4", "Input length of segment", { x + 75, y + 25, w + 35, h + 27 });
    addAndMakeVisible(segEditable);

    // Window Length
    y += 35;
    winLabel = createLabel("winLabel", "Window Length(s):", { x + 5, y + 25, w + 70, h + 27 });
    addAndMakeVisible(winLabel);

    winEditable = createEditable("winEditable", "2", "Input length of window", { x + 75, y + 25, w + 35, h + 27 });
    addAndMakeVisible(winEditable);

    // Step Length
    y += 35;
    stepLabel = createLabel("stepLabel", "Step Length(s):", { x + 5, y + 25, w + 75, h + 27 });
    addAndMakeVisible(stepLabel);

    stepEditable = createEditable("stepEditable", "0.1", "Input step size between windows; higher number = less resource intensive",
    { x + 75, y + 25, w + 35, h + 27 });
    addAndMakeVisible(stepEditable);

    // Frequencies of interest
    //y = 0;
    //x += 105;
    //foiLabel = createLabel("foiLabel", "Frequencies of Interest", { x + 15, y + 25, w + 80, h + 27 });
    //addAndMakeVisible(foiLabel);

    //// Start freq
    //y += 35;
    //fstartLabel = createLabel("fstartLabel", "Freq Start(Hz):", { x + 5, y + 25, w + 70, h + 27 });
    //addAndMakeVisible(fstartLabel);

    //fstartEditable = createEditable("fstartEditable", "1", "Start of range of frequencies", { x + 75, y + 25, w + 35, h + 27 });
    //addAndMakeVisible(fstartEditable);

    //// End Freq
    //y += 35;
    //fendLabel = createLabel("fendLabel", "Freq End(Hz):", { x + 5, y + 25, w + 70, h + 27 });
    //addAndMakeVisible(fendLabel);

    //fendEditable = createEditable("fendEditable", "40", "End of range of frequencies", { x + 75, y + 25, w + 35, h + 27 });
    //addAndMakeVisible(fendEditable);

    setEnabledState(false);
}

CoherenceEditor::~CoherenceEditor() {}

Label* CoherenceEditor::createEditable(const String& name, const String& initialValue,
    const String& tooltip, juce::Rectangle<int> bounds)
{
    Label* editable = new Label(name, initialValue);
    editable->setEditable(true);
    editable->addListener(this);
    editable->setBounds(bounds);
    editable->setColour(Label::backgroundColourId, Colours::grey);
    editable->setColour(Label::textColourId, Colours::white);
    if (tooltip.length() > 0)
    {
        editable->setTooltip(tooltip);
    }
    return editable;
}

Label* CoherenceEditor::createLabel(const String& name, const String& text,
    juce::Rectangle<int> bounds)
{
    Label* label = new Label(name, text);
    label->setBounds(bounds);
    label->setFont(Font("Small Text", 12, Font::plain));
    label->setColour(Label::textColourId, Colours::darkgrey);
    return label;
}

void CoherenceEditor::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{

}

void CoherenceEditor::labelTextChanged(Label* labelThatHasChanged)
{
    processor->updateReady(false);
    auto processor = static_cast<CoherenceNode*>(getProcessor());
    if (labelThatHasChanged == segEditable)
    {
        int newVal;
        if (updateIntLabel(labelThatHasChanged, 0, INT_MAX, 8, &newVal))
        {
            processor->setParameter(CoherenceNode::SEGMENT_LENGTH, static_cast<int>(newVal));
        }
    }
    if (labelThatHasChanged == winEditable)
    {
        float newVal;
        if (updateFloatLabel(labelThatHasChanged, 0, INT_MAX, 8, &newVal))
        {
            processor->setParameter(CoherenceNode::WINDOW_LENGTH, static_cast<float>(newVal));
        }
    }
    /*if (labelThatHasChanged == fstartEditable)
    {
        int newVal;
        if (updateIntLabel(labelThatHasChanged, 0, INT_MAX, 8, &newVal))
        {
            processor->setParameter(CoherenceNode::START_FREQ, static_cast<int>(newVal));
        }
    }
    if (labelThatHasChanged == fendEditable)
    {
        int newVal;
        if (updateIntLabel(labelThatHasChanged, 0, INT_MAX, 8, &newVal))
        {
            processor->setParameter(CoherenceNode::END_FREQ, static_cast<int>(newVal));
        }
    }*/
    if (labelThatHasChanged == stepEditable)
    {
        float newVal;
        if (updateFloatLabel(labelThatHasChanged, 0, INT_MAX, 8, &newVal))
        {
            processor->setParameter(CoherenceNode::STEP_LENGTH, static_cast<float>(newVal));
        }
    }
}

bool CoherenceEditor::updateIntLabel(Label* label, int min, int max, int defaultValue, int* out)
{
    const String& in = label->getText();
    int parsedInt;
    try
    {
        parsedInt = std::stoi(in.toRawUTF8());
    }
    catch (const std::logic_error&)
    {
        label->setText(String(defaultValue), dontSendNotification);
        return false;
    }

    *out = jmax(min, jmin(max, parsedInt));

    label->setText(String(*out), dontSendNotification);
    return true;
}

// Like updateIntLabel, but for floats
bool CoherenceEditor::updateFloatLabel(Label* label, float min, float max,
    float defaultValue, float* out)
{
    const String& in = label->getText();
    float parsedFloat;
    try
    {
        parsedFloat = std::stof(in.toRawUTF8());
    }
    catch (const std::logic_error&)
    {
        label->setText(String(defaultValue), dontSendNotification);
        return false;
    }

    *out = jmax(min, jmin(max, parsedFloat));

    label->setText(String(*out), dontSendNotification);
    return true;
}

void CoherenceEditor::startAcquisition()
{
    canvas->beginAnimation();
}

void CoherenceEditor::stopAcquisition()
{
    canvas->endAnimation();
}

void CoherenceEditor::channelChanged(int chan, bool newState)
{
    CoherenceVisualizer* cohCanvas = static_cast<CoherenceVisualizer*>(canvas.get());
    cohCanvas->channelChanged(chan, newState);
}

Visualizer* CoherenceEditor::createNewCanvas()
{
    canvas = new CoherenceVisualizer(processor);
    return canvas;
}
