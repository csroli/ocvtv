#ifndef PROCESSINGCHAINDATA
#define PROCESSINGCHAINDATA

#include "opencv2/imgproc.hpp"

using cv::Mat;

struct ProcessingChainData {
	Mat img;
	Mat diff, smallImg;
	static Mat reference_frame;
	static Mat mask_frame;
	static int64 reference_time;	
	static bool moved;	
	static bool processed;	
};
#endif
