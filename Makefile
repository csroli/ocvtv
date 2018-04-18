
all: VidCap

VidCap: main.cpp
	g++ main.cpp -o VidCap -I/usr/include/opencv -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio
