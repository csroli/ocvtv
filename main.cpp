#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>

using namespace std;
using namespace cv;

#include <thread>
#include <tbb/concurrent_queue.h>
#include <tbb/pipeline.h>
volatile bool done = false; // volatile is enough here. We don't need a mutex for this simple flag.
volatile int framecount = 0; // volatile is enough here. We don't need a mutex for this simple flag.

struct ProcessingChainData {
	Mat img;
	Mat diff, smallImg;
	static Mat reference_frame;
	static Mat mask_frame;
	static int64 reference_time;	
	static bool moved;	
	static bool processed;	
};
int64 ProcessingChainData::reference_time = 0;
Mat ProcessingChainData::reference_frame = Mat();
Mat ProcessingChainData::mask_frame = Mat();
bool ProcessingChainData::moved = false;
bool ProcessingChainData::processed = false;

void tbbqueue( VideoCapture &capture,
                       tbb::concurrent_bounded_queue<ProcessingChainData *> &guiQueue
                        );


int main( int argc, const char** argv ) {
    VideoCapture capture;
    Mat frame, image;
    string inputName;
    bool tryflip;

    inputName = "";

		if(!capture.open(1))
				cout << "Could not read " << inputName << endl;
    int64 startTime;
    if( capture.isOpened() )
    {
        cout << "Video capturing has been started ..." << endl;
				VideoWriter outputVideo;
				Size S = Size((int) capture.get(cv::CAP_PROP_FRAME_WIDTH),    //Acquire input size
											(int) capture.get(cv::CAP_PROP_FRAME_HEIGHT));
				outputVideo.open("out.mpg" , VideoWriter::fourcc('X', '2', '6', '5'), capture.get(cv::CAP_PROP_FPS),S, true);

        tbb::concurrent_bounded_queue<ProcessingChainData *> guiQueue;
        guiQueue.set_capacity(3); // TBB NOTE: flow control so the pipeline won't fill too much RAM
        auto pipelineRunner = thread( tbbqueue, ref(capture), ref(guiQueue));

        startTime = getTickCount();

				bool started = false;
        ProcessingChainData *pData=0;
        for(;! done;)
        {
            if (guiQueue.size() >2 && guiQueue.try_pop(pData))
            {
                char c = (char)waitKey(1);
                if( c == 27 || c == 'q' || c == 'Q' )
                {
                    done = true;
                }
								framecount++;
					/*			Mat dst;
								addWeighted(pData->smallImg,0.6, pData->mask_frame, 0.4, 0.0, dst);
                imshow( "result", dst );*/
								if(pData->moved || !started){
									if(!outputVideo.isOpened()){
										outputVideo.open("out.mpg" , VideoWriter::fourcc('X', '2', '6', '5'), capture.get(cv::CAP_PROP_FPS),S, true);
									}
									//imshow( "result", pData->img );
									outputVideo<<pData->img;
									started = true;
								}else if(outputVideo.isOpened()){
									outputVideo.release();
								}
								
                delete pData;
                pData = 0;
            }
        }
        double tfreq = getTickFrequency();
        double secs = ((double) getTickCount() - startTime)/tfreq;
        cout << "Execution took " << fixed << secs << " seconds." << endl;
        cout << "AVG FPS was " << (framecount/secs) << endl;
        // TBB NOTE: flush the queue after marking 'done'
        do
        {
            delete pData;
        } while (guiQueue.try_pop(pData));
        pipelineRunner.join(); 
    }
    else
    {
        cerr << "ERROR: Could not initiate capture" << endl;
        return -1;
    }

    return 0;
}

