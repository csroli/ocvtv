CC=g++
CXXFLAGS=-std=c++11 -Iinclude
CVFLAGS=`pkg-config opencv --cflags`
CVLIBS=`pkg-config opencv --libs`
OFLAGS=-g -c
#CVFLAGS=-I/usr/include/opencv -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio
OBJECTS=lib/ProcessingChainData.o lib/CompressedCapture.o lib/FrameCounter.o
all: VidCap

VidCap: main.cpp $(OBJECTS)
	$(CC) $(CXXFLAGS) $(CVFLAGS) $^ -o $@  -pthread -ltbb $(CVLIBS)

lib/ProcessingChainData.o: src/ProcessingChainData.cpp
	$(CC) $(CXXFLAGS) $(CVFLAGS) $(OFLAGS) $^ -o $@ $(CVLIBS)

lib/CompressedCapture.o: src/CompressedCapture.cpp
	$(CC) $(CXXFLAGS) $(CVFLAGS) $(OFLAGS) $^ -o $@ -ltbb $(CVLIBS)

lib/FrameCounter.o: src/FrameCounter.cpp
	$(CC) $(CXXFLAGS) $(CVFLAGS) $(OFLAGS) $^ -o $@ $(CVLIBS)

clean:
	rm -rf lib/*
	rm -rf VidCap
	
.PHONY: clean
