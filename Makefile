CVFLAGS=`pkg-config opencv --cflags --libs`
#CVFLAGS=-I/usr/include/opencv -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio
all: VidCap

VidCap: main.cpp
	g++ -std=c++11 main.cpp -o VidCap $(CVFLAGS) -pthread -ltbb
