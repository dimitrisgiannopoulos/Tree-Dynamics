// Include C++ headers
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Shader loading utilities and other
#include <common/shader.h>
#include <common/util.h>
#include <common/camera.h>
#include <common/model.h>
#include <common/texture.h>

using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();
struct Light; struct Material;
void uploadMaterial(const Material& mtl);
void uploadLight(const Light& light);
map<int, mat4> calculateModelPoseFromCoordinates(map<int, float> q);
vector<mat4> calculateSkinningTransformations(map<int, float> q);
vector<float> calculateSkinningIndices();

/////////////////////////////////////////////////////////////////////////////////////////
//Header code dump
//skeleton.h
#ifndef SKELETON_H
#define SKELETON_H

#include <GL/glew.h>
#include <vector>
#include <map>
#include <glm/glm.hpp>

class Drawable;

struct Joint {
	Joint* parent = NULL;
	glm::mat4 jointLocalTransformation, jointWorldTransformation,
		jointBindTransformation;

	/** After updating the jointLocalTransformation call this method to compute
	* the jointWorldTransformation
	*/
	void updateWorldTransformation();
};

struct Body {
	Joint* joint;  // not owned by the body, just a reference pointer
	std::vector<Drawable*> drawables; // owned by the body, thus must be freed

									  /* Free all drawables (a body can have many drawables)*/
	~Body();

	/* Given the view and projection matrix draw every attached drawables */
	void draw(
		const GLuint& modelMatrixLocation,
		const GLuint& viewMatrixLocation,
		const GLuint& projectionMatrixLocation,
		const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
};

struct Skeleton {
	std::map<int, Body*> bodies;
	std::map<int, Joint*> joints;

	// shader locations to M, V, P
	GLuint modelMatrixLocation, viewMatrixLocation, projectionMatrixLocation;

	Skeleton(
		GLuint modelMatrixLocation,
		GLuint viewMatrixLocation,
		GLuint projectionMatrixLocation);

	/* Free all bodies and joints*/
	~Skeleton();

	/* Update joint local coordinates */
	void setPose(const std::map<int, glm::mat4>& jointTransformations);

	/* Given the view and projection matrix draw every attached drawables */
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

	/* Get joint world transformations after setting the pose */
	std::map<int, glm::mat4> getJointWorldTransformations();
};

#endif
//end of skeleton.h
//////////////////////////////////////////////////////////////////////////////////////////

//skeleton.cpp
#include <glm/gtc/matrix_transform.hpp>

void Joint::updateWorldTransformation() {
	if (parent == NULL) // root
	{
		jointWorldTransformation = jointLocalTransformation;
	}
	else {
		jointWorldTransformation =
			parent->jointWorldTransformation * jointLocalTransformation;
	}
}

Body::~Body() {
	for (Drawable* d : drawables) {
		delete d;
	}
}

void Body::draw(
	const GLuint& modelMatrixLocation,
	const GLuint& viewMatrixLocation,
	const GLuint& projectionMatrixLocation,
	const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix) {
	joint->updateWorldTransformation();
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE,
		&joint->jointWorldTransformation[0][0]);
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE,
		&projectionMatrix[0][0]);

	for (Drawable* d : drawables) {
		d->bind();
		d->draw();
	}
}

Skeleton::Skeleton(
	GLuint modelMatrixLocation,
	GLuint viewMatrixLocation,
	GLuint projectionMatrixLocation) :
	modelMatrixLocation(modelMatrixLocation),
	viewMatrixLocation(viewMatrixLocation),
	projectionMatrixLocation(projectionMatrixLocation) {
}

Skeleton::~Skeleton() {
	for (auto body : bodies) {
		delete body.second;
	}

	for (auto joint : joints) {
		delete joint.second;
	}
}

void Skeleton::setPose(const std::map<int, glm::mat4>& jointTransformations) {
	for (const auto& tran : jointTransformations) {
		joints[tran.first]->jointLocalTransformation = tran.second;
	}
}

void Skeleton::draw(const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix) {
	for (auto& body : bodies) {
		body.second->draw(modelMatrixLocation, viewMatrixLocation,
			projectionMatrixLocation, viewMatrix, projectionMatrix);
	}
}

