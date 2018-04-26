#ifndef COMPRESSED_CAPTURE
#define COMPRESSED_CAPTURE

#include <stdexcept>
using std::runtime_error;

#include <iostream>
using std::cout;

#include <string>
using std::string;
using std::to_string;

#include <atomic>
using std::atomic;

#include "opencv2/videoio.hpp"
using cv::VideoCapture;
using cv::VideoWriter;
using cv::Size;
using cv::Point;
using cv::Scalar;

#include "opencv2/highgui.hpp"
using cv::imshow;
using cv::waitKey;

#include <tbb/concurrent_queue.h>
#include <tbb/pipeline.h>

#include "FrameCounter.h"
#include "ProcessingChainData.h"

#define VFORMAT 'X', '2', '6', '4'

typedef tbb::concurrent_bounded_queue<ProcessingChainData*> ImageQueue;

class CompressedCapture {
	VideoCapture capture;
	VideoWriter outputVideo;

	bool display;
	bool started;
	atomic<bool> done;

	FrameCounter fc;
	

	public:
		CompressedCapture(int devnum, bool _display = false);
		void frameCollectorThread(ImageQueue& queue);

		void stop();
		bool isDone();

		ProcessingChainData* captureFilter(tbb::flow_control& fc);
		ProcessingChainData* grayscaleFilter(ProcessingChainData* pData);
		ProcessingChainData* resizeFilter(ProcessingChainData* pData);
		ProcessingChainData* differenceFilter(ProcessingChainData* pData, int queueSize);
		ProcessingChainData* thresholdFilter(ProcessingChainData* pData);
		ProcessingChainData* dilateFilter(ProcessingChainData* pData);
};
#endif
