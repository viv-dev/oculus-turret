#pragma once
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

#include <opencv2/opencv.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>


struct ImageData
{
	cv::Mat image;
};

class WebcamHandler
{
public:

	WebcamHandler();
	~WebcamHandler();

	void startCapture();
	void stopCapture();
	
	bool getFrame(ImageData & outFrame);

	ovrSizei getImageResolution() { return resolution; }
	//float getCapturesPerSecond();

	void captureLoop();

private:

	void setFrame(const ImageData & newFrame);
	bool isStopped() { return stop; }

	ImageData frame;

	cv::VideoCapture videoCapture;

	boost::thread captureThread;
	boost::mutex mutex;

	bool stopped;
	bool changed;
	bool stop;


	ovrSizei resolution;
	float aspectRatio;

	float firstCapture;
	int captures;
	float cps;

};