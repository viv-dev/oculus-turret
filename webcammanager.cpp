#include "webcammanager.h"

#define CAMERA_DEVICE 0

WebCamManager::WebCamManager() :
	isInit(false),
	stopped(false),
	firstCapture(-1),
	captures(-1),
	cps(-1)
{

}

WebCamManager::~WebCamManager()
{
	StopCapture();

	std::cout << "[WebCamManager] deconstructed!." << std::endl;
}

bool WebCamManager::Init(float maxWidth, float maxHeight)
{
	videoCapture.open(CAMERA_DEVICE);

	if (!videoCapture.isOpened())
	{
		std::cout << "[Webcam Handler] Could not connect to camera!" << std::endl;
		videoCapture.release();
		return false;
	}
	
	std::cout << "[Webcam Handler] Connected to Webcam successfully!" << std::endl;

	if (!videoCapture.read(frame))
	{
		std::cout << "[Webcam Handler] Could not grab first frame!" << std::endl;
		videoCapture.release();
		return false;
	}

	videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, maxWidth);
	videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, maxHeight);
	videoCapture.set(CV_CAP_PROP_FPS, 60);

	if (!videoCapture.read(frame))
	{
		std::cout << "[Webcam Handler] Could not grab second frame!" << std::endl;
		videoCapture.release();
		return false;
	}

	resolution.w = frame.cols;
	resolution.h = frame.rows;

	float aspectRatio = (float)frame.cols / (float)frame.rows;

	std::cout << "[Webcam Handler] Frame width: " << resolution.w << " Frame height: " << resolution.h << std::endl;
	std::cout << "[Webcam Handler] Aspect Ratio: " << aspectRatio << std::endl;

	isInit = true;
}

void WebCamManager::StartCapture()
{
	if (isInit)
	{
		stopped = false;
		CaptureThread = boost::thread(&WebCamManager::CaptureLoop, this);
	}
	else
		std::cout << "[Webcam Handler] Cannot start capture thread without initialising video capture source." << std::endl;
}

void WebCamManager::StopCapture()
{
	if (stopped == false)
	{
		// Set stopped flag to true
		stopped = true;

		// Wait until the thread terminates and returns to the main thread
		CaptureThread.join();
	}
	
	if (isInit)
	{
		// Release the video capture
		videoCapture.release();
	}

}

void WebCamManager::SetFrame(const cv::Mat & newFrame)
{
	// Lock mutex and set new frame
	mutex.lock();
	frame = newFrame;
	mutex.unlock();
}

bool WebCamManager::GetFrame(cv::Mat & outFrame)
{
	// Lock mutex and get new frame
	mutex.lock();
	outFrame = frame;
	mutex.unlock();
	return true;
}

// Capture thread that grabs webcam images as they become available
void WebCamManager::CaptureLoop()
{
	cv::Mat capturedFrame;
	cv::Mat flippedFrame;

	while (!stopped)
	{
		// Grab frame
		videoCapture.read(capturedFrame);

		// Flip image vertically to align with OpenGL co-ordinates
		cv::flip(capturedFrame, flippedFrame, 0);

		// Store frame
		SetFrame(flippedFrame);
	}
}
