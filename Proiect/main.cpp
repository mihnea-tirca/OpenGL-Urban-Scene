//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
int glWindowWidth = 800;
int glWindowHeight = 600;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;
enum WINDOW_MODE { WINDOWED_FULL, WINDOWED };
WINDOW_MODE window_mode;
const GLFWvidmode* mode;
GLFWmonitor* monitor;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

const glm::vec3 lightPos = glm::vec3(25.5321f, 80.821f, -233.757f);
const glm::vec3 lightTarget = glm::vec3(-35.0f, 11.0f, 32.0f);
const glm::vec3 lightDir = lightPos - lightTarget;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

// presentation animation
bool stage[10];
double presentation_start;
bool animate = false;
// object animation
bool shutters_direction = false, shutters_opened = true, shutters_closed = false;
float angle = 0;
//camera
Camera myCamera(glm::vec3(-60.0958f, 8.59842f, 28.3638f));
// sa fie aceeasi viteza WASD
float currentFrame, deltaTime = 0.0f, lastFrame = 0.0f;
// pt mouse
float lastX = glWindowWidth / 2.0f;
float lastY = glWindowHeight / 2.0f;
bool firstMouse = true;

// fog
float fogDensity = 0.005f;

// movement
bool pressedKeys[1024];

// depth map
gps::Model3D screenQuad;
GLuint shadowMapFBO;
GLuint depthMapTexture;
bool showDepthMap;

// My objects
gps::Model3D terrain;
gps::Model3D houses;
gps::Model3D shutter1;
gps::Model3D shutter2;
gps::Model3D road;
gps::Model3D sidewalk;
gps::Model3D fountain;
gps::Model3D benches;
gps::Model3D lamps;
gps::Model3D trash_cans;
gps::Model3D bark;
gps::Model3D leaves;

// Sky box
gps::SkyBox mySkyBox;

// Shaders
gps::Shader myCustomShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;
gps::Shader skyBoxShader;

GLenum glCheckError_(const char *file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO	
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	myCamera.ProcessMouseMovement(xoffset, yoffset);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}

	// animatie de prezentare
	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		animate = !animate;
		if (animate) {
			// init animation
			glfwSetCursorPosCallback(glWindow, NULL);
			myCamera.initAnimation();
			for (int i = 1; i < 10; i++) {
				stage[i] = true;
			}
			presentation_start = glfwGetTime();
		}
		else {
			//end animation
			glfwSetCursorPosCallback(glWindow, mouse_callback);
			myCamera.endAnimation();
		}
	}

	// solid
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// wireframe
	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	// point
	if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	// -density
	if (key == GLFW_KEY_3) {
		if (fogDensity >= 0.001f) {
			fogDensity -= 0.001f;
		}
		myCustomShader.useShaderProgram();
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
	}

	// +density
	if (key == GLFW_KEY_4) {
		fogDensity += 0.001f;
		myCustomShader.useShaderProgram();
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
	}

	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		shutters_opened = false;
		shutters_closed = false;
		shutters_direction = !shutters_direction;
	}
	
	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		if (window_mode == WINDOWED) {
			window_mode = WINDOWED_FULL;
		}
		else if (window_mode == WINDOWED_FULL) {
			window_mode = WINDOWED;
		}
	}
}

void processMovement()
{
	if (pressedKeys[GLFW_KEY_W] && !animate) {
		myCamera.ProcessKeyboard(MOVE_FORWARD, deltaTime);
	}

	if (pressedKeys[GLFW_KEY_S] && !animate) {
		myCamera.ProcessKeyboard(MOVE_BACKWARD, deltaTime);
	}

	if (pressedKeys[GLFW_KEY_A] && !animate) {
		myCamera.ProcessKeyboard(MOVE_LEFT, deltaTime);
	}

	if (pressedKeys[GLFW_KEY_D] && !animate) {
		myCamera.ProcessKeyboard(MOVE_RIGHT, deltaTime);
	}

	if (pressedKeys[GLFW_KEY_SPACE] && !animate) {
		myCamera.ProcessKeyboard(MOVE_UP, deltaTime);
	}

	if (pressedKeys[GLFW_KEY_LEFT_CONTROL] && !animate) {
		myCamera.ProcessKeyboard(MOVE_DOWN, deltaTime);
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	//glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", monitor, NULL);

	// full screen
	monitor = glfwGetPrimaryMonitor();
	mode = glfwGetVideoMode(monitor);
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	glWindow = glfwCreateWindow(mode->width, mode->height, "We gamin'", monitor, NULL);

	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouse_callback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3, 0.3, 0.3, 1.0);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	/*glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise*/
	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initSkyBox() {
	std::vector<const GLchar*> faces;
	faces.push_back("objects/skybox/right.tga");
	faces.push_back("objects/skybox/left.tga");
	faces.push_back("objects/skybox/top.tga");
	faces.push_back("objects/skybox/bottom.tga");
	faces.push_back("objects/skybox/back.tga");
	faces.push_back("objects/skybox/front.tga");
	mySkyBox.Load(faces);
}

void initObjects() {
	screenQuad.LoadModel("objects/quad/quad.obj");
	// My objects
	terrain.LoadModel("objects/terrain/terrain.obj");
	houses.LoadModel("objects/houses/houses.obj");
	shutter1.LoadModel("objects/houses/shutter1.obj");
	shutter2.LoadModel("objects/houses/shutter2.obj");
	road.LoadModel("objects/road/road.obj");
	sidewalk.LoadModel("objects/sidewalk/sidewalk.obj");
	fountain.LoadModel("objects/fountain/fountain.obj");
	benches.LoadModel("objects/benches/benches.obj");
	lamps.LoadModel("objects/lamps/lamps.obj");
	bark.LoadModel("objects/trees/bark.obj");
	leaves.LoadModel("objects/trees/leaves.obj");
	trash_cans.LoadModel("objects/trash-cans/trash-cans.obj");
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();
	depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
	depthMapShader.useShaderProgram();
	skyBoxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyBoxShader.useShaderProgram();
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");	
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(normalMatrix * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "hasMask"), 0);
}