std::map<int, glm::mat4> Skeleton::getJointWorldTransformations() {
	std::map<int, glm::mat4> jointWorldTransformations;
	// update before computing
	for (auto joint : joints) {
		joint.second->updateWorldTransformation();
	}

	for (auto joint : joints) {
		jointWorldTransformations[joint.first] = joint.second->jointWorldTransformation;
	}

	return  jointWorldTransformations;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////



#define W_WIDTH 1024
#define W_HEIGHT 768
#define TITLE "Lab 05"

void defineJointPoints();
void quickSort(std::vector<float> &arr, std::vector<int> &indices, int left, int right);
struct Light; struct Material;
void uploadMaterial(const Material& mtl);
void uploadLight(const Light& light);


// Global variables
GLFWwindow* window;
Camera* camera;
GLuint shaderProgram;
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation, modelMatrixLocation2, planeLocation;
GLuint diffuceColorSampler, specularColorSampler;
GLuint diffuseTexturetree, specularTexturetree, diffuseTextureleaves, specularTextureleaves;
GLuint treeVAO, leavesVAO, planeVAO;
GLuint treeVerticiesVBO, treeUVVBO, treeNormalsVBO, leavesVerticiesVBO, leavesUVVBO, leavesNormalsVBO, planeColorsVAO, planeColorsVBO, planeVerticiesVBO;
std::vector<vec3> objVerticestree, objNormalstree, objVerticesleaves, objNormalsleaves;
std::vector<vec2> objUVstree, objUVsleaves;
std::vector<vec3> treeJoints;
Skeleton* skeleton;
// light properties
GLuint LaLocation, LdLocation, LsLocation, lightPositionLocation, lightPowerLocation;
// material properties
GLuint KdLocation, KsLocation, KaLocation, NsLocation;
Drawable *segment, *skeletonSkin;
GLuint useSkinningLocation, boneTransformationsLocation;
GLuint surfaceVAO, surfaceVerticesVBO, surfacesBoneIndecesVBO, maleBoneIndicesVBO;
float t = 0;

struct Light {
	glm::vec4 La;
	glm::vec4 Ld;
	glm::vec4 Ls;
	glm::vec3 lightPosition_worldspace;
	float power;
};

struct Material {
	glm::vec4 Ka;
	glm::vec4 Kd;
	glm::vec4 Ks;
	float Ns;
};

const Material boneMaterial{
	vec4{ 0.1, 0.1, 0.1, 1 },
	vec4{ 1.0, 1.0, 1.0, 1 },
	vec4{ 0.3, 0.3, 0.3, 1 },
	0.1f
};

Light light{
	vec4{ 1, 1, 1, 1 },
	vec4{ 1, 1, 1, 1 },
	vec4{ 1, 1, 1, 1 },
	vec3{ 0, 4, 4 },
	20.0f
};

void uploadMaterial(const Material& mtl) {
	glUniform4f(KaLocation, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a);
	glUniform4f(KdLocation, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
	glUniform4f(KsLocation, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
	glUniform1f(NsLocation, mtl.Ns);
}

void uploadLight(const Light& light) {
	glUniform4f(LaLocation, light.La.r, light.La.g, light.La.b, light.La.a);
	glUniform4f(LdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
	glUniform4f(LsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
	glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
		light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
	glUniform1f(lightPowerLocation, light.power);
}

// Coordinate names for mnemonic indexing
enum CoordinateName {
	//PELVIS_TRA_X = 0, PELVIS_TRA_Y, PELVIS_TRA_Z, PELVIS_ROT_X, PELVIS_ROT_Y,
	BONE1_TRA_X = 0, BONE1_TRA_Y, BONE1_TRA_Z, PELVIS_ROT_X, PELVIS_ROT_Y,
	PELVIS_ROT_Z, HIP_R_FLEX, HIP_R_ADD, HIP_R_ROT, KNEE_R_FLEX, ANKLE_R_FLEX,
	HIP_L_FLEX, HIP_L_ADD, HIP_L_ROT, KNEE_L_FLEX, ANKLE_L_FLEX,
	LUMBAR_FLEX, LUMBAR_BEND, LUMBAR_ROT, DOFS
};

// Joint names for mnemonic indexing
enum JointName {
	//BASE = 0, HIP_R, KNEE_R, ANKLE_R, SUBTALAR_R, MTP_R, HIP_L, KNEE_L, ANKLE_L, SUBTALAR_L, MTP_L, BACK, JOINTS
	ROOT = 0, POINT2, POINT3, POINT4, POINT5, POINT6, POINT7, POINT8, POINT9, POINT10, POINT11, POINT12, JOINTS
};

// Body names for mnemonic indexing
enum BodyName {
	//PELVIS = 0, FEMUR_R, TIBIA_R, TALUS_R, CALCN_R, TOES_R, FEMUR_L, TIBIA_L, TALUS_L, CALCN_L, TOES_L, TORSO, BODIES
	BONE1 = 0, BONE2, BONE3, BONE4, BONE5, BONE6, BONE7, BONE8, BONE9, BONE10, BONE11, BONE12, BONE13
};

// default pose used for binding the skeleton and the mesh
static const map<int, float> bindingPose = {
	{ CoordinateName::BONE1_TRA_X, 0.0f },
	{ CoordinateName::BONE1_TRA_Y, 0.0f },
	{ CoordinateName::BONE1_TRA_Z, 0.0f },
	{ CoordinateName::HIP_R_FLEX, 3.0f },
	{ CoordinateName::HIP_R_ADD, -5.0f },
	{ CoordinateName::HIP_R_ROT, 0.0f },
	{ CoordinateName::KNEE_R_FLEX, -15.0f },
	{ CoordinateName::ANKLE_R_FLEX, 15.0f },
	{ CoordinateName::HIP_L_FLEX, 3.0f },
	{ CoordinateName::HIP_L_ADD, -5.0f },
	{ CoordinateName::HIP_L_ROT, 0.0f },
	{ CoordinateName::KNEE_L_FLEX, -15.0f },
	{ CoordinateName::ANKLE_L_FLEX, 15.0f },
	{ CoordinateName::LUMBAR_FLEX, 0.0f },
	{ CoordinateName::LUMBAR_BEND, 0.0f },
	{ CoordinateName::LUMBAR_ROT, 0.0f }
};

map<int, mat4> calculateModelPoseFromCoordinates(map<int, float> q) {
	map<int, mat4> jointLocalTransformations;

	// base / pelvis joint
	mat4 bone1tra = translate(mat4(), vec3(
		q[CoordinateName::BONE1_TRA_X],
		q[CoordinateName::BONE1_TRA_Y],
		q[CoordinateName::BONE1_TRA_Z]));
	jointLocalTransformations[JointName::ROOT] = bone1tra;

	// right hip joint
	vec3 POINT2Offset = treeJoints[0];
	mat4 hipRTra = translate(mat4(), POINT2Offset);
	mat4 hipRRotX = rotate(mat4(), radians(q[CoordinateName::HIP_R_ADD]), vec3(1, 0, 0));
	mat4 hipRRotY = rotate(mat4(), radians(q[CoordinateName::HIP_R_ROT]), vec3(0, 1, 0));
	mat4 hipRRotZ = rotate(mat4(), radians(q[CoordinateName::HIP_R_FLEX]), vec3(0, 0, 1));
	jointLocalTransformations[JointName::POINT2] = hipRTra * hipRRotX * hipRRotY * hipRRotZ;

	// right knee joint
	vec3 kneeROffset = treeJoints[1];
	mat4 kneeRTra = translate(mat4(1.0), kneeROffset);
	mat4 kneeRRotZ = rotate(mat4(), radians(q[CoordinateName::KNEE_R_FLEX]), vec3(0, 0, 1));
	jointLocalTransformations[JointName::POINT3] = kneeRTra * kneeRRotZ;

	// right ankle joint
	vec3 ankleROffset = treeJoints[2];
	mat4 ankleRTra = translate(mat4(1.0), ankleROffset);
	mat4 ankleRRotZ = rotate(mat4(), radians(q[CoordinateName::ANKLE_R_FLEX]), vec3(0, 0, 1));
	mat4 talusRModelMatrix = ankleRRotZ;
	jointLocalTransformations[JointName::POINT4] = ankleRTra * ankleRRotZ;

	// right calcn joint
	vec3 calcnROffset = treeJoints[4];
	mat4 calcnRTra = translate(mat4(1.0), calcnROffset);
	jointLocalTransformations[JointName::POINT5] = calcnRTra;

	// right mtp joint
	vec3 toesROffset = treeJoints[5];
	mat4 mtpRTra = translate(mat4(1.0), toesROffset);
	jointLocalTransformations[JointName::POINT6] = mtpRTra;

	///////////////////////////////////////	LEFT
	//// left hip joint
	//vec3 hipLOffset = treeJoints[7];
	//mat4 hipLTra = translate(mat4(), hipLOffset);
	//mat4 hipLRotX = rotate(mat4(), radians(q[CoordinateName::HIP_L_ADD]), vec3(1, 0, 0));
	//mat4 hipLRotY = rotate(mat4(), radians(q[CoordinateName::HIP_L_ROT]), vec3(0, 1, 0));
	//mat4 hipLRotZ = rotate(mat4(), radians(q[CoordinateName::HIP_L_FLEX]), vec3(0, 0, 1));
	//jointLocalTransformations[JointName::POINT7] = hipLTra * hipLRotX * hipLRotY * hipLRotZ;

	//// left knee joint
	//vec3 kneeLOffset = treeJoints[8];
	//mat4 kneeLTra = translate(mat4(1.0), kneeLOffset);
	//mat4 kneeLRotZ = rotate(mat4(), radians(q[CoordinateName::KNEE_L_FLEX]), vec3(0, 0, 1));
	//jointLocalTransformations[JointName::POINT8] = kneeLTra * kneeLRotZ;

	//// left ankle joint
	//vec3 ankleLOffset = treeJoints[9];
	//mat4 ankleLTra = translate(mat4(1.0), ankleLOffset);
	//mat4 ankleLRotZ = rotate(mat4(), radians(q[CoordinateName::ANKLE_L_FLEX]), vec3(0, 0, 1));
	//mat4 talusLModelMatrix = ankleLRotZ;
	//jointLocalTransformations[JointName::POINT9] = ankleLTra * ankleLRotZ;

	//// left calcn joint
	//vec3 calcnLOffset = vec3(-0.062, -0.053, -0.010);
	//mat4 calcnLTra = translate(mat4(1.0), calcnLOffset);
	//jointLocalTransformations[JointName::POINT10] = calcnLTra;

	//// left mtp joint
	//vec3 toesLOffset = vec3(0.184, -0.002, -0.001);
	//mat4 mtpLTra = translate(mat4(1.0), toesLOffset);
	//jointLocalTransformations[JointName::POINT11] = mtpLTra;


	// back joint
	vec3 backOffset = treeJoints[6];
	mat4 lumbarTra = translate(mat4(1.0), backOffset);
	mat4 lumbarRotX = rotate(mat4(), radians(q[CoordinateName::LUMBAR_BEND]), vec3(1, 0, 0));
	mat4 lumbarRotY = rotate(mat4(), radians(q[CoordinateName::LUMBAR_ROT]), vec3(0, 1, 0));
	mat4 lumbarRotZ = rotate(mat4(), radians(q[CoordinateName::LUMBAR_FLEX]), vec3(0, 0, 1));
	jointLocalTransformations[JointName::POINT7] = lumbarTra * lumbarRotX * lumbarRotY * lumbarRotZ;

	return jointLocalTransformations;
}

vector<mat4> calculateSkinningTransformations(map<int, float> q) {
	auto jointLocalTransformationsBinding = calculateModelPoseFromCoordinates(bindingPose);
	skeleton->setPose(jointLocalTransformationsBinding);
	auto bindingWorldTransformations = skeleton->getJointWorldTransformations();

	auto jointLocalTransformationsCurrent = calculateModelPoseFromCoordinates(q);
	skeleton->setPose(jointLocalTransformationsCurrent);
	auto currentWorldTransformations = skeleton->getJointWorldTransformations();

	vector<mat4> skinningTransformations(JointName::JOINTS);
	for (auto joint : bindingWorldTransformations) {
		mat4 BInvWorld = glm::inverse(joint.second);
		mat4 JWorld = currentWorldTransformations[joint.first];
		skinningTransformations[joint.first] = JWorld * BInvWorld;
	}

	return skinningTransformations;
}

vector<float> calculateSkinningIndices() {
	// Task 4.3: assign a body index for each vertex in the model (skin) based
	// on its proximity to a body part (e.g. tight)
	vector<float> indices;
	for (auto v : skeletonSkin->indexedVertices) {
		// dummy
		//indices.push_back(1.0);
		if (v.y <= 2.5) {
			indices.push_back(JointName::ROOT);
		}
		else if (v.y >= 0.25 && v.y < 0.75) {
			indices.push_back(JointName::POINT2);
		}
		else if (v.y >= 0.75 && v.y < 1.25) {
			indices.push_back(JointName::POINT3);
		}
		else if (v.y >= 1.25 && v.y < 1.75) {
			indices.push_back(JointName::POINT4);
		}
		else if (v.y >= 1.75 && v.y < 2.25) {
			indices.push_back(JointName::POINT5);
		}
		else if (v.y >= 2.25 && v.y < 2.75) {
			indices.push_back(JointName::POINT6);
		}
		else if (v.y >= 2.75 && v.y < 3.25) {
			indices.push_back(JointName::POINT7);
		}
		else {
			indices.push_back(JointName::POINT8);
		}
	}
	return indices;
}


void createContext()
{
	// Create and compile our GLSL program from the shaders
	shaderProgram = loadShaders(
		"StandardShading.vertexshader",
		"StandardShading.fragmentshader");

	// load obj
	loadOBJWithTiny("MapleTreeStem.obj", objVerticestree, objUVstree, objNormalstree);
	loadOBJWithTiny("MapleTreeLeaves.obj", objVerticesleaves, objUVsleaves, objNormalsleaves);

	//find Joints of tree
	//treeJoints.push_back(vec3(0, 0, 0));
	defineJointPoints();

	// Task 6.2: load diffuse and specular texture maps
	diffuseTexturetree = loadSOIL("maple_bark.png");
	specularTexturetree = loadSOIL("MapleTree_specular.bmp");
	diffuseTextureleaves = loadSOIL("leaf.png");
	specularTextureleaves = loadSOIL("MapleTree_specular.bmp");

	// Task 6.3: get a pointer to the texture samplers (diffuseColorSampler, specularColorSampler)
	diffuceColorSampler = glGetUniformLocation(shaderProgram, "diffuceColorSampler");
	specularColorSampler = glGetUniformLocation(shaderProgram, "specularColorSampler");

	// get pointers to the uniform variables
	modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");
	viewMatrixLocation = glGetUniformLocation(shaderProgram, "V");
	projectionMatrixLocation = glGetUniformLocation(shaderProgram, "P");
	KaLocation = glGetUniformLocation(shaderProgram, "mtl.Ka");
	KdLocation = glGetUniformLocation(shaderProgram, "mtl.Kd");
	KsLocation = glGetUniformLocation(shaderProgram, "mtl.Ks");
	NsLocation = glGetUniformLocation(shaderProgram, "mtl.Ns");
	LaLocation = glGetUniformLocation(shaderProgram, "light.La");
	LdLocation = glGetUniformLocation(shaderProgram, "light.Ld");
	LsLocation = glGetUniformLocation(shaderProgram, "light.Ls");
	lightPositionLocation = glGetUniformLocation(shaderProgram, "light.lightPosition_worldspace");
	lightPowerLocation = glGetUniformLocation(shaderProgram, "light.power");
	useSkinningLocation = glGetUniformLocation(shaderProgram, "useSkinning");
	boneTransformationsLocation = glGetUniformLocation(shaderProgram, "boneTransformations");
	planeLocation = glGetUniformLocation(shaderProgram, "planeCoeffs");

	vector<vec3> segmentVertices = {
		vec3(0.0f, 0.0f, 0.0f),
		vec3(0.0f, 0.5f, 0.0f)
	};
	segment = new Drawable(segmentVertices);


	//create plane
	glGenVertexArrays(1, &planeVAO);
	glBindVertexArray(planeVAO);
	glGenBuffers(1, &planeVerticiesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVerticiesVBO);
	float size = 20.0f;
	const GLfloat planeVertices[] = {
		-size, 0, -size,
		-size, 0,  size,
		size, 0,  size,
		-size, 0, -size,
		size, 0,  -size,
		size, 0,  size
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices),
		&planeVertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	const GLfloat planeColors[] = {
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0
	};
	glGenBuffers(1, &planeColorsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, planeColorsVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeColors), &planeColors[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);



	skeleton = new Skeleton(modelMatrixLocation, viewMatrixLocation, projectionMatrixLocation);


	// pelvis
	Joint* baseJoint = new Joint(); // creates a joint
	baseJoint->parent = NULL; // assigns the parent joint (NULL -> no parent)
	skeleton->joints[JointName::ROOT] = baseJoint; // adds the joint in the skeleton's dictionary

	Body* pelvisBody = new Body(); // creates a body
	pelvisBody->drawables.push_back(new Drawable(vector<vec3>{ vec3(0, 0, 0), vec3(0, 0.5, 0) }));
	pelvisBody->drawables.push_back(new Drawable(vector<vec3>{ vec3(0, 0.5, 0), vec3(0, 1, 0) }));
	pelvisBody->drawables.push_back(new Drawable(vector<vec3>{ vec3(0, 1, 0), vec3(0, 1.5, 0) }));
	pelvisBody->joint = baseJoint; // relates to a joint
	skeleton->bodies[BodyName::BONE1] = pelvisBody; // adds the body in the skeleton's dictionary

													// right femur
	Joint* hipR = new Joint();
	hipR->parent = baseJoint;
	skeleton->joints[JointName::POINT2] = hipR;

	Body* femurR = new Body();
	femurR->drawables.push_back(new Drawable(vector<vec3>{ vec3(0, 0, 0), vec3(0, 0.5, 0) }));
	femurR->joint = hipR;
	skeleton->bodies[BodyName::BONE3] = femurR;

	// right tibia
	Joint* kneeR = new Joint();
	kneeR->parent = hipR;
	skeleton->joints[JointName::POINT3] = kneeR;

	Body* tibiaR = new Body();
	tibiaR->drawables.push_back(new Drawable(vector<vec3>{ vec3(0, 0.5, 0), vec3(0, 1, 0) }));
	tibiaR->joint = kneeR;
	skeleton->bodies[BodyName::BONE4] = tibiaR;

	// right talus
	Joint* ankleR = new Joint();
	ankleR->parent = kneeR;
	skeleton->joints[JointName::POINT4] = ankleR;

	Body* talusR = new Body();
	talusR->drawables.push_back(new Drawable(vector<vec3>{ vec3(0, 1, 0), vec3(0, 1.5, 0) }));
	talusR->joint = ankleR;
	skeleton->bodies[BodyName::BONE5] = talusR;

	// right calcn
	Joint* subtalarR = new Joint();
	subtalarR->parent = ankleR;
	skeleton->joints[JointName::POINT5] = subtalarR;

	Body* calcnR = new Body();
	calcnR->drawables.push_back(new Drawable(vector<vec3>{ vec3(0, 1.5, 0), vec3(0, 2, 0) }));
	calcnR->joint = subtalarR;
	skeleton->bodies[BodyName::BONE6] = calcnR;

	// toes
	Joint* mtpR = new Joint();
	mtpR->parent = subtalarR;
	skeleton->joints[JointName::POINT6] = mtpR;

	Body* toesR = new Body();
	toesR->drawables.push_back(new Drawable(vector<vec3>{ vec3(0, 2, 0), vec3(0, 2.5, 0) }));
	toesR->joint = mtpR;
	skeleton->bodies[BodyName::BONE7] = toesR;

	// torso
	Joint* back = new Joint();
	back->parent = baseJoint;
	skeleton->joints[JointName::POINT7] = back;

	Body* torso = new Body();
	torso->drawables.push_back(new Drawable(vector<vec3>{ vec3(0, 2.5, 0), vec3(0, 3, 0) }));
	torso->joint = back;
	skeleton->bodies[BodyName::BONE8] = torso;

	skeletonSkin = new Drawable("MapleTreeStem.obj");
	auto maleBoneIndices = calculateSkinningIndices();
	glGenBuffers(1, &maleBoneIndicesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, maleBoneIndicesVBO);
	glBufferData(GL_ARRAY_BUFFER, maleBoneIndices.size() * sizeof(float),
		&maleBoneIndices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(3);

	// obj
	// Task 6.1: bind object vertex positions to attribute 0, UV coordinates
	// to attribute 1 and normals to attribute 2
	//*/
	//   glGenVertexArrays(1, &treeVAO);
	//   glBindVertexArray(treeVAO);

	//   // vertex VBO
	//   glGenBuffers(1, &treeVerticiesVBO);
	//   glBindBuffer(GL_ARRAY_BUFFER, treeVerticiesVBO);
	//   glBufferData(GL_ARRAY_BUFFER, objVerticestree.size() * sizeof(glm::vec3),
	//       &objVerticestree[0], GL_STATIC_DRAW);
	//   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//   glEnableVertexAttribArray(0);

	////normals VBO
	//   glGenBuffers(1, &treeNormalsVBO);
	//   glBindBuffer(GL_ARRAY_BUFFER, treeNormalsVBO);
	//   glBufferData(GL_ARRAY_BUFFER, objNormalstree.size() * sizeof(glm::vec3),
	//       &objNormalstree[0], GL_STATIC_DRAW);
	//   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//   glEnableVertexAttribArray(1);

	//   // uvs VBO
	//   glGenBuffers(1, &treeUVVBO);
	//   glBindBuffer(GL_ARRAY_BUFFER, treeUVVBO);
	//   glBufferData(GL_ARRAY_BUFFER, objUVstree.size() * sizeof(glm::vec2),
	//       &objUVstree[0], GL_STATIC_DRAW);
	//   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	//   glEnableVertexAttribArray(2);
	//*/

	glGenVertexArrays(1, &leavesVAO);
	glBindVertexArray(leavesVAO);

	// vertex VBO
	glGenBuffers(1, &leavesVerticiesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, leavesVerticiesVBO);
	glBufferData(GL_ARRAY_BUFFER, objVerticesleaves.size() * sizeof(glm::vec3),
		&objVerticesleaves[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	//normals VBO
	glGenBuffers(1, &leavesNormalsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, leavesNormalsVBO);
	glBufferData(GL_ARRAY_BUFFER, objNormalsleaves.size() * sizeof(glm::vec3),
		&objNormalsleaves[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);

	// uvs VBO
	glGenBuffers(1, &leavesUVVBO);
	glBindBuffer(GL_ARRAY_BUFFER, leavesUVVBO);
	glBufferData(GL_ARRAY_BUFFER, objUVsleaves.size() * sizeof(glm::vec2),
		&objUVsleaves[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(2);
}

void free()
{
	delete segment;
	delete skeleton;
	delete skeletonSkin;

	glDeleteBuffers(1, &surfaceVAO);
	glDeleteVertexArrays(1, &surfaceVerticesVBO);
	glDeleteVertexArrays(1, &surfacesBoneIndecesVBO);

	glDeleteVertexArrays(1, &maleBoneIndicesVBO);

	//   glDeleteBuffers(1, &triangleVerticiesVBO);
	//   glDeleteBuffers(1, &triangleNormalsVBO);
	//   glDeleteVertexArrays(1, &triangleVAO);
	//	  glDeleteBuffers(1, &triangleColorsVBO);


	//   glDeleteBuffers(1, &treeVerticiesVBO);
	//   glDeleteBuffers(1, &treeUVVBO);
	//   glDeleteBuffers(1, &treeNormalsVBO);
	//   glDeleteVertexArrays(1, &treeVAO);
	glDeleteBuffers(1, &planeVerticiesVBO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeColorsVBO);


	glDeleteBuffers(1, &leavesVerticiesVBO);
	glDeleteBuffers(1, &leavesUVVBO);
	glDeleteBuffers(1, &leavesNormalsVBO);
	glDeleteVertexArrays(1, &leavesVAO);

	glDeleteTextures(1, &diffuseTexturetree);
	glDeleteTextures(1, &diffuseTextureleaves);
	glDeleteProgram(shaderProgram);
	glfwTerminate();
}

void mainLoop()
{
	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		// camera
		camera->update();

		// first segment
		//segment->bind();

		mat4 bone1 = mat4(1);
		glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &bone1[0][0]);

		// draw segment
		//segment->draw(GL_LINES);
		//segment->draw(GL_POINTS);


		mat4 bone2 = mat4(1)*translate(mat4(), vec3(0, 0.5, 0));
		glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &bone2[0][0]);

		// draw segment
		//segment->draw(GL_LINES);
		//segment->draw(GL_POINTS);

		glBindVertexArray(planeVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseTextureleaves);
		glUniform1i(diffuceColorSampler, 0);
		//glDrawArrays(GL_TRIANGLES, 0, 6);

		// bind
		//glBindVertexArray(treeVAO);
		mat4 projectionMatrix = camera->projectionMatrix;
		mat4 viewMatrix = camera->viewMatrix;
		mat4 scale = glm::scale(mat4(), vec3(0.1, 0.1, 0.1));
		mat4 translation = translate(mat4(), vec3(0, 0, 0));

		glm::mat4 modelMatrix = glm::mat4(1.0) * translation * scale;
		/////////////////////////////////////////////////////////////////////////////////////////////LAB6

		map<int, float> q;
		t += 0.4;
		q[CoordinateName::BONE1_TRA_X] = t;
		q[CoordinateName::BONE1_TRA_Y] = t;
		q[CoordinateName::BONE1_TRA_Z] = t;
		q[CoordinateName::PELVIS_ROT_X] = t;
		q[CoordinateName::PELVIS_ROT_Y] = t;
		q[CoordinateName::PELVIS_ROT_Z] = t;
		q[CoordinateName::HIP_R_FLEX] = t;
		q[CoordinateName::HIP_R_ADD] = t;
		q[CoordinateName::HIP_R_ROT] = t;
		q[CoordinateName::KNEE_R_FLEX] = t;
		q[CoordinateName::ANKLE_R_FLEX] = t;
		q[CoordinateName::HIP_L_FLEX] = t;
		q[CoordinateName::HIP_L_ADD] = t;
		q[CoordinateName::HIP_L_ROT] = t;
		q[CoordinateName::KNEE_L_FLEX] = t;
		q[CoordinateName::ANKLE_L_FLEX] = t;
		q[CoordinateName::LUMBAR_FLEX] = t;
		q[CoordinateName::LUMBAR_BEND] = t;
		q[CoordinateName::LUMBAR_ROT] = t;

		auto jointLocalTransformations = calculateModelPoseFromCoordinates(q);
		skeleton->setPose(jointLocalTransformations);


		glUniform1i(useSkinningLocation, 1);
		uploadMaterial(boneMaterial);
		skeleton->draw(viewMatrix, projectionMatrix);

		skeletonSkin->bind();

		glUniform1i(useSkinningLocation, 1);

		mat4 maleModelMatrix = glm::mat4(1.0) * translation * scale;
		glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &maleModelMatrix[0][0]);
		glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
		glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

		// Task 4.2: calculate the bone transformations
		auto T = calculateSkinningTransformations(q);
		glUniformMatrix4fv(boneTransformationsLocation, T.size(),
			GL_FALSE, &T[0][0][0]);

		glUniform1i(useSkinningLocation, 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseTexturetree);
		glUniform1i(diffuceColorSampler, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularTexturetree);
		glUniform1i(specularColorSampler, 1);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//for trunk
		skeletonSkin->draw();
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//for leaves
		//*/

		//	glfwSwapBuffers(window);
		//	glfwPollEvents();
		//} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		//			glfwWindowShouldClose(window) == 0);
		/////////////////////////////////////////////////////////////////////////////////////////////////


		// Task 1.4c: transfer uniforms to GPU
		//glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
		//glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
		//glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

		//// Task 6.4: bind textures and transmit diffuse and specular maps to the GPU
		////*/
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, diffuseTexturetree);
		//glUniform1i(diffuceColorSampler, 0);

		//glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_2D, specularTexturetree);
		//glUniform1i(specularColorSampler, 1);
		////*/

		//// draw
		//glDrawArrays(GL_TRIANGLES, 0, objVerticestree.size());

		///////////////////////////////////////////////////////////////////////

		// bind
		glBindVertexArray(leavesVAO);

		GLuint a;
		// Task 6.4: bind textures and transmit diffuse and specular maps to the GPU
		//*/
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, a = (glfwGetKey(window, GLFW_KEY_SPACE) != GLFW_PRESS) ? diffuseTextureleaves : diffuseTexturetree); //dokimh ths glfwGetKey
		glUniform1i(diffuceColorSampler, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularTextureleaves);
		glUniform1i(specularColorSampler, 1);
		//*/

		// draw
		glDrawArrays(GL_TRIANGLES, 0, objVerticesleaves.size());
		glfwSwapBuffers(window);

		glfwPollEvents();
	} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);
}

void initialize()
{
	// Initialize GLFW
	if (!glfwInit())
	{
		throw runtime_error("Failed to initialize GLFW\n");
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);
	if (window == NULL)
	{
		glfwTerminate();
		throw runtime_error(string(string("Failed to open GLFW window.") +
			" If you have an Intel GPU, they are not 3.3 compatible." +
			"Try the 2.1 version.\n"));
	}
	glfwMakeContextCurrent(window);

	// Start GLEW extension handler
	glewExperimental = GL_TRUE;

	// Initialize GLEW
	if (glewInit() != GLEW_OK)
	{
		glfwTerminate();
		throw runtime_error("Failed to initialize GLEW\n");
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Hide the mouse and enable unlimited movement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, W_WIDTH / 2, W_HEIGHT / 2);

	// Gray background color
	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	glEnable(GL_PROGRAM_POINT_SIZE);

	// enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// enable textures
	glEnable(GL_TEXTURE_2D);

	// Log
	logGLParameters();

	// Create camera
	camera = new Camera(window);
}

int main(void)
{
	try
	{
		initialize();
		createContext();
		mainLoop();
		free();
	}
	catch (exception& ex)
	{
		cout << ex.what() << endl;
		getchar();
		free();
		return -1;
	}

	return 0;
}


void defineJointPoints()
{
	for (int i = 0; i < 9; i++)
		treeJoints.push_back(vec3(0, (float) i/2., 0));//9 points

	treeJoints.push_back(vec3((7, 15, -10)*0.1));
	treeJoints.push_back(vec3((-10, 16, 0)*0.1));
	treeJoints.push_back(vec3((8, 18, 7)*0.1));
	treeJoints.push_back(vec3((0, 18, -7)*0.1));
	treeJoints.push_back(vec3((-6, 20, 8)*0.1));
	treeJoints.push_back(vec3((8, 22, -2)*0.1));
	treeJoints.push_back(vec3((-8, 24, -5)*0.1));
	treeJoints.push_back(vec3((3, 25, 8)*0.1));
	treeJoints.push_back(vec3((3, 26, -8)*0.1));
	treeJoints.push_back(vec3((-7, 27, 4)*0.1));
	treeJoints.push_back(vec3((7, 29, 2)*0.1));
	treeJoints.push_back(vec3((-4, 32, -8)*0.1));
	treeJoints.push_back(vec3((-1, 33, 8)*0.1));
	treeJoints.push_back(vec3((5, 35, -5)*0.1));
	treeJoints.push_back(vec3((-6, 35, -1)*0.1));
	treeJoints.push_back(vec3((3, 36, 1)*0.1));
	treeJoints.push_back(vec3((0, 37, -4)*0.1));
	treeJoints.push_back(vec3((-4, 40, 3)*0.1));
	treeJoints.push_back(vec3((4, 42, 0)*0.1));
	treeJoints.push_back(vec3((-3, 43, -3)*0.1));
}

//Quicksort algorithm from low to high
void quickSort(std::vector<float> &arr, std::vector<int> &indices, int left, int right)
{
	int i = left, j = right;
	float tmp1;
	int tmp2;
	float pivot = arr[(left + right) / 2];



	/* partition */

	while (i <= j)
	{
		while (arr[i] < pivot)	i++;

		while (arr[j] > pivot)	j--;


		if (i <= j)
		{
			tmp1 = arr[i];
			arr[i] = arr[j];
			arr[j] = tmp1;


			tmp2 = indices[i];
			indices[i] = indices[j];
			indices[j] = tmp2;

			i++;
			j--;
		}
	};



	/* recursion */

	if (left < j)
		quickSort(arr, indices, left, j);

	if (i < right)
		quickSort(arr, indices, i, right);

}
