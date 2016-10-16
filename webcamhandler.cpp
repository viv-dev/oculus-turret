#include "webcamhandler.h"

#define PI 3.14159265f
#define HALF_PI (PI / 2.0f)
#define TWO_PI (PI * 2.0f)
#define DEGREES_TO_RADIANS (PI / 180.0f)
#define RADIANS_TO_DEGREES (180.0f / PI)

#define CAMERA_DEVICE 0
#define CAMERA_WIDTH 1280
#define CAMERA_HEIGHT 720
#define CAMERA_HFOV_DEGREES 78f
#define IMAGE_DISTANCE 3.67f
#define CAMERA_DEVICE 0
#define CAMERA_LATENCY 0.040
#define CAMERA_HALF_FOV (CAMERA_HFOV_DEGREES / 2.0f) * DEGREES_TO_RADIANS
#define CAMERA_SCALE (tan(CAMERA_HALF_FOV) * IMAGE_DISTANCE)

WebcamHandler::WebcamHandler() :
	stopped(false),
	changed(false),
	stop(false),
	firstCapture(-1),
	captures(-1),
	cps(-1)
{
	videoCapture.open(CAMERA_DEVICE);

	if (!videoCapture.isOpened())
	{
		std::cout << "Could not connect to camera!" << std::endl;
	}
	else
	{
		std::cout << "Connected to Webcam successfully!" << std::endl;
	}

	if (!videoCapture.read(frame.image))
	{
		std::cout << "Could not grab first frame!" << std::endl;
	}

	//resolution.w = videoCapture.get(CV_CAP_PROP_FRAME_WIDTH);
	//resolution.h = videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT);

	//videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 1920.0);
	//videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 1080.0);

	if (!videoCapture.read(frame.image))
	{
		std::cout << "Could not grab second frame!" << std::endl;
	}

	resolution.w = frame.image.cols;
	resolution.h = frame.image.rows;

	float aspectRatio = (float)frame.image.cols / (float)frame.image.rows;

	std::cout << "Frame width: " << resolution.w << " Frame height: " << resolution.h << std::endl;
	std::cout << "Aspect Ratio: " << aspectRatio << std::endl;
}

WebcamHandler::~WebcamHandler()
{

}

void WebcamHandler::startCapture()
{
	stopped = false;

	

	captureThread = boost::thread(&WebcamHandler::captureLoop, this);
}

void WebcamHandler::stopCapture()
{
	// Set stopped flag to true
	stopped = true;

	// Wait until the thread terminates and returns to the main thred
	captureThread.join();

	// Release the video capture
	videoCapture.release();
}

void WebcamHandler::setFrame(const ImageData & newFrame)
{
	// Lock mutex and set new frame
	mutex.lock();
	frame = newFrame;
	mutex.unlock();
}

bool WebcamHandler::getFrame(ImageData & outFrame)
{
	mutex.lock();
	outFrame = frame;
	mutex.unlock();
	return true;
}

void WebcamHandler::captureLoop()
{
	ImageData capturedFrame;

	while (!stopped)
	{
		videoCapture.read(capturedFrame.image);
		cv::flip(capturedFrame.image.clone(), capturedFrame.image, 0);
		setFrame(capturedFrame);
	}
}