void initFBO() {
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));
	const GLfloat near_plane = 0.01f, far_plane = 700.0f;
	glm::mat4 lightProjection = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
	return lightSpaceTrMatrix;
}

void drawObjects(gps::Shader shader, bool depthPass) {
		
	shader.useShaderProgram();
	
	model = glm::mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	terrain.Draw(shader);
	houses.Draw(shader);

	if (!shutters_closed && !shutters_opened) {
		if (shutters_direction) {
			angle -= deltaTime * 100.0f;
		}
		else {
			angle += deltaTime * 100.0f;
		}
	}

	if (angle < -179.0f && shutters_direction) {
		shutters_closed = true;
		angle = -180.0f;
	}

	if (angle > -1.0f && !shutters_direction) {
		shutters_opened = true;
		angle = 0.0f;
	}

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-37.8676f, 10.6024f, 58.9943f));
	model = glm::rotate(model, glm::radians(-angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, glm::vec3(37.8676f, -10.6024f, -58.9943f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	shutter1.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-38.4758f, 10.611f, 58.9943f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, glm::vec3(38.4758f, -10.611f, -58.9943f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	shutter2.Draw(shader);

	model = glm::mat4(1.0f);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	road.Draw(shader);
	sidewalk.Draw(shader);
	bark.Draw(shader);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "hasMask"), 1);
	leaves.Draw(shader);
	lamps.Draw(shader);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "hasMask"), 0);
	fountain.Draw(shader);
	benches.Draw(shader);
	trash_cans.Draw(shader);
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyBoxShader.shaderProgram, "view"), 1, GL_FALSE,
		glm::value_ptr(view));
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyBoxShader.shaderProgram, "projection"), 1, GL_FALSE,
		glm::value_ptr(projection));
	mySkyBox.Draw(skyBoxShader, view, projection);
}

void renderScene() {
	currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawObjects(depthMapShader, true);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (showDepthMap) {
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
	}
	else {
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		myCustomShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		model = glm::mat4(1.0f);
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * model)) * lightDir));

		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(computeLightSpaceTrMatrix()));

		drawObjects(myCustomShader, false);
		if (animate && glfwGetTime() > presentation_start + 0.01) {
			if (glfwGetTime() < presentation_start + 5 && stage[1]) {
				myCamera.ProcessKeyboard(MOVE_FORWARD, deltaTime);
			}
			else if (glfwGetTime() < presentation_start + 8 && stage[2]) {
				stage[1] = false;
				myCamera.ProcessKeyboard(MOVE_FORWARD, deltaTime);
				myCamera.ProcessMouseMovement(-1.0f, 0.0f);
			}
			else if (glfwGetTime() > presentation_start + 8 && glfwGetTime() < presentation_start + 12 && stage[3]) {
				stage[2] = false;
				myCamera.ProcessMouseMovement(-1.0f, 0.0f);
			}
			else if(myCamera.getYaw() < -90.0f && stage[4]) {
				stage[3] = false;
				myCamera.ProcessMouseMovement(1.0f, 0.0f);
			}
			else if (myCamera.getPosition().x > -54.0f && stage[5]) {
				stage[4] = false;
				myCamera.setMovementSpeed(5.0f);
				myCamera.ProcessKeyboard(MOVE_LEFT, deltaTime);
			}
			else if (myCamera.getYaw() < 0.0f && stage[6]) {
				stage[5] = false;
				myCamera.setMouseSensitivity(0.08f);
				myCamera.ProcessKeyboard(MOVE_FORWARD, deltaTime);
				myCamera.ProcessMouseMovement(1.0f, 0.0f);
			}
			else if (myCamera.getYaw() > -140.0f && stage[7]) {
				stage[6] = false;
				myCamera.ProcessKeyboard(MOVE_FORWARD, deltaTime);
				myCamera.ProcessMouseMovement(-1.5f, 0.0f);
			}
			else if (myCamera.getYaw() < -90.0f && stage[8]) {
				stage[7] = false;
				myCamera.ProcessKeyboard(MOVE_FORWARD, deltaTime);
				myCamera.ProcessMouseMovement(1.0f, 0.0f);
			}
		}
	}
}

void cleanup() {
	glDeleteTextures(1,&depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}

int main(int argc, const char * argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}

	initOpenGLState();
	initSkyBox();
	initObjects();
	initShaders();
	initUniforms();
	initFBO();

	glCheckError();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(glWindow)) {
		renderScene();
		processMovement();	

		glfwPollEvents();
		glfwSwapBuffers(glWindow);

		//glCheckError();
	}

	cleanup();

	return 0;
}
