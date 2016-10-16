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
	DestroyRift();
}

bool RiftManager::InitRift()
{
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result))
	{
		std::cout << "[RiftManager] ERROR: Failed to initialise Oculus SDK." << std::endl;
		return false;
	}

	ovrGraphicsLuid luid;
	result = ovr_Create(&session, &luid);

	if (OVR_FAILURE(result))
	{
		std::cout << "[RiftManager] ERROR: Failed to establish a session with a HDM." << std::endl;
		return false;
	}

	hmdDesc = ovr_GetHmdDesc(session);

	result = ovr_ConfigureTracking(session, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);
	if (OVR_FAILURE(result))
	{
		std::cout << "[RiftManager] ERROR: Failed to configure tracking information." << std::endl;
		return false;
	}

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

		if (ovr_CreateSwapTextureSetGL(session, GL_SRGB8_ALPHA8, eyeBuffers[eye].eyeTextureSize.w, eyeBuffers[eye].eyeTextureSize.h, &eyeBuffers[eye].textureSet) == ovrSuccess)
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
			std::cout << "[RiftManager] ERROR: failed creating swap texture set for eye num: " << eye << std::endl;
			return false;
		}

		glGenFramebuffers(1, &eyeBuffers[eye].eyeFBO);

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

	// Set up positional data.
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
		//eyeLayer.Viewport[eye] = OVR::Recti(eye == ovrEye_Left ? 0 : bufferSize.w / 2, 0, bufferSize.w / 2, bufferSize.h);
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

	GetPose();
	//ovrTrackingState hmdState = ovr_GetTrackingState(session, ovr_GetPredictedDisplayTime(session, 0), ovrTrue);
	//ovr_CalcEyePoses(hmdState.HeadPose.ThePose, viewOffset, eyeRenderPose);
}

void RiftManager::BindFrameBuffer(EyeIndex eye)
{
	eyeBuffers[eye].textureSet->CurrentIndex = (eyeBuffers[eye].textureSet->CurrentIndex + 1) % eyeBuffers[eye].textureSet->TextureCount;
	auto tex = reinterpret_cast<ovrGLTexture*>(&eyeBuffers[eye].textureSet->Textures[eyeBuffers[eye].textureSet->CurrentIndex]);

	glBindFramebuffer(GL_FRAMEBUFFER, eyeBuffers[eye].eyeFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->OGL.TexId, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, eyeBuffers[eye].depthBuffer, 0);

}

void RiftManager::SetView(EyeIndex eye, float near, float far)
{
	glViewport(0, 0, eyeBuffers[eye].eyeTextureSize.w, eyeBuffers[eye].eyeTextureSize.h);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_FRAMEBUFFER_SRGB);

	ovrMatrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], near, far, ovrProjection_RightHanded);

	glMatrixMode(GL_PROJECTION);
	glLoadTransposeMatrixf(proj.M[0]);
}

void RiftManager::UnBindFrameBuffer(EyeIndex eye)
{
	glBindFramebuffer(GL_FRAMEBUFFER, eyeBuffers[eye].eyeFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
}

void RiftManager::EndFrame()
{
	eyeLayer.SensorSampleTime = sensorSampleTime;

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

void RiftManager::DestroyRift()
{
	ovr_Destroy(session);
	ovr_Shutdown();
}

const ovrSizei RiftManager::GetResolution()
{
	return resolution;
}

float RiftManager::GetFoV()
{
	return (atanf(hmdDesc.DefaultEyeFov[0].LeftTan) + atanf(hmdDesc.DefaultEyeFov[0].RightTan));
}

ovrSizei RiftManager::GetBufferSize()
{
	ovrSizei bufferSize;
	bufferSize.w = eyeBuffers[leftEye].eyeTextureSize.w + eyeBuffers[rightEye].eyeTextureSize.w;
	bufferSize.h = std::max(eyeBuffers[leftEye].eyeTextureSize.h, eyeBuffers[rightEye].eyeTextureSize.h);

	return bufferSize;
}

//float &x, float &y, float &z
void RiftManager::GetPose()
{
	//Grab the current state of the Headset
	ovrTrackingState state = ovr_GetTrackingState(session, ovr_GetTimeInSeconds(), ovrFalse);
	OVR::Posef pose = state.HeadPose.ThePose;

	float yaw, pitch, roll;
	float digiYaw, digiPitch, digiRoll;
	pose.Rotation.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&yaw, &pitch, &roll);

	/*cout << "Yaw: " << fixed << setprecision(2) << RadToDegree(yaw) <<
	" Pitch: " << fixed << setprecision(2) << RadToDegree(pitch) <<
	" Roll: " << fixed << setprecision(2) << RadToDegree(roll) << endl;*/

	std::stringstream ss;
	std::string message;

	/*if (yaw < 0)
	{
	digiYaw = RadToDegree(yaw) + 360.0;
	}
	else
	{
	digiYaw = RadToDegree(yaw);
	} */

	digiYaw = OVR::RadToDegree(yaw);
	digiYaw += 150.0;

	if (digiYaw > 300)
		digiYaw = 300;

	if (digiYaw < 0)
		digiYaw = 0;

	digiPitch = OVR::RadToDegree(pitch);
	digiPitch += 150.0;


	digiYaw = round(digiYaw / DEGREES_TO_DIGI);
	digiPitch = round(digiPitch / DEGREES_TO_DIGI);
	digiRoll = round(OVR::RadToDegree(roll) / DEGREES_TO_DIGI);


	//ss << "^yaw:" << setprecision(4) << RadToDegree(yaw) << "pitch:" << setprecision(4) << RadToDegree(pitch) << "roll:" << setprecision(4) << RadToDegree(roll) << "#";
	ss << "^" << digiYaw << "," << digiPitch << "#";

	std::cout << ss.str() << std::endl;
}