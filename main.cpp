#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect.hpp"
#include <iostream>

using namespace std;
using namespace cv;

#include <thread>
#include <tbb/concurrent_queue.h>
#include <tbb/pipeline.h>

#include "ProcessingChainData.h"
#include "CompressedCapture.h"
#include "FrameCounter.h"

volatile bool done = false; // volatile is enough here. We don't need a mutex for this simple flag.
volatile int framecount = 0; // volatile is enough here. We don't need a mutex for this simple flag.

void tbbqueue( CompressedCapture &capture,
                       ImageQueue &imageQueue
                        );


int main( int argc, const char** argv ) {
	try {
		int devnum = 0;
		bool display = false;
		if(argc>1) devnum = atoi(argv[1]);
		if(argc>2) display = true;

		CompressedCapture cap(devnum, display);

		ImageQueue imageQueue;
		imageQueue.set_capacity(3); 
		auto pipelineRunner = thread( tbbqueue, ref(cap), ref(imageQueue));

		cap.frameCollectorThread(imageQueue);

		pipelineRunner.join(); 
	} catch (exception &e){
		cout << e.what() << endl;
	}	

	return 0;
}

void tbbqueue( CompressedCapture &capture, ImageQueue &imageQueue) {
    tbb::parallel_pipeline(5, 
			tbb::make_filter<void,ProcessingChainData*>(tbb::filter::serial_in_order,
				[&](tbb::flow_control& fc)->ProcessingChainData*{
					return capture.captureFilter(fc);
				}
			)&
			tbb::make_filter<ProcessingChainData*,ProcessingChainData*>(tbb::filter::serial_in_order,
				[&](ProcessingChainData *pData)->ProcessingChainData* {
					return capture.grayscaleFilter(pData);
				}
			)&
			tbb::make_filter<ProcessingChainData*,ProcessingChainData*>(tbb::filter::serial_in_order,
				[&](ProcessingChainData *pData)->ProcessingChainData* {
					return capture.resizeFilter(pData);
				}
			)&
			tbb::make_filter<ProcessingChainData*,ProcessingChainData*>(tbb::filter::serial_in_order,
				[&](ProcessingChainData *pData)->ProcessingChainData* {
					return capture.differenceFilter(pData, imageQueue.size());
				}
			)&
			tbb::make_filter<ProcessingChainData*,ProcessingChainData*>(tbb::filter::serial_in_order,
				[&](ProcessingChainData *pData)->ProcessingChainData* {
					return capture.thresholdFilter(pData);
				}
			)&
			tbb::make_filter<ProcessingChainData*,ProcessingChainData*>(tbb::filter::serial_in_order,
				[&](ProcessingChainData *pData)->ProcessingChainData* {
					return capture.dilateFilter(pData);
				}
			)&
		//push frame to GUI 
		tbb::make_filter<ProcessingChainData*,void>(tbb::filter::serial_in_order,
			[&](ProcessingChainData *pData) {   
				if (!capture.isDone()) {
					try {
						imageQueue.push(pData);
					} catch (...) {
						cout << "Pipeline caught an exception on the queue" << endl;
						capture.stop();
					}
				}
			}
		)
	);

}
