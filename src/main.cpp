/*
CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <glad/glad.h>
#include <iterator>
#include <list>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"

#include "WindowManager.h"
#include "Shape.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;
shared_ptr<Shape> shape;


double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}

class controller
{
public:
	int up, down, left, right, x;

	int removeX;
	int removeY;
};

controller cont;


static const float H_SPEED = 12.5f;
static const float V_SPEED = 16;
static const float TERMINAL_VELOCITY = 30;
static const float H_ACCEL = 10;
static const float V_ACCEL = 30;



class character
{
public:
	glm::vec3 pos, velocity, acceleration;

	float maxspeed, check;
	float animationspeed;

	int jumping, collideLeft, collideUp, collideDown, collideRight;

	character()
	{
		velocity = acceleration = glm::vec3(0, 0, 0);
		pos = glm::vec3(-5, -6, 0);
		maxspeed = check = 0;
		jumping = collideLeft = collideRight = collideUp = collideDown = 0;
		animationspeed = 0;
	}

	void process(double ftime)
	{

		std::cout << " YVELOCITY " << velocity.y << endl;

		animationspeed = abs(velocity.x * ftime);
		
		
		if (collideDown) {
			acceleration.y = 0;
			velocity.y = 0;
			jumping = 0;
			pos.y = round(pos.y);

			if (!jumping && cont.up) {
				acceleration.y = V_ACCEL;
				velocity.y = -1 * V_SPEED;
				jumping = 1;
			}
		}
		else {
			acceleration.y = V_ACCEL;
		}


		if (velocity.y != 0 && jumping) {
			//cout << "TRUE" << endl;
			if (velocity.y > TERMINAL_VELOCITY) {
				velocity.y = TERMINAL_VELOCITY;
				acceleration.y = 0;
			}
		}
		

		/*
		if (cont.up) {
			velocity.y = -0.2 * V_SPEED;
		}
		else if (cont.down) {
			velocity.y = 0.2 * V_SPEED;
		}
		else {
			velocity.y = 0;
		}
		*/

		if (!collideLeft && cont.left && velocity.x < H_SPEED) {
			//velocity.x = H_SPEED * ftime;
			acceleration.x = H_ACCEL;
			//cout << "TRUE ";
		}

		if (!collideRight && cont.right && velocity.x > -1 * H_SPEED) {
			//velocity.x = -1 * H_SPEED * ftime;
			acceleration.x = -1 * H_ACCEL;
		}

		float hspeedTemp = H_SPEED;
		velocity.x = clamp(velocity.x, float(-hspeedTemp), float(hspeedTemp));

		if (!cont.right && !cont.left) {
			if (abs(velocity.x) < 0.5) {
				velocity.x = 0;
				acceleration.x = 0;
			}
			else {
				acceleration.x = H_ACCEL * -2.5 * (velocity.x / abs(velocity.x));

				if (check != 0 && abs(velocity.x - check) > abs(check)) {
					acceleration.x = 0;
					velocity.x = 0;
				}
				check = velocity.x;
			}
		}
		else {
			check = 0;
		}


		if (collideLeft) {
			acceleration.x = clamp(acceleration.x, -100.f, 0.f);
			velocity.x = clamp(velocity.x, -100.f, 0.f);
			pos.x = round(pos.x);
		}
		if (collideRight) {
			acceleration.x = clamp(acceleration.x, 0.f, 100.f);
			velocity.x = clamp(velocity.x, 0.f, 100.f);
			pos.x = round(pos.x);
		}
		if (collideDown && jumping && velocity.y > 0) {
			acceleration.y = clamp(acceleration.y, 0.f, -100.f);
			velocity.y = clamp(velocity.y, 0.f, -100.f);
			pos.y = round(pos.y);
		}

		velocity += float(ftime) * acceleration;


		pos += 0.5f * velocity * float(ftime);

		if (abs(pos.x) > maxspeed) maxspeed = abs(pos.x);

		


	}
};

character mychar = *(new character());




