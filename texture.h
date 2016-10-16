#pragma once

#include <map>
#include <string>
#include <iostream>

#include <glew.h>

class Texture
{
public:
	Texture();
	~Texture();

	void LoadWebcamImage(unsigned char *image, int width, int height);
	void Update(unsigned char *image);

	bool LoadWebcamTexture();
	void DeleteTexture();

	bool Bind();
	void UnBind();

	int getWidth() { return width; }
	int getHeight() { return height; }

private:

	GLuint textureID;

	int width;
	int height;
	
	unsigned char *image;

};