#include <Windows.h>
#include <wincon.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <Extras\OVR_Math.h>
#include <glew.h>
#include <glfw3.h>
#include <glm.hpp>

#include "serialmanager.h"
#include "riftmanager.h"
#include "display.h"
#include "configreader.h"


#define DEGREES_TO_DIGI 0.2929

using namespace OVR;

SerialManager serialManager;
RiftManager riftManager;

// Static data class that parses and stores config information
ConfigReader configReader = ConfigReader::getInstance();

int run()
{
	// Read config from config file
	if (!configReader.ParseConfig())
	{
		std::cout << "[MainThread] Failed to read config. Press any key to end." << std::endl;
		getchar();
		return -1;
	}

	// Setup and connect to Arbotix-M serial port
	std::string serialPort = "\\\\.\\" + configReader.getPort();
	std::cout << "[MainThread] Trying to open serial line on port: " << configReader.getPort() << std::endl;
	if (!serialManager.Init(serialPort,
							configReader.getBaudRate(),
							configReader.getParity(),
							configReader.getStopBits(),
							configReader.getByteSize()))
	{
		std::cout << "[MainThread] ERROR: Serial initialisation error. Press any key to end." << std::endl;
		getchar();
		return -1;
	}
	//serialManager.Start();		// Start the serial threads
	serialManager.SendRstMsg(); // Make sure platform is centered

	// Initialise the SDK
	if (!riftManager.InitRift())
	{
		std::cout << "[MainThread] ERROR: Failed to initialise Oculus SDK. Press any key to end." << std::endl;
		riftManager.Destroy();
		getchar();
		return -1;
	}

	// Recenter the reference position based on the direction the user is facing
	std::cout << std::endl << std::endl << "[MainThread] Please put on the Oculus Headset, face the desired direction, and press any button to begin headtracking." << std::endl;
	getchar();
	riftManager.RecenterPose();

	// Create display window
	ovrSizei resolution = riftManager.GetResolution();	//Gets the resolution appropriate to the Oculus Headset being used.
	Display display(resolution.w, resolution.h);		//Create a display handler and setup rendering window

	if (!display.WindowSetup())
	{
		std::cout << "[MainThread] ERROR: Failed to setup window. Press any key to end." << std::endl;
		riftManager.Destroy();
		display.Destroy();
		getchar();
		return -1;
	}

	// Initialise Oculus Rendering Buffers
	if (!riftManager.InitBuffers())
	{
		std::cout << "[MainThread] ERROR: Failed to initialise Oculus Buffers. Press any key to end." << std::endl;
		riftManager.Destroy();
		display.Destroy();
		getchar();
		return -1;
	}

	// Initialise display buffers for verticies and textures
	display.InitRender(riftManager.GetFoV(), riftManager.GetBufferSize(), configReader.getWidth(), configReader.getHeight());

	while (!glfwWindowShouldClose(display.GetWindow()))
	{
		// Handle events for the frame
		glfwPollEvents();

		riftManager.BeginFrame();

		for (unsigned int eye = 0; eye < EyeIndex::NUM_EYES; eye++)
		{
			riftManager.SetView((EyeIndex)eye, 1.0f, 1000.0f);
			riftManager.BindFrameBuffer((EyeIndex)eye);

			if (!display.OnRender(eye))
			{
				std::cout << "[Main] Rendering error. Press any key to end." << std::endl;
				riftManager.Destroy();
				display.Destroy();
				glfwTerminate();
				getchar();
				return -1;
			}

			riftManager.UnBindFrameBuffer((EyeIndex)eye);
		}

		riftManager.EndFrame();

		// Update glfw window
		glfwSwapBuffers(display.GetWindow());

		// Send headpose
		float yaw, pitch, roll;
		riftManager.GetPose(yaw, pitch, roll);
		serialManager.SendPoseMsg(yaw, pitch, roll);
	}

	riftManager.Destroy();
	display.Destroy();
	std::cout << "[Main] Terminating program..." << std::endl;

	return 0;
}


int main(int argc, char ** argv)
{
	return run();
}

