// Assignment2_OpenGL.cpp : Defines the entry point for the console application.
//

#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <IL/il.h>

#include "shader_helper.h"

//window client area size

const int ClientAreaWidth = 800;
const int ClientAreaHeight = 800;

//global variables

GLFWwindow *window;
GLuint gpuProgram;
GLuint vertexBuffer, vertexArray;
GLuint floorTexture;

//locations of uniform variables in the GPU program
GLuint locPlaneTex;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

bool InitWindow();

bool LoadTextures();
bool LoadShaders();
void CreateSquare();
void InitUniforms();
void InitInputAssembler();
void InitRasterizer();
void InitPerSampleProcessing();

void MainLoop();
void Render();
void Cleanup();

int main()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to load GLFW!" << std::endl;
		return 1;
	}

	if (!InitWindow())
	{
		std::cerr << "Failed to create window!" << std::endl;
		Cleanup();
		return 1;
	}

	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to load GLEW!" << std::endl;
		Cleanup();
		return 1;
	}

	//init DevIL
	ilInit();

	if (!LoadTextures())
	{
		Cleanup();
		return 1;
	}

	if (!LoadShaders())
	{
		std::cerr << "Failed to load shaders!" << std::endl;
		Cleanup();
		return 1;
	}

	CreateSquare();
	InitUniforms();
	InitInputAssembler();
	InitRasterizer();
	InitPerSampleProcessing();

	MainLoop();

	Cleanup();

	return 0;
}

bool InitWindow()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(ClientAreaWidth, ClientAreaHeight, "Assignment 2 - Raytracing", nullptr, nullptr);

	if (!window)
		return false;

	//set the window's OpenGL context as the current OpenGL context

	glfwMakeContextCurrent(window);

	//set event handlers for the window

	glfwSetKeyCallback(window, key_callback);

	//the parameter 1 means VSync is enabled
	//change to 0 to disable VSync

	glfwSwapInterval(1);

	return true;
}

bool LoadTextures()
{
	ILuint imageId;
	ilGenImages(1, &imageId);
	ilBindImage(imageId);

	if (!ilLoadImage("wood_floor.bmp"))
	{
		std::cerr << "Unable to open wood_floor.bmp" << std::endl;
		return false;
	}

	//this is a GIF image, which uses a palette,
	//so we need to convert it to RGBA
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	//create the texture and store data into it

	glGenTextures(1, &floorTexture);
	glBindTexture(GL_TEXTURE_2D, floorTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),
		0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());

	ilDeleteImages(1, &imageId);

	//configure the sampling method for the texture

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return true;
}

bool LoadShaders()
{
	//load shaders

	GLenum vertexShader, fragmentShader;

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	bool success = false;

	if (try_compile_shader_from_file(vertexShader, "VertexShader.glsl") &&
		try_compile_shader_from_file(fragmentShader, "FragmentShader.glsl"))
	{
		gpuProgram = glCreateProgram();

		glAttachShader(gpuProgram, vertexShader);
		glAttachShader(gpuProgram, fragmentShader);

		success = try_link_program(gpuProgram);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return success;
}

void CreateSquare()
{
	//each vertex is 2 floats for the position

	float vertexBufferData[] = {
		-1.0f, +1.0f,
		-1.0f, -1.0f,
		+1.0f, +1.0f,
		+1.0f, -1.0f
	};

	//this is going to be a triangle strip, so we don't need an index buffer

	//create vertex buffer and store data into it

	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	glBufferStorage(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, 0);
}

void InitUniforms()
{
	locPlaneTex = glGetUniformLocation(gpuProgram, "planeTex");

	glProgramUniform1i(gpuProgram, locPlaneTex, 0);//use texture slot 0
}

void InitInputAssembler()
{
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

						//index		num_components	type		normalize?	stride				offset
	glVertexAttribPointer(0,		2,				GL_FLOAT,	GL_FALSE,	sizeof(float[2]), (void*)0);
	glEnableVertexAttribArray(0);
}

void InitRasterizer()
{
	glViewport(0, 0, ClientAreaWidth, ClientAreaHeight);
	glEnable(GL_CULL_FACE);
}

void InitPerSampleProcessing()
{
	glClearColor(0.6f, 0.7f, 0.9f, 1.0f);
	glEnable(GL_DEPTH_TEST);
}

void Cleanup()
{
	glDeleteProgram(gpuProgram);
	glDeleteVertexArrays(1, &vertexArray);
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteTextures(1, &floorTexture);

	ilShutDown();
	glfwTerminate();
}

void Render()
{
	//clear the depth and color buffers

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//bind whatever needs to be bound

	glBindVertexArray(vertexArray);
	glUseProgram(gpuProgram);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, floorTexture);

	//draw the triangles

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	//swap the back and front buffers

	glfwSwapBuffers(window);
}

void MainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		Render();
	}
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, true);
	}
}