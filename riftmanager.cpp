#include "riftmanager.h"

#include <algorithm> 
#include <iostream>
#include <sstream>
#include <math.h>

#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <Extras\OVR_Math.h>

#define DEGREES_TO_DIGI 0.2929

RiftManager::RiftManager()
{}

RiftManager::~RiftManager()
{
	std::cout << "[RiftManager] Destroying HDM session." << std::endl;
	ovr_Destroy(session);

	std::cout << "[RiftManager] Shutting down OVR." << std::endl;
	ovr_Shutdown();

	session = nullptr;

	std::cout << "[RiftManager] Shutting down complete." << std::endl;
}

bool RiftManager::InitRift()
{
	// Initialise the headset
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result))
	{
		std::cout << "[RiftManager] ERROR: Failed to initialise Oculus SDK." << std::endl;
		return false;
	}

	// Create and grab a session object for the rift
	ovrGraphicsLuid luid;
	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result))
	{
		std::cout << "[RiftManager] ERROR: Failed to establish a session with a HDM." << std::endl;
		return false;
	}

	// Grab a headset description struct for the session
	hmdDesc = ovr_GetHmdDesc(session);

	// Configure what tracking features to enable
	result = ovr_ConfigureTracking(session, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);
	if (OVR_FAILURE(result))
	{
		std::cout << "[RiftManager] ERROR: Failed to configure tracking information." << std::endl;
		return false;
	}

	// Save resolution inforamtion for the headset
	resolution.w = hmdDesc.Resolution.w;
	resolution.h = hmdDesc.Resolution.h;

	return true;
}

bool RiftManager::InitBuffers()
{
	//Set up buffers for each eye
	for (int eye = 0; eye < NUM_EYES; eye++)
	{
		eyeBuffers[eye].eyeTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1.0f);
		ovrResult successType = ovr_CreateSwapTextureSetGL(session, GL_SRGB8_ALPHA8, eyeBuffers[eye].eyeTextureSize.w, eyeBuffers[eye].eyeTextureSize.h, &eyeBuffers[eye].textureSet);
		
		if (successType == ovrSuccess)
		{
			for (int i = 0; i < eyeBuffers[eye].textureSet->TextureCount; i++)
			{
				ovrGLTexture* tex = (ovrGLTexture*)&eyeBuffers[eye].textureSet->Textures[i];

				glBindTexture(GL_TEXTURE_2D, tex->OGL.TexId);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
		}
		else
		{
			std::cout << "[RiftManager] ERROR: failed creating swap texture set for eye num: " << eye << ". Return type = " << successType << std::endl;
			return false;
		}

		// Create eye framebuffer
		glGenFramebuffers(1, &eyeBuffers[eye].eyeFBO);

		// Create eye depth buffer
		glGenTextures(1, &eyeBuffers[eye].depthBuffer);
		glBindTexture(GL_TEXTURE_2D, eyeBuffers[eye].depthBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		GLenum internalFormat = GL_DEPTH_COMPONENT24;
		GLenum type = GL_UNSIGNED_INT;
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, eyeBuffers[eye].eyeTextureSize.w, eyeBuffers[eye].eyeTextureSize.h, 0, GL_DEPTH_COMPONENT, type, NULL);

		//Set static default head pose
		// Set Identity quaternion
		eyeRenderPose[eye].Orientation.x = 0.0f;
		eyeRenderPose[eye].Orientation.y = 0.0f;
		eyeRenderPose[eye].Orientation.z = 0.0f;
		eyeRenderPose[eye].Orientation.w = 1.0f;
		// Set World's origin position
		eyeRenderPose[eye].Position.x = 0.0f;
		eyeRenderPose[eye].Position.y = 0.0f;
		eyeRenderPose[eye].Position.z = 0.0f;
	}

	eyeRenderDesc[leftEye] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[leftEye]);
	eyeRenderDesc[rightEye] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[rightEye]);

	// Set up positional data of viewing plane
	viewOffset[leftEye] = eyeRenderDesc[leftEye].HmdToEyeViewOffset;
	viewOffset[rightEye] = eyeRenderDesc[rightEye].HmdToEyeViewOffset;

	viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
	viewScaleDesc.HmdToEyeViewOffset[leftEye] = viewOffset[leftEye];
	viewScaleDesc.HmdToEyeViewOffset[rightEye] = viewOffset[rightEye];

	// Set up layer
	ovrSizei bufferSize = GetBufferSize();

	eyeLayer.Header.Type = ovrLayerType_EyeFov;
	eyeLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft | ovrLayerFlag_HeadLocked;   // Because OpenGL.

	for (int eye = 0; eye < 2; ++eye)
	{
		eyeLayer.ColorTexture[eye] = eyeBuffers[eye].textureSet;
		eyeLayer.Viewport[eye].Pos.x = 0;
		eyeLayer.Viewport[eye].Pos.y = 0;
		eyeLayer.Viewport[eye].Size = eyeBuffers[eye].eyeTextureSize;
		eyeLayer.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
		eyeLayer.RenderPose[eye] = eyeRenderPose[eye];
		
	}
	eyeLayer.SensorSampleTime = sensorSampleTime;

	//Create mirror texture and FBO to see replication on computer monitor
	if (ovr_CreateMirrorTextureGL(session, GL_SRGB8_ALPHA8, resolution.w, resolution.h, reinterpret_cast<ovrTexture**>(&mirrorTexture)) != ovrSuccess)
	{
		std::cout << "[RiftManager] ERROR: failed creating mirror texture set" << std::endl;
		return false;
	}

	glGenFramebuffers(1, &mirrorFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTexture->OGL.TexId, 0);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	return true;
}

