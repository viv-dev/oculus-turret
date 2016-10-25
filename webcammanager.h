#pragma once
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

#include <opencv2/opencv.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

class WebCamManager
{
public:

	WebCamManager();
	~WebCamManager();

	bool Init(float maxWidth, float maxHeight);
	void StartCapture();
	void StopCapture();
	
	bool GetFrame(cv::Mat & outFrame);

	ovrSizei GetImageResolution() { return resolution; }
	float GetCapturesPerSecond();

	void CaptureLoop();

private:

	void SetFrame(const cv::Mat & newFrame);

	cv::Mat frame;

	cv::VideoCapture videoCapture;

	boost::thread CaptureThread;
	boost::mutex mutex;

	bool isInit;
	bool stopped;

	ovrSizei resolution;
	float aspectRatio;

	float firstCapture;
	int captures;
	float cps;

};