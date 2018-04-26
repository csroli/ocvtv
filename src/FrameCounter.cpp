#include "FrameCounter.h"

FrameCounter::FrameCounter():frameCount(0), lastFrameCount(0){
	f = getTickFrequency(); 
	reset();
};

void FrameCounter::count(){frameCount++; lastFrameCount++;};

string FrameCounter::stop(){
		double secs = ((double) getTickCount() - startTick)/f;
		stringstream S;
		S << "Execution took " << fixed << secs << " seconds.\n";
		S << "AVG FPS was " << (frameCount/secs) << "\n";
		return S.str();
};

void FrameCounter::reset(){
	frameCount = 0;
	lastFrameCount = 0;
	startTick = getTickCount();
	lastTick = getTickCount();
};
