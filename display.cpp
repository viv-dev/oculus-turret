#include "display.h"
#include <iostream>
#include <math.h>

#define CAMERA_HFOV_DEGREES 70.42f
#define CAMERA_HFOV_RADIANS 1.2290f

Display::Display(int width, int height)
{
	m_width = width;
	m_height = height;
}

Display::~Display()
{
	glfwDestroyWindow(m_window);
}

void Display::InitRender(float ovrFov, ovrSizei ovrBufferSize)
{
	//Set the size of the rendering window (doesn't have to match the size of the context window)
	webcamHandler.startCapture();
	
	if(!webcamHandler.getFrame(frame))
	{
		std::cout << "[Display] Could not grab webcam frame!" << std::endl;
	}

	ovrSizei webcamResolution = webcamHandler.getImageResolution();

	eyeTextures[0].LoadWebcamImage(frame.image.data, webcamResolution.w, webcamResolution.h);
	eyeTextures[1].LoadWebcamImage(frame.image.data, webcamResolution.w, webcamResolution.h);

	shader.CreateShaderProgram("shaders/rect-textured.vs", "shaders/rect-textured.fs");

	std::cout << "ovrFov: " << ovrFov << " ovrBufferSize: " << ovrBufferSize.w << "," << ovrBufferSize.h << std::endl;

	//Calculate appropriate values for rendering plane based on camera FoV and resolution
	unsigned int useableWidth = webcamResolution.w * ovrFov / CAMERA_HFOV_RADIANS;

	float widthFinal = ovrBufferSize.w / 2;
	float heightFinal = webcamResolution.h * widthFinal / useableWidth;

	float heightGL = ((heightFinal) / (float)(ovrBufferSize.h));
	float widthGL = ((webcamResolution.w * (heightFinal / (float)webcamResolution.h)) / (float)widthFinal);

	std::cout << "widthGL: " << widthGL << " heightGL: " << heightGL << std::endl;

	//Array of verticies that represent the four points of our rectangle
	GLfloat rectangle_verticies[] =
	{
		// Positions          // Colors           // Texture Coords
		widthGL,   heightGL, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // Top Right
		widthGL,  -heightGL, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // Bottom Right
		-widthGL, -heightGL, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // Bottom Left
		-widthGL,  heightGL, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // Top Left 
	};

	/*GLfloat rectangle_verticies[] =
	{
		// Positions          // Colors           // Texture Coords
		1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // Top Right
		1.0f,  -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // Bottom Right
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // Bottom Left
		-1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // Top Left 
	};*/

	GLuint rectangle_indices[] =
	{
		0,1,3,	//First triangle
		1,2,3	//Second triangle
	};

	glGenVertexArrays(1, &rectangleVAO);	//Create Vertex Array Object (VAO)
	glGenBuffers(1, &rectangleVBO);			//Create Vertex Buffer Object (VBO)
	glGenBuffers(1, &rectangleEBO);			//Create Element Buffer Object (EBO)

	//Bind VAO first and then set Vertex Buffers and Attribute Pointers
	glBindVertexArray(rectangleVAO);

	//Bind VBO object and configure buffer with verticies information
	glBindBuffer(GL_ARRAY_BUFFER, rectangleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_verticies), rectangle_verticies, GL_STATIC_DRAW);

	//Bind EBO object and configure buffer with indices information
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectangleEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectangle_indices), rectangle_indices, GL_STATIC_DRAW);

	//Define how to interpret the VBO in the VAO
	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// TexCoord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	//Unbind VAO and VBO to avoid accidental reconfiguration
	glBindBuffer(GL_ARRAY_BUFFER, 0); //Unbind VBO
	glBindVertexArray(0); //Unbind VAO

	
}

bool Display::OnRender(unsigned int eye)
{
	shader.Use();

	if(!eyeTextures[eye].Bind())
		return false;	

	webcamHandler.getFrame(frame);
	eyeTextures[eye].Update(frame.image.data);

	glBindVertexArray(rectangleVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	return true;
}

bool Display::WindowSetup()
{
	//Define Window Paramaters before calling glfwCreateWindow
	if (!glfwInit())
	{
		std::cout << "[Display] GLFW failed to initialise" << std::endl;
		return false;
	}

	//Set window properties
	glfwWindowHint(GLFW_DEPTH_BITS, 16);							//Colour bit depth
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);					//OpenGL Major version 3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);					//OpenGL Minor version 3
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);	//Specify the OPENGL profile
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);						//Make the window non-resizeable

	return OpenWindow();
}

bool Display::OpenWindow()
{
	m_window = glfwCreateWindow(m_width, m_height, "OculusTurret", nullptr, nullptr);

	if (!m_window)
	{
		std::cout << "[Display] Unable to create rendering window" << std::endl;
		glfwTerminate();
		false;
	}

	glfwMakeContextCurrent(m_window);
	glfwSwapInterval(0);

	glewExperimental = GL_TRUE; //Needed in core profile for it to work

	if (glewInit() != GLEW_OK)
	{
		std::cout << "[Display] GLEW failed to initialise" << std::endl;
		glfwTerminate();
		return false;
	}

	return true;
}


	/*glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, MouseMoveCallback);
	glfwSetCursorEnterCallback(window, CursorEnterCallback);
	glfwSetCharCallback(window, CharacterCallback);
	glfwSetScrollCallback(window, ScrollCallback);*/