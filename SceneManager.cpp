///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declare the global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0, height = 0, colorChannels = 0;
	GLuint textureID = 0;

	// Flip image when loading
	stbi_set_flip_vertically_on_load(true);

	// Load image
	unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);
	if (!image) {
		std::cout << "Failed to load texture:" << filename << std::endl;
		return false;
	}

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Apply specific wrapping mode based on texture tag
	if (tag == "panda" || tag == "thinkpad")  // Ensure
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// Texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Upload texture data to always use RGBA if it needs
	GLenum format = (colorChannels == 4) ? GL_RGBA : GL_RGB; // needed to load RGBA for transparent textures
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	// Free image and unbind
	stbi_image_free(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Register texture
	m_textureIDs[m_loadedTextures].ID = textureID;
	m_textureIDs[m_loadedTextures].tag = tag;
	m_loadedTextures++;

	return true;
}


/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		if (m_textureIDs[i].ID != 0) {
			glDeleteTextures(1, &m_textureIDs[i].ID);
			m_textureIDs[i].ID = 0; // Ensure it's reset
		}
	}
	m_loadedTextures = 0; // Reset texture count
}


/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}


/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		// find the defined material that matches the tag
		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			// pass the material properties into the shader
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}


/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** Plastic Material ***/
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);  // Light blue
	plasticMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);  // Increased reflection
	plasticMaterial.shininess = 300.0f;  // Shinier surface
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	/*** Hard Plastic Material ***/
	OBJECT_MATERIAL hardplasticMaterial;
	hardplasticMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);  // Very dark grey
	hardplasticMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Moderate reflectivity
	hardplasticMaterial.shininess = 150.0f;  // Slightly less than plastic but still semi-glossy
	hardplasticMaterial.tag = "hardplastic";
	m_objectMaterials.push_back(hardplasticMaterial);

	/*** Wood Material (Matte Finish) ***/
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.55f, 0.27f, 0.07f);  // Warm brown
	woodMaterial.specularColor = glm::vec3(0.1f, 0.05f, 0.02f);  // Minimal shine
	woodMaterial.shininess = 20.0f;  // A slight finish
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	/*** Silicone Material (Matte & Rubber-like) ***/
	OBJECT_MATERIAL siliconeMaterial;
	siliconeMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);  // Soft white
	siliconeMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);  // Rubber-like sheen
	siliconeMaterial.shininess = 2.0f;  // Slightly shinier, but still mostly matte
	siliconeMaterial.tag = "silicone";
	m_objectMaterials.push_back(siliconeMaterial);

	/*** Rug Material (Soft & Matte) ***/
	OBJECT_MATERIAL rugMaterial;
	rugMaterial.diffuseColor = glm::vec3(0.65f, 0.45f, 0.3f);  // Warmer brown
	rugMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);  // Very low reflectivity
	rugMaterial.shininess = 1.0f;  // Matte surface
	rugMaterial.tag = "rug";
	m_objectMaterials.push_back(rugMaterial);

	/*** Wall Material (Soft Reflection) ***/
	OBJECT_MATERIAL wallMaterial;
	wallMaterial.diffuseColor = glm::vec3(0.55f, 0.55f, 0.55f);  // Light grey
	wallMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);  // More subtle but present
	wallMaterial.shininess = 5.0f;  // Minimal but adds a bit of light interaction
	wallMaterial.tag = "wall";
	m_objectMaterials.push_back(wallMaterial);

	/*** Metal Material (Highly Reflective) ***/
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);  // Neutral grey metal
	metalMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);  // Strong reflection
	metalMaterial.shininess = 300.0f;  // High shine, metallic look
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	/*** Window Material (Glass-like Reflection) ***/
	OBJECT_MATERIAL windowMaterial;
	windowMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);  // Very low diffuse to ensure transparency
	windowMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);  // Strong specular highlights
	windowMaterial.shininess = 500.0f;  // Glass-like effect
	windowMaterial.tag = "window";
	m_objectMaterials.push_back(windowMaterial);
}
/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting in shaders
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/***  Window Light ***/
	//Simulating Outdoor Light Source ***/
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(-110.0f, 50.0f, 20.0f));  // Move further outside
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.7f, 0.7f, 0.7f));  // Reduce ambient light for contrast
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.4f, 0.4f, 0.4f));  // Brighter direct lighting
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.5f, 0.5f, 0.5f));  // Stronger reflections
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.013f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.002f);

	// Recess Light - Softer Warm Light 
	m_pShaderManager->setVec3Value("pointLights[1].position", glm::vec3(30.0f, 30.0f, 0.0f));  // Centered above
	m_pShaderManager->setVec3Value("pointLights[1].ambient", glm::vec3(0.105f, 0.084f, 0.07f));  // Softer warm glow
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", glm::vec3(0.175f, 0.14f, 0.105f));  // Slightly stronger warm light
	m_pShaderManager->setVec3Value("pointLights[1].specular", glm::vec3(0.105f, 0.07f, 0.056f));  // Minimal reflections
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
	m_pShaderManager->setFloatValue("pointLights[1].constant", 0.5f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.015f);  // More gradual falloff
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.002f);  // Softer drop-off

	// Second Recess Light - Softer Warm Light 
	m_pShaderManager->setVec3Value("pointLights[2].position", glm::vec3(0.0f, 50.0f, 0.0f));  // Centered above
	m_pShaderManager->setVec3Value("pointLights[2].ambient", glm::vec3(0.105f, 0.084f, 0.07f));  // Softer warm glow
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", glm::vec3(0.175f, 0.14f, 0.105f));  // Slightly stronger warm light
	m_pShaderManager->setVec3Value("pointLights[2].specular", glm::vec3(0.105f, 0.07f, 0.056f));  // Minimal reflections
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
	m_pShaderManager->setFloatValue("pointLights[2].constant", 0.5f);
	m_pShaderManager->setFloatValue("pointLights[2].linear", 0.015f);  // More gradual falloff
	m_pShaderManager->setFloatValue("pointLights[2].quadratic", 0.002f);  // Softer drop-off


}



