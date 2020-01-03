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

#ifndef COHERENCE_VIS_H_INCLUDED
#define COHERENCE_VIS_H_INCLUDED

#include "AtomicSynchronizer.h"
#include "CoherenceNode.h"
#include <VisualizerWindowHeaders.h>
//#include "../../Processors/Visualization/MatlabLikePlot.h"
#include "../../Source/Processors/Visualization/MatlabLikePlot.h"
class VerticalGroupSet : public Component
{
public:
	VerticalGroupSet(Colour backgroundColor = Colours::silver);
	VerticalGroupSet(const String& componentName, Colour backgroundColor = Colours::silver);
	~VerticalGroupSet();

	void addGroup(std::initializer_list<Component*> components);

private:
	Colour bgColor;
	int leftBound;
	int rightBound;
	OwnedArray<DrawableRectangle> groups;
	static const int PADDING = 5;
	static const int CORNER_SIZE = 8;
};

class CoherenceVisualizer : public Visualizer
	, public ComboBox::Listener
	, public Button::Listener
	, public Label::Listener
{
public:
	CoherenceVisualizer(CoherenceNode* n);
	~CoherenceVisualizer();

	void resized() override;

	void refreshState() override;
	void update() override;
	void refresh() override;
	void beginAnimation() override;
	void endAnimation() override;
	void setParameter(int, float) override;
	void setParameter(int, int, int, float) override;
	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;
	void labelTextChanged(Label* labelThatHasChanged) override;
	void buttonEvent(Button* buttonEvent);
	void buttonClicked(Button* buttonClick) override;
	void paint(Graphics& g) override;
	void UpdateElectrodeOnTransition();
	void UpdateVisualizerStateOntransition(bool flag);
	// Add/remove active channels (when changed in editor/new source) from group options
	void channelChanged(int chan, bool newState);

private:
	// Update list of combinations to choose to graph.
	void updateCombList();
	// Update state of buttons based on grouping changing from non clicking ways
	void updateGroupState();
	// Update buttons based on inputs (checks if you have too many or too few buttons for the number of inputs).
	void updateElectrodeButtons(int numInputs, int numButtons);
	// creates a button for both group 1 and 2
	void createElectrodeButton(int index);

	CoherenceNode* processor;

	ScopedPointer<Viewport>  viewport;
	ScopedPointer<Component> canvas;
	juce::Rectangle<int> canvasBounds;

	//ScopedPointer<MatlabLikePlot> referencePlot;
	//ScopedPointer<MatlabLikePlot> currentPlot;

	ScopedPointer<Label> optionsTitle;

	ScopedPointer<VerticalGroupSet> channelGroupSet;
	ScopedPointer<Label> group1Title;
	ScopedPointer<Label> group2Title;
	Array<ElectrodeButton*> group1Buttons;
	Array<ElectrodeButton*> group2Buttons;

	ScopedPointer<VerticalGroupSet> combinationGroupSet;
	ScopedPointer<Label> combinationLabel;
	ScopedPointer<ComboBox> combinationBox;

	ScopedPointer<VerticalGroupSet> columnTwoSet;

	ScopedPointer<ToggleButton> linearButton;
	ScopedPointer<ToggleButton> expButton;
	ScopedPointer<Label> alpha;
	ScopedPointer<Label> alphaE;

	ScopedPointer<Label> artifactDesc;
	ScopedPointer<Label> artifactEq;
	ScopedPointer<Label> artifactE;
	ScopedPointer<Label> artifactCount;

	ScopedPointer<TextButton> resetTFR;
	ScopedPointer<TextButton> clearGroups;
	ScopedPointer<TextButton> defaultGroups;

	ScopedPointer<Label> foiLabel;
	ScopedPointer<Label> fstartLabel;
	ScopedPointer<Label> fstartEditable;
	ScopedPointer<Label> fendLabel;
	ScopedPointer<Label> fendEditable;

	bool ChanNumChange = false;
	int lastDelElement;
	int lastAddElement;

	Array<int> group1Channels;
	Array<int> group2Channels;

	Array<int> group1ChannelsCoh2Spec;
	Array<int> group2ChannelsCoh2Spec;

	float freqStep;
	int nCombs;
	int curComb;

	int freqStart;
	int freqEnd;

	ScopedPointer<MatlabLikePlot> cohPlot;
	std::vector<double> coherence;
	std::vector<std::vector<float>> coh;

	bool IsSpectrogram = false;
	ScopedPointer<ToggleButton> CoherenceViewer;
	ScopedPointer<ToggleButton> SpectrogramViewer;
	ScopedPointer<Label> SpecCalText;
	std::vector<ScopedPointer<MatlabLikePlot>> plotHoldingVect;
	
	/*End*/

	bool updateIntLabel(Label* label, int min, int max, int defaultValue, int* out);
	bool updateFloatLabel(Label* label, float min, float max,
		float defaultValue, float* out);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CoherenceVisualizer);
};

#endif // COHERENCE_VIS_H_INCLUDED