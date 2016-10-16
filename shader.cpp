#include "Shader.h"

Shader::Shader()
{}

Shader::~Shader()
{
	DestroyShaderProgram();
}

void Shader::Use()
{
	// If the program exists, use it
	if (glIsProgram(shaderProgram.id))
		glUseProgram(this->shaderProgram.id);
}

bool Shader::CreateShaderProgram(const GLchar* vertexPath, const GLchar* fragmentPath)
{
	// Read the vertex and fragment shaders from file
	std::string vertexCode = ReadShaderFromFile(vertexPath);
	std::string fragmentCode = ReadShaderFromFile(fragmentPath);

	// Check we managed to get a valid string
	if (vertexCode.empty() || fragmentCode.empty())
	{
		std::cout << "[Shader] ERROR: Empty shader file!" << std::endl;DestroyShaderProgram();
		DestroyShaderProgram();
		return false;
	}

	// Check we compiled shaders correctly
	bool success = CompileShader(shaderProgram.vertexShader, GL_VERTEX_SHADER, vertexCode.c_str()) &&
		CompileShader(shaderProgram.fragmentShader, GL_FRAGMENT_SHADER, fragmentCode.c_str());
	if (!success)
	{
		std::cout << "[Shader] ERROR: Shader compilation error!" << std::endl;
		DestroyShaderProgram();
		return false;
	}

	// Check we were able to link program correctly
	success = LinkShaders(shaderProgram.id, shaderProgram.vertexShader, shaderProgram.fragmentShader);
	if (!success)
	{
		std::cout << "[Shader] ERROR: Shader linking error!" << std::endl;
		DestroyShaderProgram();
		return false;
	}

	return true;
}

void Shader::DestroyShaderProgram()
{
	// Find the program and corresponding shaders by ID and delete them if they exist
	if (glIsProgram(shaderProgram.id))
		glDeleteProgram(shaderProgram.id);

	if (glIsShader(shaderProgram.vertexShader))
		glDeleteShader(shaderProgram.vertexShader);

	if (glIsShader(shaderProgram.fragmentShader))
		glDeleteShader(shaderProgram.fragmentShader);
}

bool Shader::CompileShader(GLuint &shaderID, GLenum shaderType, const char *shaderSrc)
{
	// Create and compile shader
	shaderID = glCreateShader(shaderType);

	glShaderSource(shaderID, 1, &shaderSrc, NULL);
	glCompileShader(shaderID);

	// Check Compilation Succeeded
	GLint compileSuccess;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileSuccess);
	if (!compileSuccess)
	{
		GLchar infoLog[512];
		glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
		std::cout << "[Shader] ERROR: Could not compile shader! Error log returned: " << std::endl << infoLog << std::endl;

		glDeleteShader(shaderID);

		return false;
	}

	return true;
}

bool Shader::LinkShaders(GLuint& const shaderProgram, const GLuint vertexShader, const GLuint fragmentShader)
{
	// Create program and link the vertex and fragment shaders into them
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	//Check Linking Succeeded
	GLint linkSuccess;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkSuccess);
	if (!linkSuccess)
	{
		GLchar infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "[Shader] ERROR: Could not link shader program! Error log returned: " << std::endl << infoLog << std::endl;

		DestroyShaderProgram();

		return false;
	}

	// Delete shaders now that they have been linked into a program
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return true;
}

std::string Shader::ReadShaderFromFile(const char *filename)
{
	// Open the specified shader file
	std::ifstream shaderFile(filename);

	if (!shaderFile.is_open())
	{
		std::cout << "[Shader] ERROR: Cannot open shader file: " << filename << std::endl;
		return NULL;
	}

	// Read shader file contents into string stream and then close file
	std::stringstream shaderStream;
	shaderStream << shaderFile.rdbuf();
	shaderFile.close();

	// Extract and return a string from the string stream
	return shaderStream.str();
}