void tbbqueue( VideoCapture &capture,
                       tbb::concurrent_bounded_queue<ProcessingChainData *> &guiQueue
                       )
{
    tbb::parallel_pipeline(5, // TBB NOTE: (recommendation) NumberOfFilters
                           // capturing filter 
                           tbb::make_filter<void,ProcessingChainData*>(tbb::filter::serial_in_order,
                                                                  [&](tbb::flow_control& fc)->ProcessingChainData*
                          {   
                              Mat frame;
                              capture >> frame;
                              if( done || frame.empty() )
                              {
                                  done = true;
                                  fc.stop();
                                  return 0;
                              }
                              auto pData = new ProcessingChainData;
                              pData->img = frame.clone();
                              return pData;
                          }
                          )&
                           //gray 
                           tbb::make_filter<ProcessingChainData*,ProcessingChainData*>(tbb::filter::serial_in_order,
                                                                  [&](ProcessingChainData *pData)->ProcessingChainData*
                          {
															cvtColor( pData->img, pData->smallImg, COLOR_BGR2GRAY );
                              return pData;
                          }
                          )&
													// resize
                           tbb::make_filter<ProcessingChainData*,ProcessingChainData*>(tbb::filter::serial_in_order,
                                                                  [&](ProcessingChainData *pData)->ProcessingChainData*
                          {
															double fx = 1.0 / 2.0;
															resize( pData->smallImg, pData->smallImg, Size(), fx, fx, INTER_LINEAR );
															if(pData->reference_time == 0){
																pData->reference_time = getTickCount();
																pData->reference_frame = pData->smallImg.clone();
																pData->mask_frame = Mat::zeros(pData->smallImg.size(), pData->smallImg.type());
																pData->moved=false;
																pData->processed = true;
															}else if((getTickCount()-pData->reference_time)*5 > getTickFrequency()){
																pData->processed = false;
															}
                              return pData;
                          }
                          )&
                           // diff  
                           tbb::make_filter<ProcessingChainData*,ProcessingChainData*>(tbb::filter::serial_in_order,
                                                                  [&](ProcessingChainData *pData)->ProcessingChainData*
                          {
															if(!pData->processed){
																if(guiQueue.size()>=2){
																	cv::absdiff(pData->smallImg, pData->reference_frame, pData->diff);
																}else{
																	pData->diff = Mat::zeros(pData->smallImg.size(), pData->smallImg.type());
																}
															}
                              return pData;
                          }
                          )&
                           // threshold 
                           tbb::make_filter<ProcessingChainData*,ProcessingChainData*>(tbb::filter::serial_in_order,
                                                                  [&](ProcessingChainData *pData)->ProcessingChainData*
                          {
															if(!pData->processed){
																cv::threshold(pData->diff, pData->diff,255/10,255,THRESH_BINARY);
															}
                              return pData;
                          }
                          )&
                           // dilatation 
                           tbb::make_filter<ProcessingChainData*,ProcessingChainData*>(tbb::filter::serial_in_order,
                                                                  [&](ProcessingChainData *pData)->ProcessingChainData*
                          {
															if(!pData->processed){
																int erode_size=1;
																int dilation_size = 1;
																Mat element = getStructuringElement( MORPH_RECT,
																								Size( 2*dilation_size + 1, 2*dilation_size+1 ),
																								Point( dilation_size, dilation_size ) );
																Mat er_element = getStructuringElement( MORPH_RECT,
																								Size( 2*erode_size + 1, 2*erode_size+1 ),
																								Point( erode_size, erode_size ) );
																erode( pData->diff, pData->diff, er_element );
																dilate( pData->diff, pData->diff, element );

																uchar* p = pData->diff.data;

																int count = 0;
																for( unsigned int i =0; i < pData->diff.cols*pData->diff.rows; ++i)
																		if(*p++ != 0)
																			count++;
																pData->moved = count;	
																if(count){
																	pData->mask_frame = pData->diff;
																	pData->reference_time = getTickCount();
																	pData->reference_frame = pData->smallImg.clone();
																}else{
																	pData->mask_frame = Mat::zeros(pData->smallImg.size(), pData->smallImg.type());
																}
																pData->processed=true;
															}
														return pData;
                          }
                          )&
                           //push frame to GUI 
                           tbb::make_filter<ProcessingChainData*,void>(tbb::filter::serial_in_order,
                                                                  [&](ProcessingChainData *pData)
                          {   
                              if (! done) {
                                  try {
                                      guiQueue.push(pData);
                                  } catch (...) {
                                      cout << "Pipeline caught an exception on the queue" << endl;
                                      done = true;
                                  }
                              }
                          }
                          )
                          );

}
