#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glew.h> // Include glew to get all the required OpenGL headers

class Shader
{
public:
	// Shader Program Struct
	struct ShaderProgram
	{
		GLuint id;				// Shader program ID
		GLuint vertexShader;	// Vertex Shader ID
		GLuint fragmentShader;  // Fragment Shader ID
	};

	Shader();
	~Shader();

	// Use the program
	bool CreateShaderProgram(const GLchar* vertexPath, const GLchar* fragmentPath);
	void DestroyShaderProgram();

	// Use the shader program
	void Use();

private:

	ShaderProgram shaderProgram;

	bool CompileShader(GLuint &shaderID, GLenum shaderType, const char *shaderSrc);
	bool LinkShaders(GLuint& const programShader, const GLuint vertexShader, const GLuint fragmentShader);

	std::string ReadShaderFromFile(const char *filename);

};

