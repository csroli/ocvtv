#include "CompressedCapture.h"

CompressedCapture::CompressedCapture(int devnum, bool _display):
	display(_display),
	started(false),
	fc()
{
	if(!capture.open(devnum))
		throw runtime_error("Can't open device: " + to_string(devnum)); 

	if(!capture.isOpened()){
		throw runtime_error("Can't use device: " + to_string(devnum)); 
	}
	done.store(false);
}

void CompressedCapture::stop(){
	done.store(true);
};

bool CompressedCapture::isDone(){
	return done.load();
};

void CompressedCapture::frameCollectorThread(ImageQueue& queue){
	ProcessingChainData *pData=0;
	Mat empty(1,1,CV_8UC1,Scalar(0));
	imshow( "MonitorColor" , empty);
	for(;!done;){
		if(queue.size() > 2 && queue.try_pop(pData)){
				char c = (char)cv::waitKey(1);
				if( c == 27 || c == 'q' || c == 'Q' ){
						done = true;
				}
	/*			Mat dst;
				addWeighted(pData->smallImg,0.6, pData->mask_frame, 0.4, 0.0, dst);*/
				fc.count();
				if(pData->moved || !started){
					if(!outputVideo.isOpened()){
						Size S = Size((int) capture.get(cv::CAP_PROP_FRAME_WIDTH),    
													(int) capture.get(cv::CAP_PROP_FRAME_HEIGHT));
						outputVideo.open("out.avi" , VideoWriter::fourcc(VFORMAT), capture.get(cv::CAP_PROP_FPS),S, true);
					}
					if(display){
						imshow( "MonitorColor", pData->img );
					}
					outputVideo<<pData->img;
					if(!started) started = true;
				}				

				delete pData;
				pData = 0;
		}
	}
	do {
			delete pData;
	} while (queue.try_pop(pData));
	auto msg = fc.stop();
	cout << msg;
}


ProcessingChainData* CompressedCapture::captureFilter(tbb::flow_control& fc){
	Mat frame;
	capture >> frame;
	if( done || frame.empty() ) {
			done.store(true);
			fc.stop();
			return 0;
	}
	auto pData = new ProcessingChainData;
	pData->img = frame.clone();
	return pData;
};

ProcessingChainData* CompressedCapture::grayscaleFilter(ProcessingChainData* pData){
	cvtColor( pData->img, pData->smallImg, cv::COLOR_BGR2GRAY );
	return pData;
};

ProcessingChainData* CompressedCapture::resizeFilter(ProcessingChainData* pData){
	double fx = 1.0 / 2.0;
	resize( pData->smallImg, pData->smallImg, Size(), fx, fx, cv::INTER_LINEAR );
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
};

ProcessingChainData* CompressedCapture::differenceFilter(ProcessingChainData* pData, int queueSize){
	if(!pData->processed){
		if(queueSize >= 2){
			cv::absdiff(pData->smallImg, pData->reference_frame, pData->diff);
		}else{
			pData->diff = Mat::zeros(pData->smallImg.size(), pData->smallImg.type());
		}
	}
	return pData;
};

ProcessingChainData* CompressedCapture::thresholdFilter(ProcessingChainData* pData){
	if(!pData->processed){
		cv::threshold(pData->diff, pData->diff,255/10,255,cv::THRESH_BINARY);
	}
	return pData;
};

ProcessingChainData* CompressedCapture::dilateFilter(ProcessingChainData* pData){
	if(!pData->processed){
		int erode_size=1;
		int dilation_size = 1;
		Mat element = getStructuringElement( cv::MORPH_RECT,
			Size( 2*dilation_size + 1, 2*dilation_size+1 ),
			Point( dilation_size, dilation_size ) );
		Mat er_element = getStructuringElement( cv::MORPH_RECT,
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
};
