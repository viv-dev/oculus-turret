#include <Windows.h>
#include <wincon.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <math.h>

#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <Extras\OVR_Math.h>
#include <glew.h>
#include <glfw3.h>
#include <glm.hpp>

#include "serialmanager.h"
#include "webcamhandler.h"
#include "riftmanager.h"
#include "display.h"
#include "configreader.h"

#define DEGREES_TO_DIGI 0.2929

//C++98 standard library does not have a round function, this is a workaround for VS2010
#if (_MSC_VER == 1600)
float round(float number)
{
	return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}

double round(double number)
{
	return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}
#endif

using namespace OVR;
using namespace std;

SerialManager m_serialManager;
RiftManager riftManager;
ConfigReader configReader = ConfigReader::getInstance();

bool initSerial()
{
	m_serialManager.Init("\\\\.\\COM5",
						 configReader.getBaudRate(),
						 configReader.getParity(),
						 configReader.getStopBits(),
						 configReader.getByteSize());

	m_serialManager.Start();
	return true;
}


void run(void)
{

	configReader.ParseConfig();
	initSerial();


	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result))
	{
		std::cout << "[RiftManager] ERROR: Failed to initialise Oculus SDK." << std::endl;
		return;
	}

	ovrSession session;
	ovrHmdDesc hmdDesc;
	ovrGraphicsLuid luid;
	result = ovr_Create(&session, &luid);

	if (OVR_FAILURE(result))
	{
		std::cout << "[RiftManager] ERROR: Failed to establish a session with a HDM." << std::endl;
		return;
	}

	hmdDesc = ovr_GetHmdDesc(session);


	bool isEnd = false;

	string recieve;

	ovrSizei resolution = hmdDesc.Resolution; //Gets the resolution appropriate to the Oculus Headset being used.

	cout << endl << endl << "[MainThread] Please put on the Oculus Headset, face the desired direction, and press any button to begin headtracking." << endl;

	getchar();

	ovr_RecenterPose(session);

	while (!isEnd)
	{
		//Grab the current state of the Headset
		ovrTrackingState state = ovr_GetTrackingState(session, ovr_GetTimeInSeconds(), ovrFalse);
		Posef pose = state.HeadPose.ThePose;

		float yaw, pitch, roll;
		float digiYaw, digiPitch, digiRoll;
		pose.Rotation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &pitch, &roll);
		
		/*cout << "Yaw: " << fixed << setprecision(2) << RadToDegree(yaw) << 
				" Pitch: " << fixed << setprecision(2) << RadToDegree(pitch) << 
				" Roll: " << fixed << setprecision(2) << RadToDegree(roll) << endl;*/
		
		std::stringstream ss;
		string message;
		
		/*if (yaw < 0)
		{
			digiYaw = RadToDegree(yaw) + 360.0;
		}
		else
		{
			digiYaw = RadToDegree(yaw);
		} */

		digiYaw = RadToDegree(yaw);
		digiYaw += 150.0;

		if(digiYaw > 300)
			digiYaw = 300;

		if(digiYaw < 0)
			digiYaw = 0;

		digiPitch = RadToDegree(pitch);
		digiPitch += 150.0;
		

		digiYaw		= round(digiYaw/DEGREES_TO_DIGI);
		digiPitch	= round(digiPitch/DEGREES_TO_DIGI);
		digiRoll	= round(RadToDegree(roll) /DEGREES_TO_DIGI);


		//ss << "^yaw:" << setprecision(4) << RadToDegree(yaw) << "pitch:" << setprecision(4) << RadToDegree(pitch) << "roll:" << setprecision(4) << RadToDegree(roll) << "#";
		ss << "^" << digiYaw << "," << digiPitch << "#";
		message = ss.str();

		m_serialManager.Write(message.c_str(), message.size());

		m_serialManager.ReadAvailable(recieve);
		cout << recieve;

		Sleep(20);

		/*HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD coord;
		coord.X = 0;
		coord.Y = 0;
		SetConsoleCursorPosition(hStdOut, coord);*/

	}

	ovr_Destroy(session);

}

