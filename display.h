#pragma once

#include "OVR_CAPI.h"
#include <OVR_CAPI_GL.h>
#include <glew.h>
#include <glfw3.h>
#include <glm.hpp>

#include "webcammanager.h"
#include "texture.h"
#include "shader.h"

class Display
{
public:
	Display(int width, int height);
	~Display();

	bool OnRender(unsigned int eye);
	bool InitRender(float ovrFov, ovrSizei ovrTextSize, float maxWidth, float maxHeight);
	bool WindowSetup();
	void Destroy();
	
	GLFWwindow* GetWindow(){ return m_window; }

private:

	bool OpenWindow();

	//Window Handle
	GLFWwindow	*m_window;

	//Window Dimensions
	int windowWidth;
	int windowHeight;

	GLuint rectangleVAO, rectangleVBO, rectangleEBO;
	
	cv::Mat frame;
	Texture eyeTextures[2];
	Shader shader;

	WebCamManager webcamManager;
};