#ifndef FRAME_COUNTER
#define FRAME_COUNTER

#include <string>
#include <sstream>
#include <ios>

#include "opencv2/core.hpp"

using std::string;
using std::stringstream;
using std::to_string;

using std::fixed;

using cv::getTickCount;
using cv::getTickFrequency;

class FrameCounter {
	int64 startTick;
	int64 lastTick;
	int64 frameCount;
	int64 lastFrameCount;
	
	double f;

	public:
		FrameCounter();
		void count();
		string stop();
		void reset();

};
#endif
