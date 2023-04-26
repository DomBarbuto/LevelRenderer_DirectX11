#pragma once
#include <DirectXMath.h>
#include <iostream>
using namespace DirectX;

class Camera
{
public:
	Camera();
	Camera(float _aspectRatio);
	~Camera();

	float GetCamMoveSpeed();
	float GetMouseXSensitivity();
	float GetMouseYSensitivity();
	float GetRightStickXSensitivity();
	float GetRightStickYSensitivity();
	void SetMouseXSensitivity(float _val);
	void SetMouseYSensitivity(float _val);
	void SetRightStickXSensitivity(float _val);
	void SetRightStickYSensitivity(float _val);
	
	// Getters/Setters for world cam position
	XMVECTOR GetPositionXM() const;
	XMFLOAT3 GetPosition() const;
	void SetPosition(float _x, float _y, float _z);
	void SetPosition(const XMFLOAT3& _v);

	// Get all basis vectors more either storing or manipulating
	XMVECTOR GetRightXM() const;
	XMFLOAT3 GetRight() const;
	XMVECTOR GetUpXM() const;
	XMFLOAT3 GetUp() const;
	XMVECTOR GetForwardXM() const;
	XMFLOAT3 GetForward() const;

	XMFLOAT4X4 GetViewMatrix();
	XMFLOAT4X4 GetPerspectiveMatrix();

	// Move along camera x axis
	void Strafe(float _distance);
	// Move along camera z axis
	void Walk(float _distance);
	// Move along camera y axis
	void Slide(float _distance);

	// Look up/down, rotating y and z axis around x axis
	void PitchX(float _angle);
	// Roate left/right, rotating all basis vectors around the global Y Axis
	void YawY(float _angle);
	
	// Set aspect ratio
	void SetAspectRatio(float _ratio);
	// Assemble the view matrix
	void UpdateViewMatrix();
	// Assemble persepective matrix
	void SetCamLens();

private:

	float m_aspectRatio;
	float m_verticalFOV = XMConvertToRadians(65.0f);	// 65 degrees == default
	float m_nearZ = 0.1f;
	float m_farZ = 2000;

	// Camera controls
	float m_camMoveSpeed = 0.01f;
	float m_mouseXSensitivity = 0.8f;
	float m_mouseYSensitivity = 0.8f;
	float m_rightStickXSensitivity = 8.0f;
	float m_rightStickYSensitivity = 8.0f;

	XMFLOAT3 m_vecPosition;
	XMFLOAT3 m_axisRight;
	XMFLOAT3 m_axisUp;
	XMFLOAT3 m_axisForward;

	XMFLOAT4X4 m_vMatrix;
	XMFLOAT4X4 m_pMatrix;
};

