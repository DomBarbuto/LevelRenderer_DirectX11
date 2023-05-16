#pragma once
#include <DirectXMath.h>
#include <vector>		//resizable array

// This reads .h2b files which are optimized binary .obj+.mtl files
#include "h2bParser.h"
using namespace DirectX;

// Simple basecode showing how to create a window and attatch a d3d11surface
#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries
#define GATEWARE_ENABLE_MATH
#define GATEWARE_ENABLE_INPUT
#define GATEWARE_ENABLE_AUDIO
// Ignore some GRAPHICS libraries we aren't going to use
#define GATEWARE_DISABLE_GDIRECTX12SURFACE // we have another template for this
#define GATEWARE_DISABLE_GRASTERSURFACE // we have another template for this
#define GATEWARE_DISABLE_GOPENGLSURFACE // we have another template for this
#define GATEWARE_DISABLE_GVULKANSURFACE // we have another template for this
// With what we want & what we don't defined we can include the API
#include "../gateware-main/Gateware.h"
#include "FileIntoString.h"
#include "Camera.h"
// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

//////////////////////// Members ////////////////////////
UINT m_windowWidth = 1080;//1080;
UINT m_windowHeight = 720;// 720;
double m_targetFPS = 30.0;
double m_frameTime = 1.0 / m_targetFPS;
float m_camMoveSpeedOG = 0.01f;

/////////////////////// LIGHTING /////////////////////////
// Directional Light
XMFLOAT4 m_origSunlightColor = { 0.9f, 0.9f, 1.0f, 1.0f };
XMFLOAT3 m_originalSunlightDirection = { -1, -2, -2 };
// Point lights
const UINT m_maxPointLights = 16;
// Spot lights
const UINT m_maxSpotLights = 16;

//////////////////////// MISC ////////////////////////////
UINT m_gridDensity = 25;			// 25 is default

//////////////////////// Structs ////////////////////////
struct MY_VERTEX
{
	XMFLOAT3 pos;
	XMFLOAT3 uvw;
	XMFLOAT3 nrm;
};

struct PerInstanceData
{
	XMFLOAT4X4 wMatrix;
};

struct POINT_LIGHT
{
	GW::MATH::GMATRIXF transform;
	XMFLOAT4 color;
	float energy;
	float distance;
	float q_attenuation;
	float l_attenuation;
};

struct SPOT_LIGHT
{
	GW::MATH::GMATRIXF transform;
	XMFLOAT4 color;
	float energy;
	float distance;
	float q_attenuation;
	float l_attenuation;
	float spotSize;
	float spotBlend;
	float padding1, padding2;
};

// All constant buffer structs need to be 16 byte aligned
struct CB_PerScene
{
	H2B::ATTRIBUTES currOBJAttributes[40];	// ?
	float numPointLights;
	float numSpotLights;
	float pad1;
	float pad2;
};

struct CB_PerObject
{
	XMFLOAT4X4 vMatrix;					// 64 bytes
	XMFLOAT4X4 pMatrix;					// 64 bytes
	XMFLOAT4 materialIndex;				// Replicated for byte-align
};

struct CB_PerFrame
{
	XMFLOAT4 dirLight_Color;		// 16 bytes
	XMFLOAT3 dirLight_Direction;	// 12 bytes
	float time;
	POINT_LIGHT pointLights[m_maxPointLights];
	SPOT_LIGHT spotLights[m_maxSpotLights];
	SPOT_LIGHT cameraFlashlight;
	XMFLOAT4 flashlightPowerOn;

};

static struct Clock
{
	std::chrono::high_resolution_clock clock;
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::chrono::time_point<std::chrono::high_resolution_clock> stop;

	double GetMSElapsed()
	{
		auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start);
		return elapsed.count();
	}

	void Start()
	{
		start = stop = clock.now();
	}

	void Restart()
	{
		start = std::chrono::high_resolution_clock::now();
	}

};


//////////////////////// Geometry ////////////////////////

/// <summary>
/// The grid will span -0.5f to 0.5f on X and Y planes in NDC
/// </summary>
/// <param name="_density">Number of squares for rows and columns. 
/// EX: Density of 25 == 25 rows X 25 cols == 26 lines X 26 lines.</param>
/// <returns></returns>
//std::vector<MY_VERTEX> CREATE_GRID()
//{
//	std::vector<MY_VERTEX> temp;
//	float step = 1.0f / m_gridDensity;	// With density 25 (default), step is 0.04f
//	XMFLOAT4 origColor(1, 1, 1, 1);		// White
//
//	// Rows. Top to bottom, plotting each vertex left and right sides
//	for (float row = 0.5f; row >= -0.5f; row -= step)
//	{
//		temp.push_back({ {-0.5f, row, 0, 1}, {origColor} });	// Left vertex
//		temp.push_back({ {0.5f, row, 0, 1}, {origColor} });		// Right vertex
//	}
//	// Cols. Left to right, plotting each vertex top and bottom sides
//	for (float col = -0.5f; col <= 0.5f; col += step)
//	{
//		temp.push_back({ {col, 0.5f, 0, 1}, {origColor} });		// Top vertex
//		temp.push_back({ {col, -0.5f, 0, 1}, {origColor} });	// Bottom vertex
//	}
//
//	return temp;
//}