/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/keyboard.png",  // Selected Folder location for textures, went on google to find similar images with the same theme for each shape.
		"keyboard");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/thinkpad.png",
		"thinkpad");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/circular-brushed-gold-texture.jpg",
		"hinge");
	bReturn = CreateGLTexture("../5-2_Assignment/textures/wood1.jpg",
		"wood1");
	bReturn = CreateGLTexture("../5-2_Assignment/textures/wood2.jpg",
		"wood2");
	bReturn = CreateGLTexture("../5-2_Assignment/textures/couch.jpg",
		"couch");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/zipper.png",
		"zipper");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/panda.png",
		"panda");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/rug.jpg",
		"rug");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/screen.jpg",
		"screen");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/laptoptexture.jpg",
		"pctexture");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/i7logo.jpg",
		"i7");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/suitcase.jpg",
		"suitcase");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/window.png",
		"window");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/rusticwood.jpg",
		"rusticwood");
	bReturn = CreateGLTexture(
		"../5-2_Assignment/textures/whitewood.jpg",
		"whitewood");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Enable blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Load the textures for the 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	// Load models/meshes
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->DrawHalfSphereMesh();
	m_basicMeshes->DrawHalfTorusMesh();
	m_basicMeshes->DrawHalfSphereMeshLines();
}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(16.5f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); //changed colors to make the 3d object visible since it is also white
	SetShaderTexture("rusticwood");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	// Head (Sphere) - Apply Panda Face																							----------------------------------------------------------------------------------
	scaleXYZ = glm::vec3(1.51f, 1.13f, 1.51f);
	positionXYZ = glm::vec3(9.0f, 7.8f, -4.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawHalfSphereMesh();

	// Head (Cylinder) //																										
	scaleXYZ = glm::vec3(1.5f, 1.1f, 1.5f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 207.5f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(9.0f, 6.8f, -4.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderTexture("panda");
	SetTextureUVScale(3.0f, 1.0f);
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawCylinderMesh(false, false);

	// Left Ear (Cylinder) // 
	scaleXYZ = glm::vec3(0.55f, 0.3f, 0.55f); // Scale for the cylinder body																							HEAD SECTION
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(7.8f, 8.7f, -4.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // Black Ears
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawSphereMesh();

	// Right Ear (Cylinder) // 
	scaleXYZ = glm::vec3(0.55f, 0.3f, 0.55f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(10.3f, 8.7f, -4.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // Black Ears		
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawSphereMesh();

	// Neck Zipper (Cylinder) // 
	scaleXYZ = glm::vec3(1.4f, 1.0f, 1.4f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(9.0f, 6.4f, -4.0f); // Position on the table								
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // Black for the zipper/neck
	SetShaderTexture("zipper");
	SetTextureUVScale(3.0, 3.0);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawCylinderMesh();

	// Body (Cylinder) // 
	scaleXYZ = glm::vec3(1.5f, 5.0f, 1.5f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(9.0f, 1.5f, -4.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawCylinderMesh();

	// Base Tapered (Cylinder) // 
	scaleXYZ = glm::vec3(1.5f, 0.7f, 1.5f); // Scale for the cylinder body
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(9.0f, 1.5f, -4.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Body after tapered (Cylinder) // 
	scaleXYZ = glm::vec3(1.3f, 0.8f, 1.3f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(9.0f, 1.2f, -4.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawCylinderMesh();

	// Base Tapered 2nd (Cylinder) // 
	scaleXYZ = glm::vec3(1.3f, 1.0f, 1.3f); // Scale for the cylinder body
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(9.0f, 1.2f, -4.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Base (Cylinder) // 
	scaleXYZ = glm::vec3(1.0f, 0.6f, 1.0f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(9.0f, 0.3f, -4.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawCylinderMesh();


	// Base rounded edge (Torus) // 
	scaleXYZ = glm::vec3(0.83f, 0.83f, 0.83f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(9.0f, 0.3f, -4.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawTorusMesh();

	//wall in background ---****																						---------------------------------------------------------------
	scaleXYZ = glm::vec3(70.0f, 1.0f, 45.0f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 15.0f, -40.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f); // Dark grey
	SetShaderMaterial("wall");
	m_basicMeshes->DrawPlaneMesh();

	//wall in background behind camera---****																									ENVIRONMENT  OBJECTS 
	scaleXYZ = glm::vec3(70.0f, 1.0f, 45.0f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 15.0f, 30.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f); //
	SetShaderMaterial("wall");
	m_basicMeshes->DrawPlaneMesh();


	// To enable blending for transparency before drawing the window
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	scaleXYZ = glm::vec3(32.5f, 1.0f, 20.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-70.0f, 30.0f, -2.5f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.75f, 0.75f, 0.75f, 0.4f);  // Reduce alpha to make it transparent (0.0f = fully transparent, 1.0f = opaque)
	SetShaderMaterial("window");
	SetShaderTexture("window");  
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();
	glDisable(GL_BLEND); // Disables the blending after drawing the window


	//wall above window
	scaleXYZ = glm::vec3(5.0f, 1.0f, 35.0f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(-70.0f, 55.0f, -5.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.55f, 0.57f, 0.57f, 1.0f); // Dark grey
	SetShaderMaterial("wall");
	m_basicMeshes->DrawPlaneMesh();

	//wall under window---****																						---------------------------------------------------------------
	scaleXYZ = glm::vec3(20.0f, 1.0f, 35.0f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(-70.0f, -10.0f, -5.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.63f, 0.63f, 0.63f, 1.0f); // Dark grey
	SetShaderMaterial("wall");
	m_basicMeshes->DrawPlaneMesh();

	//right wall--****																						---------------------------------------------------------------
	scaleXYZ = glm::vec3(45.0f, 1.0f, 35.0f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(70.0f, 15.0f, -5.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.64f, 0.64f, 0.64f, 1.0f); // Dark grey
	SetShaderMaterial("wall");
	m_basicMeshes->DrawPlaneMesh();

	//Ceiling																												BACKGROUND SCENARIO
	scaleXYZ = glm::vec3(35.0f, 1.0f, 70.0f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(0.0f, 60.0f, -05.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.65f, 0.65f, 0.65f, 1.0f); // Dark grey
	m_basicMeshes->DrawPlaneMesh();

	//floor rug																													BACKGROUND SCENARIO
	scaleXYZ = glm::vec3(35.0f, 1.0f, 70.0f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(0.0f, -30.0f, -05.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 1.0f, 1.0f, 1.0f); // Dark grey
	SetShaderTexture("rug");
	SetShaderMaterial("rug");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawPlaneMesh();

	//room wall mouldings																												--------------------------------------------------

	scaleXYZ = glm::vec3(139.7f, 0.6f, 2.5f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -28.7f, 29.6f); // Position on the table																					ROOM MOULDINGS
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.f, 1.0f, 1.0f, 1.0f); // Dark grey
	SetShaderMaterial("wall");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.6f, 2.5f, 65.0f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-70.0f, -28.7f, -3.0f); // Position on the table																					ROOM MOULDINGS
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.f, 1.0f, 1.0f, 1.0f); // Dark grey
	SetShaderMaterial("wall");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(9.7f, 0.6f, 2.5f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-14.9f, -28.7f, -39.5f); // Position on the table																					ROOM MOULDINGS
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.f, 1.0f, 1.0f, 1.0f); // Dark grey
	SetShaderMaterial("wall");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(29.9f, 0.6f, 2.5f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(54.96f, -28.7f, -39.5f); // Position on the table																					ROOM MOULDINGS
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.f, 1.0f, 1.0f, 1.0f); // Dark grey
	SetShaderMaterial("wall");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.6f, 2.5f, 69.7f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(69.68f, -28.7f, -4.98f); // Position on the table																					ROOM MOULDINGS
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.f, 1.0f, 1.0f, 1.0f); // Dark grey
	SetShaderMaterial("wall");
	m_basicMeshes->DrawBoxMesh();


	//WINDOWSILL

	scaleXYZ = glm::vec3(0.6f, 2.5f, 64.5f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-69.3f, 10.0f, -2.3f); // Position on the table																					BOTTOM SIL
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f); // Light gray for a "darker white"
	SetShaderMaterial("wall");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.6f, 2.5f, 64.5f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-69.3f, 50.0f, -2.3f); // Position on the table																					TOP SIL
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f); // Light gray for a "darker white"
	SetShaderMaterial("wall");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.6f, 40.5f, 2.5f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-69.3f, 30.0f, -33.3f); // Position on the table																					RIGHT SIL
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f); // Light gray for a "darker white"
	SetShaderMaterial("wall");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.6f, 40.5f, 2.5f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-69.3f, 30.0f, 28.7f); // Position on the table																					LEFT SIL
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f); // Light gray for a "darker white"
	SetShaderMaterial("wall");
	m_basicMeshes->DrawBoxMesh();







	///// TABLE //////																									------------------------------------------------------------------


	// table top
	scaleXYZ = glm::vec3(16.5f, 1.0f, 10.0f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -2.0f, 0.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	SetShaderTexture("wood2");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();



	//table right moulding
	scaleXYZ = glm::vec3(1.0f, 2.0f, 20.0f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(17.0f, -1.0f, 0.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("wood");
	SetShaderTexture("wood2");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	//table left moulding
	scaleXYZ = glm::vec3(1.0f, 2.0f, 20.0f); // Scale for the cylinder body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-17.0f, -1.0f, 0.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("wood");
	SetShaderTexture("wood2");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	//table front moulding
	scaleXYZ = glm::vec3(35.0f, 1.0f, 2.0f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -1.0f, 10.5f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("wood");
	SetShaderTexture("wood2");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();


	//table back moulding
	scaleXYZ = glm::vec3(35.0f, 1.0f, 2.0f); // Scale for the cylinder body
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -1.0f, -10.5f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("wood");
	SetShaderTexture("wood2");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	//


	//--------------TABLE LEGS--------------																																--------------------------------------------------------------------+
	scaleXYZ = glm::vec3(20.0f, 1.5f, 1.5f); // LEFT FRAME OF TABLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-15.75f, -2.8f, 0.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(20.0f, 1.5f, 1.5f); // RIGHT FRAME OF TABLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(15.75f, -2.8f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(30.0f, 1.5f, 1.5f); // FRONT FRAME OF TABLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 00.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -2.8f, 9.25f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(30.0f, 1.5f, 1.5f); // BACK FRAME OF TABLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 00.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -2.8f, -9.25f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(30.0f, 1.5f, 1.5f); // BACK FRAME OF INNER TABLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 00.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -2.8f, -3.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(1.5f, 26.3f, 1.5f); // INNER RIGHT LEG
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(15.75f, -16.7f, -3.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(1.5f, 26.3f, 1.5f); // OUTER RIGHT LEG
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(15.75f, -16.7f, -9.25f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(1.5f, 26.3f, 1.5f); // OUTER LEFT LEG
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-15.75f, -16.7f, -9.25f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(1.5f, 26.3f, 1.5f); // OUTER LEFT LEG
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-15.75f, -16.7f, -3.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(30.0f, 1.5f, 1.5f); // LOWER OUTER BACK FRAME OF TABLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 00.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.f, -29.2f, -9.25f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(30.0f, 1.5f, 1.5f); // LOWER BACK FRAME OF TABLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 00.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -29.2f, -3.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(20.0f, 1.5f, 1.5f); // LOWER RIGHT FRAME OF TABLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(15.75f, -29.2f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(20.0f, 1.5f, 1.5f); // LEFT FRAME OF TABLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-15.75f, -29.2f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(30.0f, 1.5f, 1.5f); // FRONT FRAME OF TABLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 00.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -29.2f, 9.25f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();




	//TV SCREEN 
	scaleXYZ = glm::vec3(50.0f, 25.0f, 0.8f); //																						TV CONSOLE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-40.0f, 24.5f, -34.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color 
	SetShaderTexture("pctexture");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(24.0f, 0.1f, 11.5f); //																							TV SCREEN ITSELF
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-40.0f, 24.5f, -33.59f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color 
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawPlaneMesh();


	scaleXYZ = glm::vec3(50.0f, 89.8f, 5.1f); //																						ENTERTAINMENT WALL
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-44.8f, 15.0f, -37.25f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color 
	SetShaderTexture("wood2");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();



	// LAPTOP OBJECT

	scaleXYZ = glm::vec3(22.0f, 0.8f, 12.5f); //																						Scale for base of laptop
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 0.5f, 0.8f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color 
	SetShaderTexture("pctexture");
	SetShaderMaterial("hardplastic");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(22.0f, 0.2f, 13.0f); //																			Secondary base for laptop
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 1.0f, 0.7f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderMaterial("hardplastic");
	SetShaderTexture("pctexture");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(10.0f, 0.15f, 3.5f); //																						Keyboard overlay
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 1.2f, -1.25f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, .0f); // White color for the body
	SetShaderMaterial("hardplastic");
	SetShaderTexture("keyboard");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.0f); //																							I7 Logo on laptop
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-13.0f, 1.2f, 5.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f); // black color 
	SetShaderTexture("i7");
	SetShaderMaterial("hardplastic");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(2.0f, 0.1f, 1.5f); //																							THINKPAD LOGO on laptop
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.5f, 1.2f, 5.5f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // black color
	SetShaderTexture("thinkpad");
	SetShaderMaterial("hardplastic");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(2.0f, 0.1f, 1.5f); //																							THINKPAD LOGO on laptop lid
	XrotationDegrees = 120.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.0f, 10.6f, -11.5f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // black color 
	SetShaderTexture("thinkpad");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(22.0f, 0.2f, 13.0f); //																						Laptop Screen Half with angle 
	XrotationDegrees = 60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 6.7f, -9.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderTexture("pctexture");
	SetShaderMaterial("hardplastic");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(10.5f, 0.2f, 5.5f); //																						      Laptop Screen itself
	XrotationDegrees = 60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 6.8f, -8.88f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the body
	SetShaderTexture("screen");
	SetShaderMaterial("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(0.4f, 1.0f, 0.4f); //																							   Right hinge for laptop
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(4.5f, 1.0f, -5.5f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // black color 
	SetShaderTexture("pctexture");
	SetShaderMaterial("metal");
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.4f, 1.0f, 0.4f); //																							      Left hinge for laptop
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(-11.0f, 1.0f, -5.5f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f); // black color 
	SetShaderMaterial("metal");
	SetShaderTexture("pctexture");
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(4.0f, 0.8f, 2.0f); //																									LAPTOP TOUCHPAD
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 1.15f, 4.7f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(3.25f, 0.1, 0.8f); //																									LAPTOP TOUCHPAD LEFT BUTTON
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-6.37f, 1.15f, 2.3f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // black color
	SetShaderTexture("pctexture");
	SetShaderMaterial("hardplastic");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(1.55f, 0.1, 0.05f); //																									LAPTOP TOUCHPAD LEFT BUTTON RED LINE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-6.37f, 1.21f, 2.65f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f); // black color 
	SetShaderMaterial("hardplastic");
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(3.25f, 0.1, 0.8f); //																									LAPTOP TOUCHPAD RIGHT BUTTON
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.63f, 1.15f, 2.3f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // black color 
	SetShaderTexture("pctexture");
	SetShaderMaterial("hardplastic");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(1.55f, 0.1, 0.05f); //																									LAPTOP TOUCHPAD RIGHT BUTTON RED LINE
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.63f, 1.21f, 2.65f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f); // black color 
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(1.5f, 0.1, 0.8f); //																									LAPTOP TOUCHPAD MIDDLE BUTTON
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.0f, 1.151f, 2.3f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color 
	SetShaderTexture("pctexture");
	SetShaderMaterial("hardplastic");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.8f, 0.1, 0.8f); //																									LAPTOP TOUCHPAD MIDDLE BUTTON EXTEND
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.25f, 1.151f, 2.3f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color 
	SetShaderTexture("pctexture");
	SetShaderMaterial("hardplastic");
	m_basicMeshes->DrawPrismMesh();

	scaleXYZ = glm::vec3(0.8f, 0.1, 0.8f); //																									LAPTOP TOUCHPAD MIDDLE BUTTON EXTEND
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.75f, 1.151f, 2.3f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color 
	SetShaderTexture("pctexture");
	SetShaderMaterial("hardplastic");
	m_basicMeshes->DrawPrismMesh();


	//MOUSE OBJECT


	scaleXYZ = glm::vec3(2.5f, 2.5f, 4.0f); //																											Mouse Body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(11.0f, 0.1f, 2.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color 
	m_basicMeshes->DrawHalfSphereMesh();

	scaleXYZ = glm::vec3(1.0f, 0.7f, 1.0f); //																											Mouse Scroll Wheel
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(11.3f, 1.7f, 0.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // black color 
	m_basicMeshes->DrawCylinderMesh();



	// COUCH IN BACKGROUND

	scaleXYZ = glm::vec3(50.0f, 3.0f, 20.0f); //																									CUSHION 1
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(15.0f, -10.7f, -29.8f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // black color 
	SetShaderTexture("couch");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(50.0f, 3.0f, 20.0f); //																									CUSHION 2
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(15.0f, 0.75f, -38.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 1.0f, 1.0f, 1.0f); // black color 
	SetShaderTexture("couch");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(50.0f, 17.5f, 19.5f); //																						COUCH BASE			
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(15.0f, -21.0f, -30.0f); // Position on the table
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 1.0f, 1.0f); // black color 
	SetShaderTexture("wood2");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();


	scaleXYZ = glm::vec3(17.0f, 8.0f, 23.0f); //																									
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(30.0f, -5.0f, -24.8f); // Position on the table															SUITCASE
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 1.0f, 1.0f); // black color 
	SetShaderTexture("suitcase");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(2.0f, 1.2f, 0.8f); //																									
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(26.0f, -2.4f, -13.0f); // Position on the table															SUITCASE LEFT NUB
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color 
	m_basicMeshes->DrawHalfSphereMesh();

	scaleXYZ = glm::vec3(2.0f, 1.2f, 0.8f); //																									
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(34.5f, -2.4f, -13.0f); // Position on the table															SUITCASE RIGHT NUB
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // black color 
	m_basicMeshes->DrawHalfSphereMesh();

	scaleXYZ = glm::vec3(17.5f, 1.5f, 23.5f); //																									
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(30.0f, -5.0f, -24.8f); //																	SUITCASE ZIPPER SECTION
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // 
	SetShaderMaterial("silicone");
	m_basicMeshes->DrawBoxMesh();
}
