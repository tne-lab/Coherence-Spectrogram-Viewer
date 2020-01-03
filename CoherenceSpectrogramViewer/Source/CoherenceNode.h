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

#ifndef COHERENCE_NODE_H_INCLUDED
#define COHERENCE_NODE_H_INCLUDED

/*

Coherence Node - continuously compute and display magnitude-squared coherence
(measure of phase synchrony) between pairs of LFP signals for a set of frequencies
of interest. Displays either raw coherence values or change from a saved baseline,
in units of z-score.

*/

#include <ProcessorHeaders.h>
#include <VisualizerEditorHeaders.h>

//#include "CoherenceVisualizer.h"
//
#include "AtomicSynchronizer.h"
#include "CumulativeTFR.h"

#include <time.h>
#include <vector>
#include <chrono>
#include <ctime> 
#include <iostream>
#include<fstream>

class CoherenceNode : public GenericProcessor, public Thread
{
	friend class CoherenceEditor;
	friend class CoherenceVisualizer;
public:
	CoherenceNode();
	~CoherenceNode();

	bool hasEditor() const override;

	AudioProcessorEditor* createEditor() override;

	void createEventChannels() override;

	void setParameter(int parameterIndex, float newValue) override;

	void process(AudioSampleBuffer& continuousBuffer) override;

	bool isReady() override;
	bool enable() override;
	bool disable() override;

	// thread function - coherence calculation
	void run() override;

	// Handle changing channels/groups
	void updateSettings() override;

	// Returns array of active input channels 
	Array<int> getActiveInputs();

	// Get source info
	int getFullSourceID(int chan);

	// Save info
	void saveCustomParametersToXml(XmlElement* parentElement) override;
	void loadCustomParametersFromXml();

	// Variable to store Power across all channels 
	// ttlpwr[1] corresponds to channel 1 power across frequency
	std::vector<std::vector<float>> ttlpwr;
	Array<int> TotalNumofChannels;



private:

	AtomicallyShared<Array<FFTWArrayType>> dataBuffer;
	// # Freqs x # Combinations
	AtomicallyShared<std::vector<std::vector<double>>> meanCoherence;

	ScopedPointer<CumulativeTFR> TFR;
	Array<bool> CHANNEL_READY;

	bool ready;

	// freq of interest
	Array<float> foi;

	// Segment Length
	int segLen;
	// Window Length
	float winLen;
	// Step Length
	float stepLen; // Iterval between times of interest
	// Interp Ratio ??
	int interpRatio; //

	// Array of channels for regions 1 and 2
	Array<int> group1Channels;
	Array<int> group2Channels;

	uint32 validSubProcFullID;

	// returns the region for the requested channel
	int getChanGroup(int chan);

	// Append FFTWArrays to data buffer
	void updateDataBufferSize(int newSize);
	void updateMeanCoherenceSize();

	///// TFR vars
	// Number of channels for region 1
	int nGroup1Chans;
	// Number of channels for region 2
	int nGroup2Chans;
	// Number of freq of interest
	int nFreqs;
	float freqStep;
	int freqStart;
	int freqEnd;
	// Number of times of interest
	int nTimes;
	// Fs (sampling rate?)
	float Fs;

	float alpha;

	int nSamplesAdded; // holds how many samples were added for each channel
	AudioBuffer<float> channelData; // Holds the segment buffer for each channel.
	int nSamplesWait; // How many seconds to wait after an artifact is seen.
	int nSamplesWaited; // Holds how many samples we've waited after an artifact. (wait 1 second before getting data again)

	// Total Combinations
	int nGroupCombs;

	// from 0 to 10
	static const int COH_PRIORITY = 5;
	const char * path;

	// Get iterator for this channel in it's respective group
	int getGroupIt(int group, int chan);

	void updateGroup(Array<int> group1Channels, Array<int> group2Channels);
	void updateAlpha(float alpha);
	void resetTFR();
	void updateReady(bool isReady);

	// Artifact checking
	void discardCurBuffer(int nSamples);
	float artifactThreshold;
	int numTrials;
	float numArtifacts;

	std::ofstream cohFile;
	void checkCohFile();

	// This is to store data in case of switch and we wish to retrive old data
	AtomicallyShared<Array<FFTWArrayType>> dataBufferII;
	// Int variable can occupy two values 1 and 0. 
	// 1 means Coherence
	// 0 means Spectrogram
	// This seperates coherence and spectrogram logically
    int WhatisIT; // MOVED TO .cpp when created. = 1;//1-Coherence & 0-Spectrogram
	/*End*/

	enum Parameter
	{
		SEGMENT_LENGTH,
		WINDOW_LENGTH,
		START_FREQ,
		END_FREQ,
		STEP_LENGTH,
		ARTIFACT_THRESHOLD
	};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CoherenceNode);
};

#endif // COHERENCE_NODE_H_INCLUDED