class camera
{
public:
	glm::vec3 pos, rot;

	camera()
	{
		pos = rot = glm::vec3(0, 0, 0);
	}
	glm::mat4 process(double ftime)
	{
		float speed = 0;
		float yspeed = 0;


		if (cont.left == 1)
			speed = H_SPEED * ftime;
		else if (cont.right == 1)
			speed = -H_SPEED * ftime;

		if (cont.up == 1)
			yspeed = -8 * ftime;
		else if (cont.down == 1)
			yspeed = 8 * ftime;


		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::mat4 R2 = glm::rotate(glm::mat4(1), rot.z, glm::vec3(1, 0, 0));
		glm::vec4 dir = glm::vec4(speed, yspeed, 0, 1);
		dir = dir * R;
		pos += glm::vec3(dir.x, dir.y, dir.z);

		pos = mychar.pos;

		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		return R2 * R * T;
	}
};

camera mycam;







class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog,psky,cloudshader;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our box to OpenGL
	GLuint VertexBufferID, VertexNormDBox, VertexTexBox, IndexBufferIDBox, VertexColorIDBox;

	//texture data
	GLuint TextureNormal, TextureNight;
	GLuint TextureGrass, TextureClouds, TextureBackground, TextureVegetation;


	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			cont.up = 1;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			cont.up = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			cont.down = 1;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			cont.down = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			cont.left = 1;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			cont.left = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			cont.right = 1;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			cont.right = 0;
		}

		if (key == GLFW_KEY_UP && action == GLFW_PRESS)
		{
			cont.up = 1;
		}
		if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
		{
			cont.up = 0;
		}
		if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
		{
			cont.down = 1;
		}
		if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
		{
			cont.down = 0;
		}
		if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
		{
			cont.left = 1;
		}
		if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
		{
			cont.left = 0;
		}
		if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
		{
			cont.right = 1;
		}
		if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE)
		{
			cont.right = 0;
		}

		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		{
			cont.up = 1;
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
		{
			cont.up = 0;
		}

		if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
		{
			cont.down = 1;
		}
		if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
		{
			cont.down = 0;
		}

		if (key == GLFW_KEY_X && action == GLFW_PRESS)
		{
			cont.x = 1;
		}
		if (key == GLFW_KEY_X && action == GLFW_RELEASE)
		{
			cont.x = 0;
		}
	}

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;
		float newPt[2];
		if (action == GLFW_PRESS)
		{
			/*glfwGetCursorPos(window, &posX, &posY);
			std::cout << "Pos X " << posX <<  " Pos Y " << posY << std::endl;

			//change this to be the points converted to WORLD
			//THIS IS BROKEN< YOU GET TO FIX IT - yay!
			newPt[0] = 0;
			newPt[1] = 0;

			std::cout << "converted:" << newPt[0] << " " << newPt[1] << std::endl;
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
			//update the vertex array with the updated points
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*6, sizeof(float)*2, newPt);
			glBindBuffer(GL_ARRAY_BUFFER, 0);*/
		}
	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}

	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom()
	{
		//generate the VAO
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferID);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);

		GLfloat rect_vertices[] = {
			// front
			-1.0, -1.0,  1.0,
			1.0, -1.0,  1.0,
			1.0,  1.0,  1.0,
			-1.0,  1.0,  1.0,
			// back
			-1.0, -1.0, -1.0,
			1.0, -1.0, -1.0,
			1.0,  1.0, -1.0,
			-1.0,  1.0, -1.0,
			//tube 8 - 11
			-1.0, -1.0,  1.0,
			1.0, -1.0,  1.0,
			1.0,  1.0,  1.0,
			-1.0,  1.0,  1.0,
			//12 - 15
			-1.0, -1.0, -1.0,
			1.0, -1.0, -1.0,
			1.0,  1.0, -1.0,
			-1.0,  1.0, -1.0
		};
		//make it a bit smaller
		for (int i = 0; i < 48; i++)
			rect_vertices[i] *= 0.5;
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, sizeof(rect_vertices), rect_vertices, GL_DYNAMIC_DRAW);

		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//color
		GLfloat cube_norm[] = {
			// front colors
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,

			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,

			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,

			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,

		};
		glGenBuffers(1, &VertexNormDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexNormDBox);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_norm), cube_norm, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//color
		glm::vec2 cube_tex[] = {
			// front colors
			glm::vec2(0.0, 2.0),
			glm::vec2(2.0, 2.0),
			glm::vec2(2.0, 0.0),
			glm::vec2(0.0, 0.0),

			glm::vec2(0.0, 2.0),
			glm::vec2(2.0, 2.0),
			glm::vec2(2.0, 0.0),
			glm::vec2(0.0, 0.0),

			glm::vec2(0.0, 2.0),
			glm::vec2(2.0, 2.0),
			glm::vec2(2.0, 0.0),
			glm::vec2(0.0, 0.0),

			glm::vec2(0.0, 2.0),
			glm::vec2(2.0, 2.0),
			glm::vec2(2.0, 0.0),
			glm::vec2(0.0, 0.0),

		};
		glGenBuffers(1, &VertexTexBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexTexBox);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex), cube_tex, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &IndexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		GLushort cube_elements[] = {
			// front
			0, 1, 2,
			2, 3, 0,
			// back
			7, 6, 5,
			5, 4, 7,
			//tube 8-11, 12-15
			8,12,13,
			8,13,9,
			9,13,14,
			9,14,10,
			10,14,15,
			10,15,11,
			11,15,12,
			11,12,8
		};
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);


		//color
		GLfloat cube_colors[] = {
			// front colors
			1.0, 0.0, 0.5,
			1.0, 0.0, 0.5,
			1.0, 0.0, 0.5,
			1.0, 0.0, 0.5,
			// back colors
			0.5, 0.5, 0.0,
			0.5, 0.5, 0.0,
			0.5, 0.5, 0.0,
			0.5, 0.5, 0.0,
			// tube colors
			0.0, 1.0, 1.0,
			0.0, 1.0, 1.0,
			0.0, 1.0, 1.0,
			0.0, 1.0, 1.0,
			0.0, 1.0, 1.0,
			0.0, 1.0, 1.0,
			0.0, 1.0, 1.0,
			0.0, 1.0, 1.0,
		};
		glGenBuffers(1, &VertexColorIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexColorIDBox);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_colors), cube_colors, GL_STATIC_DRAW);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


		glBindVertexArray(0);

		string resourceDirectory = "../resources" ;
		// Initialize mesh.
		shape = make_shared<Shape>();
		//shape->loadMesh(resourceDirectory + "/t800.obj");
		shape->loadMesh(resourceDirectory + "/sphere.obj");
		shape->resize();
		shape->init();

		int width, height, channels;
		char filepath[1000];

		//texture 1
		string str = resourceDirectory + "/hills.png";
		strcpy(filepath, str.c_str());
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &TextureGrass);
		//glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureGrass);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//texture Night
		str = resourceDirectory + "/sky2.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &TextureNight);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureNight);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);


		//texture 2
		//str = resourceDirectory + "/normalMap.jpg";
		str = resourceDirectory + "/normTextures.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &TextureNormal);
		glActiveTexture(GL_TEXTURE1); 
		glBindTexture(GL_TEXTURE_2D, TextureNormal);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);



		//cloud
		//str = resourceDirectory + "/clouds.jpg";
		str = resourceDirectory + "/blockTextures3.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &TextureClouds);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureClouds);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);


		str = resourceDirectory + "/blockTextures.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &TextureBackground);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureBackground);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);


		str = resourceDirectory + "/vegetation.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &TextureVegetation);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureVegetation);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);



		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex");//tex, tex2... sampler in the fragment shader
		GLuint Tex2Location = glGetUniformLocation(prog->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(prog->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

		Tex1Location = glGetUniformLocation(psky->pid, "tex");//tex, tex2... sampler in the fragment shader
		Tex2Location = glGetUniformLocation(psky->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(psky->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

		Tex1Location = glGetUniformLocation(cloudshader->pid, "tex");//tex, tex2... sampler in the fragment shader
		Tex2Location = glGetUniformLocation(cloudshader->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(cloudshader->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

	}

	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);
		//glDisable(GL_DEPTH_TEST);
		// Initialize the GLSL program.
		prog = std::make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
		if (!prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("setColor");
		prog->addUniform("campos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");


		psky = std::make_shared<Program>();
		psky->setVerbose(true);
		psky->setShaderNames(resourceDirectory + "/skyvertex.glsl", resourceDirectory + "/skyfrag.glsl");
		if (!psky->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		psky->addUniform("dn");
		psky->addUniform("P");
		psky->addUniform("V");
		psky->addUniform("M");
		psky->addUniform("campos");
		psky->addAttribute("vertPos");
		psky->addAttribute("vertNor");
		psky->addAttribute("vertTex");


		cloudshader = std::make_shared<Program>();
		cloudshader->setVerbose(true);
		cloudshader->setShaderNames(resourceDirectory + "/cloudvertex.glsl", resourceDirectory + "/cloudfrag.glsl");
		if (!cloudshader->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		cloudshader->addUniform("dn");
		cloudshader->addUniform("P");
		cloudshader->addUniform("V");
		cloudshader->addUniform("M");
		cloudshader->addUniform("campos");
		cloudshader->addUniform("dontrender");
		cloudshader->addUniform("texoff");
		cloudshader->addAttribute("vertPos");
		cloudshader->addAttribute("vertNor");
		cloudshader->addAttribute("vertTex");
		cloudshader->addAttribute("vertCol");


	}

	


	class cloudData {
	public:
		mat4 M;
		float zpos;
		vec2 tex;
		vec2 pos;
	};


	
	struct comparething {
		typedef cloudData first;
		typedef cloudData second;

		_NODISCARD constexpr bool operator()(const cloudData& first, const cloudData& second) const {
			return (first.zpos > second.zpos);
		}
	};
	


	struct removeCondition {
		typedef cloudData first;

		_NODISCARD constexpr bool operator()(const cloudData& first) const {
			return (first.pos.x == cont.removeX && first.pos.y == cont.removeY);
		}
	};

	bool areCloseLeft(vec3 pos1, vec3 pos2, float padding, float threshold) {
		return true;
	}

	bool areCloseRight(vec3 pos1, vec3 pos2, float padding, float threshold) {
		return true;
	}

	bool areCloseY(vec3 pos1, vec3 pos2, float threshold) {
		return true;
	}

	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
	void render()
	{

		srand((unsigned)time(NULL));

		float SCALEVAL = 1;

		static const int CLOUDHEIGHT = 10;
		static const int CLOUDWIDTH = 45;

		static int CLOUDCOUNT = CLOUDWIDTH * CLOUDHEIGHT;

		static float HORIZONTAL_SPACE = SCALEVAL * 1;
		static float VERTICAL_SPACE = SCALEVAL * 1;

		static float HSTART = -7;
		static float VSTART = -2;


		float hpos = HSTART;
		float vpos = VSTART;

		

		
		/*short* terrainData
		short terrainData[CLOUDHEIGHT][CLOUDWIDTH] = {
			{11,0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0, 11,},
			{11,0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0, 11,},
			{11,0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0, 11,},
			{11,1,	15,	2,	3,	4,	0,	11,	0,	0,	1,	4,	6,	0,	0,	3,	0,	11,	0,	0,	1,	4,	6,	1,	1,	3,	9,	4,	5,	1,	11,	2,	6,	1, 11,},
			{11,1,	1,	8,	4,	6,	12,	13,	12,	0,	1,	3,	6,	1,	1,	3,	1,	1,	9,	2,	4,	3,	11,	1,	15,	1,	1,	12,	6,	3,	4,	1,	1,	9, 11,},
			{11,1,	9,	2,	3,	15,	13,	10,	13,	10,	2,	4,	6,	1,	1,	3,	5,	8,	8,	1,	1,	4,	6,	1,	1,	3,	9,	4,	5,	1,	11,	2,	6,	1, 11,},
			{11,1,	1,	8,	4,	6,	14,	11,	5,	8,	1,	4,	6,	1,	1,	3,	1,	1,	9,	2,	9,	3,	11,	1,	1,	7,	1,	12,	6,	3,	4,	1,	1,	15, 11,}

		};
		
		short decorData[CLOUDHEIGHT][CLOUDWIDTH] = {
			{0,0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0, 11,},
			{0,0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0, 11,},
			{0,0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0, 11,},
			{0,1,	15,	2,	3,	4,	0,	11,	0,	0,	1,	4,	6,	0,	0,	3,	0,	11,	0,	0,	1,	4,	6,	1,	1,	3,	9,	4,	5,	1,	11,	2,	6,	1, 11,},
			{0,1,	1,	8,	4,	6,	12,	13,	12,	0,	1,	3,	6,	1,	1,	3,	1,	1,	9,	2,	4,	3,	11,	1,	15,	1,	1,	12,	6,	3,	4,	1,	1,	9, 11,},
			{0,1,	9,	2,	3,	15,	13,	10,	13,	10,	2,	4,	6,	1,	1,	3,	5,	8,	8,	1,	1,	4,	6,	1,	1,	3,	9,	4,	5,	1,	11,	2,	6,	1, 11,},
			{0,1,	1,	8,	4,	6,	14,	11,	5,	8,	1,	4,	6,	1,	1,	3,	1,	1,	9,	2,	9,	3,	11,	1,	1,	7,	1,	12,	6,	3,	4,	1,	1,	15, 11,}
		};


		*/

		static int terrainloaded = 0;

		static short terrainData[CLOUDHEIGHT][CLOUDWIDTH];
		static short decorData[CLOUDHEIGHT][CLOUDWIDTH];

		if (!terrainloaded) {

			for (size_t i = 0; i < CLOUDWIDTH; i++)
			{
				for (size_t j = 0; j < 3; j++)
				{
					terrainData[j][i] = 0;
					decorData[j][i] = 0;
				}

				terrainData[3][i] = int(16 * rand() / float(RAND_MAX));
				decorData[3][i] = int(16 * rand() / float(RAND_MAX));

				if (rand() / float(RAND_MAX) < 0.1) terrainData[3][i] = 0;
				if (terrainData[3][i - 1] == 0 && rand() / float(RAND_MAX) < 0.5) {
					terrainData[3][i] = 0;
					decorData[3][i] = 0;
				}

				for (size_t j = 4; j < CLOUDHEIGHT; j++)
				{
					terrainData[j][i] = 1 + int(15 * rand() / float(RAND_MAX));
					decorData[j][i] = 1 + int(15 * rand() / float(RAND_MAX));
				}
			}

			for (size_t i = 0; i < CLOUDHEIGHT; i++)
			{
				terrainData[i][0] = 13;
				terrainData[i][1] = 13;
				terrainData[i][CLOUDWIDTH - 1] = 13;
				terrainData[i][CLOUDWIDTH - 2] = 13;
			}
		}

		terrainloaded = 1;









		int boxOn;
		int boxAbove;
		int boxLeft;
		int boxRight;
		int boxBelow, boxBelowL, boxBelowM, boxBelowR;

		vec2 currentPos = vec2(-1.f * round(mychar.pos.x) + 7, round(mychar.pos.y) + CLOUDHEIGHT - 3);

		boxOn = terrainData[int(currentPos.y)][int(currentPos.x)];
		boxAbove = terrainData[int(currentPos.y) - 1][int(currentPos.x)];

		boxBelowL = terrainData[int(currentPos.y) + 1][int(round(currentPos.x-1))];
		boxBelowM = terrainData[int(currentPos.y) + 1][int(currentPos.x)];
		boxBelowR = terrainData[int(currentPos.y) + 1][int(round(currentPos.x+1))];

		boxBelow = (abs(boxBelowL) + abs(boxBelowM) + abs(boxBelowR)) != 0;

		boxLeft = terrainData[int(currentPos.y)][int(currentPos.x) - 1];
		boxRight = terrainData[int(currentPos.y)][int(currentPos.x) + 1];



		float posDiff = mychar.pos.x - round(mychar.pos.x);

		if (boxBelow && (mychar.pos.y != floor(mychar.pos.y)))
		{
			cout << " BOX BELOW " << endl;
			mychar.collideDown = true;

			if (boxBelowL && posDiff > 0.1 && abs(mychar.pos.y - round(mychar.pos.y)) < 0.05) {
				mychar.collideDown = true;
			}
			else if (boxBelowR && posDiff < -0.1 && abs(mychar.pos.y - round(mychar.pos.y)) < 0.05) {
				mychar.collideDown = true;
			}
			else if (boxBelowM && abs(mychar.pos.y - round(mychar.pos.y)) < 0.1) {
				mychar.pos.y = round(mychar.pos.y);
				mychar.collideDown = true;
			}
			else {
				mychar.collideDown = false;
			}
		}
		else if (!boxBelowM) {
			mychar.collideDown = false;
		}


		if (boxLeft != 0 && (mychar.pos.x != floor(mychar.pos.x)))
		{

			mychar.collideLeft = true;
			if (abs(mychar.pos.x - floor(mychar.pos.x)) < 0.1) {
				mychar.pos.x = round(mychar.pos.x);
				mychar.collideLeft = true;
			}
			else {
				mychar.collideLeft = false;
			}
		}
		else if (boxLeft == 0) {
			mychar.collideLeft = false;
		}


		if (boxRight != 0 && (mychar.pos.x != ceil(mychar.pos.x)))
		{
			mychar.collideRight = true;
			if (abs(mychar.pos.x - ceil(mychar.pos.x)) < 0.1) {
				mychar.pos.x = round(mychar.pos.x);
				mychar.collideRight = true;
			}
			else {
				mychar.collideRight = false;
			}
		}
		else if (boxRight == 0) {
			mychar.collideRight = false;
		}

		cout << posDiff << " " << mychar.pos.x << " " << boxBelowL << " " << boxBelowM << " " << boxBelowR << endl;




		double frametime = get_last_elapsed_time();

		mychar.process(frametime);


		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width/(float)height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks - please leave these alone for now
		
		glm::mat4 V, M, P; //View, Model and Perspective matrix
		V = mycam.process(frametime);
		M = glm::mat4(1);
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width/ (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones
		

		// RENDER SKY

		float sangle = 3.1415926 / 2.;
		glm::mat4 RotateXSky = glm::rotate(glm::mat4(1.0f), sangle, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 camp = -mycam.pos;
		glm::mat4 TransSky = glm::translate(glm::mat4(1.0f), camp);
		glm::mat4 SSky = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));

		glm::mat4 RSky = glm::rotate(glm::mat4(1.0f), 0.01f * mychar.pos.x, glm::vec3(0.0f, 0.0f, 1.0f));

		M = TransSky * RotateXSky * RSky * SSky;


		psky->bind();		
		glUniformMatrix4fv(psky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(psky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(psky->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(psky->getUniform("campos"), 1, &mycam.pos[0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureNormal);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, TextureNight);
		static float ttime = 0;
		ttime += frametime;
		float dn = sin(ttime)*0.5 +0.5;
		glUniform1f(psky->getUniform("dn"), dn);		
		glDisable(GL_DEPTH_TEST);
		shape->draw(psky, FALSE);
		glEnable(GL_DEPTH_TEST);	
		psky->unbind();

		
		glm::mat4 RotateX;
		glm::mat4 TransZ;
		glm::mat4 S;

	

		//RENDER CLOUDS

		// Draw the box using GLSL.
		cloudshader->bind();
		//glDisable(GL_DEPTH_TEST);

		//send the matrices to the shaders
		glUniformMatrix4fv(cloudshader->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(cloudshader->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(cloudshader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(cloudshader->getUniform("campos"), 1, &mycam.pos[0]);

		glBindVertexArray(VertexArrayID);
		//actually draw from vertex 0, 3 vertices
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);



		

		mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10));


		static float w = 0;
		w += 0.01;
		


		//S = 10.0f * glm::scale(glm::mat4(1.0f), glm::vec3(6.f, 4.f, 0.f));
		S = SCALEVAL * glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, 1));
		
		mat4 RotateY = inverse(glm::rotate(glm::mat4(1), mycam.rot.y, glm::vec3(0, 1, 0)));
		mat4 TransCloud = mat4(0);
		mat4 R = mat4(0);
		vec2 textureOffset = vec2(0);

		
		

		float MAXRADIUS = 30.f;
		float MAXHEIGHTVAR = 15.f;
		float MAXROT = 3.14159f * 2.0f;

		static list<mat4> cloudList;
		static list<cloudData> cloudListFinal;
		static list<vec2> cloudTexList;



		if (cloudList.size() == 0) {
			for (size_t i = 0; i < CLOUDHEIGHT; i++)
			{
				hpos = HSTART;
				for (size_t j = 0; j < CLOUDWIDTH; j++)
				{
					float r = 0;
					float r2 = 0;


					vec4 positions = glm::vec4(hpos, vpos, 0, 0);

					TransCloud = glm::translate(glm::mat4(1.0f), vec3(positions));

					//r = 4.f * rand() / float(RAND_MAX);
					//r2 = 4.f * rand() / float(RAND_MAX);
					//textureOffset = vec2(floor(r) / 4.f, floor(r2) / 4.f);

					short texID = terrainData[CLOUDHEIGHT - i - 1][j];

					if (texID > 0) {
						textureOffset = vec2((texID / 4) / 4.f, (texID % 4) / 4.f);

						//M = TransCloud * T * RotateY * S;
						cloudList.push_front(TransCloud);
						cloudTexList.push_front(textureOffset);
					}



					hpos += HORIZONTAL_SPACE;
				}

				vpos += VERTICAL_SPACE;

			}
		}

		CLOUDCOUNT = cloudList.size();

		R = glm::rotate(glm::mat4(1), 0.f, glm::vec3(0, 1, 0));

		auto current = cloudList.begin();
		auto currentTex = cloudTexList.begin();

		if (cloudListFinal.size() == 0) {

			for (size_t j = 0; j < CLOUDHEIGHT; j++)
			{
				for (size_t k = 0; k < CLOUDWIDTH; k++) {

					if (terrainData[j][k]) {
						int i = CLOUDHEIGHT * j + k;

						cloudData newCloud;
						newCloud.M = *current * T * RotateY * R * S;
						newCloud.tex = *currentTex;

						newCloud.pos.x = k;
						newCloud.pos.y = j;

						vec4 pos = vec4(0);

						cloudListFinal.push_front(newCloud);

						advance(current, 1);
						advance(currentTex, 1);
					}

				}
			}
		}

		auto currentFinal = cloudListFinal.begin();


		mat4 T0 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1000, 0));


		
		static removeCondition rc;
		
		
		if (cont.x) {
			terrainData[int(currentPos.y) + 1][int(round(currentPos.x))] = 0;
			cont.removeX = int(round(currentPos.x));
			cont.removeY = int(currentPos.y) + 1;
			//cloudListFinal.remove_if(rc);

			currentFinal = cloudListFinal.begin();

			for (size_t i = 0; i < CLOUDCOUNT; i++)
			{
				if ((*currentFinal).pos.x == cont.removeX && (*currentFinal).pos.y == cont.removeY)
				{
					(*currentFinal).M = T0 * (*currentFinal).M;
					break;
				}
				
				advance(currentFinal, 1);
			}
		}


		//CLOUDCOUNT = cloudListFinal.size();


		mat4 M2 = mat4(1);

		static float x;
		x += 0.01;
		
		currentFinal = cloudListFinal.begin();
		
		currentFinal = cloudListFinal.begin();
		for (size_t i = 0; i < CLOUDCOUNT; i++)
		{
			M = (*currentFinal).M;
			textureOffset = (*currentFinal).tex;

			S = glm::scale(glm::mat4(1.0f), 1.0f * glm::vec3(1, 1, 1));
			T = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.95f, -1));
			M2 = M * T;


			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, TextureVegetation);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, TextureNormal);


			float dontrenderval = 0;
			glUniform1f(cloudshader->getUniform("dontrender"), dontrenderval);
			glUniformMatrix4fv(cloudshader->getUniform("M"), 1, GL_FALSE, &M2[0][0]);
			glUniform2fv(cloudshader->getUniform("texoff"), 1, &textureOffset[0]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);



			dontrenderval = 0;
			

			if (M[3].y > 500) {
				dontrenderval = 1000.f;
				M = T * M;
			}
			T = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, dontrenderval, 0));
			M = T * M;

			glUniform1f(cloudshader->getUniform("dontrender"), dontrenderval);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, TextureClouds);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, TextureNormal);

			//textureOffset = *currentTex;
			glUniformMatrix4fv(cloudshader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glUniform2fv(cloudshader->getUniform("texoff"), 1, &textureOffset[0]);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);


			
			advance(currentFinal, 1);
		}



		cloudshader->unbind();

		prog->bind();

		//RENDER CHARACTER


		textureOffset = vec2((3 / 4) / 4.f, (3 % 4) / 4.f);

		vec3 color = vec3(0.3, 0.02, 0.02);

		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);
		glUniform3fv(prog->getUniform("setColor"), 1, &color[0]);


		T = glm::translate(glm::mat4(1.0f), vec3(-1.f * mychar.pos.x, -1.f * mychar.pos.y, -10));
		
		mat4 prev = mat4(1);
		mat4 T2 = glm::translate(glm::mat4(1.0f), vec3(0, 0.1, 0));
		
		//T = glm::translate(glm::mat4(1.0f), -1.f * mychar.pos);
		S = glm::scale(glm::mat4(1.0f), glm::vec3(0.3, 0.6, 0.3));
		M = T2 * T * S;
		prev = T;

		color = vec3(0.3, 0.02, 0.02);

		glUniform3fv(prog->getUniform("setColor"), 1, &color[0]);
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);

		static float w2;
		w2 += mychar.animationspeed;



		mat4 temp = mat4(1);

		float rot = 3.1415926f + 3.1415926f * 0.4f * sin(w2);
		float rot2 = 3.1415926f - 3.1415926f * 0.4f * sin(w2);

		S = glm::scale(glm::mat4(1.0f), glm::vec3(0.2, 0.4, 0.2));
		R = glm::rotate(glm::mat4(1.0f), rot, glm::vec3(0, 0, 1));
		T2 = glm::translate(glm::mat4(1.0f), vec3(0, 0.45, 0));
		T = glm::translate(glm::mat4(1.0f), vec3(0, -0.1, -0.2));
		M = prev * T * R * S * T2;

		color = vec3(0.3, 0.02, 0.02) - 0.05f;

		glUniform3fv(prog->getUniform("setColor"), 1, &color[0]);
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
		
		T = glm::translate(glm::mat4(1.0f), vec3(0, -0.1, 0.2));
		R = glm::rotate(glm::mat4(1.0f), rot2, glm::vec3(0, 0, 1));
		M = prev * T * R * S * T2;

		color = vec3(0.3, 0.02, 0.02) + 0.05f;

		glUniform3fv(prog->getUniform("setColor"), 1, &color[0]);
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);


		glBindVertexArray(0);

		glEnable(GL_DEPTH_TEST);

		prog->unbind();

		float afasfdhk = mychar.maxspeed;


	}

};
//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom();

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
