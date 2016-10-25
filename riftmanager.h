#pragma once

#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"
#include "Extras\OVR_Math.h"

#include <glew.h>

enum EyeIndex
{
	leftEye = 0,
	rightEye,
	NUM_EYES
};

class RiftManager
{
public:
	RiftManager();
	~RiftManager();

	bool InitRift();
	bool InitBuffers();
	

	void BeginFrame();
	
	void SetView(EyeIndex eye, float near, float far);
	void BindFrameBuffer(EyeIndex eye);
	void UnBindFrameBuffer(EyeIndex eye);

	void EndFrame();

	void Destroy();

	ovrSizei GetResolution();
	ovrSizei GetBufferSize();
	float GetFoV();
	void GetPose(float &yaw, float &pitch, float &roll);
	void RecenterPose();

private:

	struct OvrEyeBuffer
	{
		// Eye Reccomended Texture Size 
		ovrSizei eyeTextureSize;
		ovrSizei eyeBufferSize;

		//Eye FBO and Depth buffer
		GLuint eyeFBO;
		GLuint depthBuffer;

		// Eye Texture Set
		ovrSwapTextureSet *textureSet;

		OvrEyeBuffer():
			textureSet(nullptr)
			{}
	};

	// Oculus HDM information
	ovrSession session;
	ovrHmdDesc hmdDesc;

	// Display resolution
	ovrSizei resolution;

	// Eye rendering information
	ovrLayerEyeFov eyeLayer;
	OvrEyeBuffer eyeBuffers[NUM_EYES];
	ovrVector3f viewOffset[NUM_EYES];
	ovrEyeRenderDesc eyeRenderDesc[NUM_EYES];
	ovrPosef eyeRenderPose[NUM_EYES];
	ovrViewScaleDesc viewScaleDesc;

	// Used for tracking prediction
	double sensorSampleTime;

	// Mirror texture and FBO used to render HMD view to OpenGL window
	ovrGLTexture *mirrorTexture;
	GLuint mirrorFBO;
	
};