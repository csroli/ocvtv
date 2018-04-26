#include "ProcessingChainData.h"

int64 ProcessingChainData::reference_time = 0;
Mat ProcessingChainData::reference_frame = Mat();
Mat ProcessingChainData::mask_frame = Mat();
bool ProcessingChainData::moved = false;
bool ProcessingChainData::processed = false;

