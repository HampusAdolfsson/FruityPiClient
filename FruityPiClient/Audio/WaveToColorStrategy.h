#pragma once
#include "Color.h"
#include <time.h>
#include <fstream>

// Size of softening filter. Higher value means more smooth output, but lower responsiveness.
#define MEAN_FILTER_SIZE 8

#define SCALING 10

/**
*	Changes a signal into a lower frequency one
*/
class WavetoColorStrategy
{
	unsigned int rollingAvg;
	unsigned int prevMean[MEAN_FILTER_SIZE];
	size_t		 prevValsIndex;
	Color		 baseColor;

public:
	WavetoColorStrategy();
	Color getColor(const char *buffer, const size_t& bufSiz, const size_t& sampleSize);
};
