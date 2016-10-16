#include "texture.h"

Texture::Texture()
{
}

Texture::~Texture()
{
	DeleteTexture();
}

void Texture::LoadWebcamImage(unsigned char *image, int width, int height)
{
	this->image = image;
	this->width = width;
	this->height = height;
	LoadWebcamTexture();
}

bool Texture::LoadWebcamTexture()
{
	if (image == nullptr)
	{
		std::cout << "[Texture] ERROR: Trying to load null texture!" << std::endl;
		return false;
	}

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, image);

	if (glIsTexture(textureID))
		std::cout << "[Texture] Texture bound successfully!" << std::endl;

	glGenerateMipmap(GL_TEXTURE_2D);

	// Unbind texture when done, so we won't accidentily mess up our texture.
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

bool Texture::Bind()
{
	if (glIsTexture(textureID))
	{
		glBindTexture(GL_TEXTURE_2D, textureID);
		return true;
	}
	else
	{
		std::cout << "[Texture] ERROR: Trying to bind invalid texture!" << std::endl;
		return false;
	}
}

void Texture::UnBind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Update(unsigned char *image)
{
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, image);
}

void Texture::DeleteTexture()
{
	if (glIsTexture(textureID))
		glDeleteTextures(1, &textureID);
}


