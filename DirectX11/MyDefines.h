#pragma once
#include <DirectXMath.h>
#include <vector>		//resizable array
#include "FSLogo.h"

// This reads .h2b files which are optimized binary .obj+.mtl files
#include "h2bParser.h"
using namespace DirectX;

// Simple basecode showing how to create a window and attatch a d3d11surface
#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries
#define GATEWARE_ENABLE_MATH
#define GATEWARE_ENABLE_INPUT
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

/*
There are 2 types: STORAGE TYPES, REGISTER/COMPUTATION TYPES.
*/

/*
Use storage variables for non-temporary variables. EX: Class Members
ALL math functions start with XM and ONLY WORK ON REGISTER/COMPUTATION TYPES.
*/

/*
STORAGE->REGISTER(locally)->STORAGEs
Convert storage types to register types[XMLOAD], do the computations, and then convert back to storage types[XMSTORE].
THIS MEANS that register/computation types are typically only declared locally to where they will be used.
*/

/*
CONVERSION:
Use XMLOAD[Type] to load a storage type into a register type.
Use XMSTORE[Type] to store a register type into a storage type.

..Data member
XMFLOAT3 normal(1,5,7)
...

somewhere in function...
// Step 1 convert to high performance type
XMVECTOR tempVec = XMLoadFloat3( &normal );
// Step 2 perform all required calculations
tempVec = XMVector3Normalize( tempVec );
// Step 3 convert back to storage type
XMStoreFloat3( &normal, tempVec );
// "normal" is now actually normalized
return 0;


*/

//////////////////////// Members ////////////////////////

UINT m_windowWidth = 800;
UINT m_windowHeight = 600;
float m_targetFPS = 60.0f;
float m_frameTime = 1.0f / m_targetFPS;

UINT m_gridDensity = 25;			// 25 is default
XMFLOAT4 m_origSunlightColor = { 0.9f, 0.9f, 1.0f, 1.0f };
XMFLOAT3 m_originalSunlightDirection = { -1, -1, 2 };

//////////////////////// Structs ////////////////////////

struct MY_VERTEX
{
	XMFLOAT3 pos;
	XMFLOAT3 uvw;
	XMFLOAT3 nrm;
	//XMFLOAT4 rgba;
};

struct CB_PerObject
{
	XMFLOAT4X4 wMatrix;					// 64 bytes
	XMFLOAT4X4 vMatrix;					// 64 bytes
	XMFLOAT4X4 pMatrix;					// 64 bytes

	//Assignemnt 2
	//_OBJ_ATTRIBUTES_ currOBJAttributes;	// ?
	
	H2B::ATTRIBUTES currOBJAttributes;
};

struct CB_PerFrame
{
	XMFLOAT4 lightColor;		// 16 bytes
	XMFLOAT3 lightDirection;	// 12 bytes
	float padding;
};

struct Clock
{
	std::chrono::steady_clock clock;
	std::chrono::time_point<std::chrono::steady_clock> start;
	std::chrono::time_point<std::chrono::steady_clock> now;
	std::chrono::milliseconds duration;

	Clock()
	{
		// Start the clock
		start = now = clock.now();
	}
};

unsigned int Convert2D1D(unsigned int _x, unsigned int _y, unsigned int _width)
{
	return _x + (_y * _width);
}

void Convert1D2D(unsigned int _fromValue, unsigned int& _xResult, unsigned int& _yResult)
{
	// This is intended to create indexes for a 4x4 matrix
	unsigned int x = 0;
	unsigned int y = 0;
	if (_fromValue <= 3)
		y = 0;
	else if (_fromValue <= 7)
		y = 1;
	else if (_fromValue <= 11)
		y = 2;
	else if (_fromValue <= 15)
		y = 3;

	x = _fromValue % 4;
	_xResult = x;
	_yResult = y;

}

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




