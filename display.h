#pragma once

#include "OVR_CAPI.h"
#include <OVR_CAPI_GL.h>
#include <glew.h>
#include <glfw3.h>
#include <glm.hpp>

#include "webcamhandler.h"
#include "texture.h"
#include "shader.h"

class Display
{
public:
	Display(int width, int height);
	~Display();

	bool OnRender(unsigned int eye);
	void InitRender(float ovrFov, ovrSizei ovrTextSize);

	bool WindowSetup();
	
	GLFWwindow* GetWindow(){ return m_window; }

private:

	bool OpenWindow();

	//Window Handle
	GLFWwindow	*m_window;

	//Window Height
	int m_width;
	int m_height;

	GLuint rectangleVAO, rectangleVBO, rectangleEBO;
	
	ImageData frame;
	Texture eyeTextures[2];
	Shader shader;

	WebcamHandler webcamHandler;
};