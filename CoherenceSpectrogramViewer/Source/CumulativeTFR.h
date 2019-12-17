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

#ifndef CUMULATIVE_TFR_H_INCLUDED
#define CUMULATIVE_TFR_H_INCLUDED

//#include <FFTWWrapper.h>
#include <OpenEphysFFTW.h>
#include "CircularArray.h"

#include <vector>
#include <complex>

using FFTWArrayType = FFTWTransformableArrayUsing<0U>;
// Changed to FFTW_MEASURE, slow start. Better performance?

class CumulativeTFR
{
	// shorten some things
	template<typename T>
	using vector = std::vector<T>;

	using RealAccum = StatisticsAccumulator<double>;

	struct ComplexWeightedAccum
	{
		ComplexWeightedAccum(double alpha)
			: count(0)
			, sum(0, 0)
			, alpha(alpha)
		{}

		std::complex<double> getAverage()
		{
			return count > 0 ? sum / (double)count : std::complex<double>();
		}

		void addValue(std::complex<double> x)
		{
			sum = x + (1 - alpha) * sum; // Maybe worried about intitial value skewing results..? Could be why its so high always.
			count = 1 + (1 - alpha) * count;
		}

	private:
		std::complex<double> sum;
		size_t count;

		const double alpha;
	};


	struct RealWeightedAccum
	{
		RealWeightedAccum(double alpha)
			: count(0)
			, sum(0)
			, alpha(alpha)
		{}

		double getAverage()
		{
			return count > 0 ? sum / (double)count : double();
		}

		void addValue(double x)
		{
			sum = x + (1 - alpha) * sum;
			count = 1 + (1 - alpha) * count;
		}

	private:
		double sum;
		size_t count;

		const double alpha;
	};

public:
	CumulativeTFR(int ng1, int ng2, int nf, int nt, int Fs,
		float winLen = 2, float stepLen = 0.1, float freqStep = 0.25,
		int freqStart = 1, double fftSec = 10.0, double alpha = 0);

	// Handle a new buffer of data. Preform FFT and create pxxs, pyys.
	void addTrial(FFTWArrayType& fftBuffer, int chan);

	// Function to get coherence between two channels
	void getMeanCoherence(int chanX, int chanY, double* meanDest, int comb);

	// Calculates power for all the input channels based on powerbuffer size. 
	// Returns a vector of vector of float type i.e Vect[] corresponds to vector of power for different frequency
	std::vector<std::vector<float>> getPowerForChannels();


private:
	// Generate wavelet to multplied by the channel spectrum
	void CumulativeTFR::generateWavelet();

	const int nFreqs;
	const int Fs;
	const int nTimes;
	const int nfft;
	int segmentLen;
	float windowLen;
	float stepLen;

	float freqStep;
	int freqStart;
	int freqEnd;

	int trimTime;

	// # channels x # frequencies x # times
	vector<vector<vector<std::complex<double>>>> spectrumBuffer;
	vector<vector<std::complex<double>>> waveletArray;

	FFTWArrayType ifftBuffer;

	// For exponential average
	double alpha;
	// Store cross-spectra : # channel combinations x # frequencies x # times
	vector<vector<vector<ComplexWeightedAccum>>> pxys;
	// Store power : # channels x # frequencies x # times
	vector<vector<vector<RealWeightedAccum>>> powBuffer;

	// calculate a single magnitude-squared coherence from cross spectrum and auto-power values
	static double singleCoherence(double pxx, double pyy, std::complex<double> pxy);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CumulativeTFR);
};

#endif // CUMULATIVE_TFR_H_INCLUDED