void RiftManager::BeginFrame()
{
	sensorSampleTime = ovr_GetTimeInSeconds();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0, 0, 0, 1);

	// In 3D rendering contexts the system would need to adjust the view based on the head pose
	//ovrTrackingState hmdState = ovr_GetTrackingState(session, ovr_GetPredictedDisplayTime(session, 0), ovrTrue);
	//ovr_CalcEyePoses(hmdState.HeadPose.ThePose, viewOffset, eyeRenderPose);
}

void RiftManager::BindFrameBuffer(EyeIndex eye)
{
	// Increment texture index
	eyeBuffers[eye].textureSet->CurrentIndex = (eyeBuffers[eye].textureSet->CurrentIndex + 1) % eyeBuffers[eye].textureSet->TextureCount;
	auto tex = reinterpret_cast<ovrGLTexture*>(&eyeBuffers[eye].textureSet->Textures[eyeBuffers[eye].textureSet->CurrentIndex]);

	// Bind eye FBO and Texture
	glBindFramebuffer(GL_FRAMEBUFFER, eyeBuffers[eye].eyeFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->OGL.TexId, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, eyeBuffers[eye].depthBuffer, 0);

}

void RiftManager::SetView(EyeIndex eye, float near, float far)
{
	// Set viewport location - always at (0,0) for static webcam view
	glViewport(0, 0, eyeBuffers[eye].eyeTextureSize.w, eyeBuffers[eye].eyeTextureSize.h);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_FRAMEBUFFER_SRGB);
	
	//3D projection matrix caluclations not needed for webcam rendering
	/*ovrMatrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], near, far, ovrProjection_RightHanded);
	glMatrixMode(GL_PROJECTION);
	glLoadTransposeMatrixf(proj.M[0]);*/
}

void RiftManager::UnBindFrameBuffer(EyeIndex eye)
{
	glBindFramebuffer(GL_FRAMEBUFFER, eyeBuffers[eye].eyeFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
}

void RiftManager::EndFrame()
{
	// Update sample time
	eyeLayer.SensorSampleTime = sensorSampleTime;

	// Update layer information
	ovrLayerHeader* layers = &eyeLayer.Header;
	if (ovr_SubmitFrame(session, 0, &viewScaleDesc, &layers, 1) != ovrSuccess)
	{
		std::cout << "[RiftManager] ERROR: failed submitting frame to oculus headset" << std::endl;
	}

	// Copy the frame to the mirror buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	GLint w = mirrorTexture->OGL.Header.TextureSize.w;
	GLint h = mirrorTexture->OGL.Header.TextureSize.h;
	glBlitFramebuffer(0, h, w, 0,
					  0, 0, w, h,
					  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void RiftManager::Destroy()
{
	// Delete all openGL items if they exist
	if (session)
	{
		if (glIsFramebuffer(mirrorFBO))
			glDeleteFramebuffers(1, &mirrorFBO);

		for (int eye = 0; eye < 2; eye++)
		{
			if (glIsTexture(eyeBuffers[eye].depthBuffer))
				glDeleteTextures(1, &eyeBuffers[eye].depthBuffer);

			if (glIsFramebuffer(eyeBuffers[eye].eyeFBO))
				glDeleteFramebuffers(1, &eyeBuffers[eye].eyeFBO);

			ovr_DestroySwapTextureSet(session, eyeBuffers[eye].textureSet);
		}
	}
}

ovrSizei RiftManager::GetResolution()
{
	return resolution;
}

float RiftManager::GetFoV()
{
	// Calculate the total FOV of the headset based on the individual FOV of each eye
	return (atanf(hmdDesc.DefaultEyeFov[0].LeftTan) + atanf(hmdDesc.DefaultEyeFov[0].RightTan));
}

ovrSizei RiftManager::GetBufferSize()
{
	// Calculate the total width and height of the headset based on individual eye configuration
	ovrSizei bufferSize;
	bufferSize.w = eyeBuffers[leftEye].eyeTextureSize.w + eyeBuffers[rightEye].eyeTextureSize.w;
	bufferSize.h = std::max(eyeBuffers[leftEye].eyeTextureSize.h, eyeBuffers[rightEye].eyeTextureSize.h);

	return bufferSize;
}


void RiftManager::GetPose(float &yaw, float &pitch, float &roll)
{
	float tempYaw, tempPitch, tempRoll;
	//Grab the current state of the Headset
	ovrTrackingState state = ovr_GetTrackingState(session, ovr_GetTimeInSeconds(), ovrFalse);
	OVR::Posef pose = state.HeadPose.ThePose;	
	pose.Rotation.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&tempYaw, &tempPitch, &tempRoll);

	//Return values in degrees
	yaw = OVR::RadToDegree(tempYaw);
	pitch = OVR::RadToDegree(tempPitch);
	roll = OVR::RadToDegree(tempRoll);
}

void RiftManager::RecenterPose()
{
	// Recenter headset
	ovr_RecenterPose(session);
}