int main(int argc, char ** argv)
{
	configReader.ParseConfig();
	initSerial();

	//Initialise the SDK
	if (!riftManager.InitRift())
	{
		cout << "[MainThread] ERROR: Failed to initialise Oculus SDK. Press any key to end." << endl;
		riftManager.DestroyRift();
		getchar();
		return -1;
	}

	//Create display window
	ovrSizei resolution = riftManager.GetResolution();//Gets the resolution appropriate to the Oculus Headset being used.
	Display display(resolution.w, resolution.h); //Create a display handler and setup rendering window

	if (!display.WindowSetup())
	{
		std::cout << "[MainThread] ERROR: Window setup failed... terminating program" << std::endl;
		riftManager.DestroyRift();
		glfwTerminate();
		return -1;
	}

	// Initialise Oculus Rendering Buffers
	if (!riftManager.InitBuffers())
	{
		cout << "[MainThread] ERROR: Failed to initialise Oculus Buffers. Press any key to end." << endl;
		riftManager.DestroyRift();
		glfwTerminate();
		getchar();
		return -1;
	}

	float ovrFov = riftManager.GetFoV();
	ovrSizei ovrBufferSize = riftManager.GetBufferSize();
	
	//Initialise display buffers for verticies and textures
	display.InitRender(ovrFov, ovrBufferSize);

	cout << endl << endl << "[MainThread] Please put on the Oculus Headset, face the desired direction, and press any button to begin headtracking." << endl;
	getchar();
	riftManager.RecenterPose();

	//string startMsg = "^start#";
	//m_serialManager.Write(startMsg.c_str(), startMsg.size());
	//initSerial();
	//run();

	bool stop = false;

	while (!glfwWindowShouldClose(display.GetWindow()) && !stop)
	{
		//Handle events for the frame
		glfwPollEvents();

		riftManager.BeginFrame();

		for (unsigned int eye = 0; eye < EyeIndex::NUM_EYES; eye++)
		{
			riftManager.SetView((EyeIndex)eye, 1.0f, 1000.0f);
			riftManager.BindFrameBuffer((EyeIndex)eye);

			//Rendering actions go here
			if(!display.OnRender(eye))
			{
				stop = true;
				std::cout << "[Main] Rendering error. Press any key to end." << std::endl;

				getchar();
				return -1;
			}

			riftManager.UnBindFrameBuffer((EyeIndex)eye);
		}

		riftManager.EndFrame();

		//Update glfw window
		glfwSwapBuffers(display.GetWindow());

		float yaw, pitch, roll;
		float digiYaw, digiPitch, digiRoll;

		riftManager.GetPose(yaw, pitch, roll);

		std::stringstream ss;
		string message;
		string recieve;

		digiYaw = RadToDegree(yaw);
		digiYaw += 150.0;

		if (digiYaw > 300)
			digiYaw = 300;

		if (digiYaw < 0)
			digiYaw = 0;

		digiPitch = RadToDegree(pitch);
		digiPitch += 150.0;


		digiYaw = round(digiYaw / DEGREES_TO_DIGI);
		digiPitch = round(digiPitch / DEGREES_TO_DIGI);
		digiRoll = round(RadToDegree(roll) / DEGREES_TO_DIGI);


		//ss << "^yaw:" << setprecision(4) << RadToDegree(yaw) << "pitch:" << setprecision(4) << RadToDegree(pitch) << "roll:" << setprecision(4) << RadToDegree(roll) << "#";
		ss << "^" << digiYaw << "," << digiPitch << "#";
		message = ss.str();

		m_serialManager.Write(message.c_str(), message.size());

		m_serialManager.ReadAvailable(recieve);
		cout << recieve;
	}

	// Gracefully terminate program
	riftManager.DestroyRift();
	glfwTerminate(); 

	run();

	return 0;
}

