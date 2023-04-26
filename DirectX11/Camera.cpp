#include "Camera.h"
Camera::Camera() 
{
	XMStoreFloat3(&m_vecPosition, { 0.0f, 1.0f, 0.0f });
	XMStoreFloat3(&m_axisRight, { 1.0f, 0.0f, 0.0f });
	XMStoreFloat3(&m_axisUp, { 0.0f, 1.0f, 0.0f });
	XMStoreFloat3(&m_axisForward, { 0.0f, 0.0f, 1.0f });
	SetAspectRatio(1.0f);
	SetCamLens();
}

Camera::Camera(float _aspectRatio)
{
	XMStoreFloat3(&m_vecPosition, { 0.0f, 2.0f, -0.25f });
	XMStoreFloat3(&m_axisRight, { 1.0f, 0.0f, 0.0f });
	XMStoreFloat3(&m_axisUp, { 0.0f, 1.0f, 0.0f });
	XMStoreFloat3(&m_axisForward, { 0.0f, 0.0f, 1.0f });
	XMStoreFloat4x4(&m_vMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_pMatrix, XMMatrixIdentity());

	SetAspectRatio(_aspectRatio);
	SetCamLens();
}

Camera::~Camera()
{
}

float Camera::GetCamMoveSpeed()
{
	return m_camMoveSpeed;
}

void Camera::SetCamMoveSpeed(float speed)
{
	m_camMoveSpeed = speed;
}

float Camera::GetMouseXSensitivity()
{
	return m_mouseXSensitivity;
}

float Camera::GetMouseYSensitivity()
{
	return m_mouseYSensitivity;
}

float Camera::GetRightStickXSensitivity()
{
	return m_rightStickXSensitivity;
}

float Camera::GetRightStickYSensitivity()
{
	return m_rightStickYSensitivity;
}

void Camera::SetMouseXSensitivity(float _val)
{
	m_mouseXSensitivity = _val;
}

void Camera::SetMouseYSensitivity(float _val)
{
	m_mouseYSensitivity = _val;
}

void Camera::SetRightStickXSensitivity(float _val)
{
	m_rightStickXSensitivity = _val;
}

void Camera::SetRightStickYSensitivity(float _val)
{
	m_rightStickYSensitivity = _val;
}

XMVECTOR Camera::GetPositionXM() const
{
	return XMLoadFloat3(&m_vecPosition);
}

XMFLOAT3 Camera::GetPosition() const
{
	return m_vecPosition;
}

void Camera::SetPosition(float _x, float _y, float _z)
{
	m_vecPosition = { _x, _y, _z };
}

void Camera::SetPosition(const XMFLOAT3& _v)
{
	m_vecPosition = _v;
}

XMVECTOR Camera::GetRightXM() const
{
	return XMLoadFloat3(&m_axisRight);
}

XMFLOAT3 Camera::GetRight() const
{
	return m_axisRight;
}

XMVECTOR Camera::GetUpXM() const
{
	return XMLoadFloat3(&m_axisUp);
}

XMFLOAT3 Camera::GetUp() const
{
	return m_axisUp;
}

XMVECTOR Camera::GetForwardXM() const
{
	return XMLoadFloat3(&m_axisForward);
}

XMFLOAT3 Camera::GetForward() const
{
	return m_axisForward;
}

XMFLOAT4X4 Camera::GetViewMatrix()
{
	return m_vMatrix;
}

XMFLOAT4X4 Camera::GetPerspectiveMatrix()
{
	return m_pMatrix;
}

void Camera::Strafe(float _distance)
{
	XMVECTOR step = XMVectorReplicate(_distance);		// Multiplier vector (all components == _distance)
	XMVECTOR right = XMLoadFloat3(&m_axisRight);		// Multiplicand vecor
	XMVECTOR pos = XMLoadFloat3(&m_vecPosition);		// Addend vector
	XMStoreFloat3(&m_vecPosition, XMVectorMultiplyAdd(step, right, pos));	// Returns the product-sum of vecs
}

void Camera::Walk(float _distance)
{
	XMVECTOR step = XMVectorReplicate(_distance);		// Multiplier vector (all components == _distance)
	XMVECTOR forward = XMLoadFloat3(&m_axisForward);	// Multiplicand vecor
	XMVECTOR pos = XMLoadFloat3(&m_vecPosition);		// Addend vector
	XMStoreFloat3(&m_vecPosition, XMVectorMultiplyAdd(step, forward, pos));	// Returns the product-sum of vecs
}

void Camera::Slide(float _distance)
{
	XMStoreFloat3(&m_vecPosition, XMVector3Transform(XMLoadFloat3(&m_vecPosition), XMMatrixTranslation(0, 1 * _distance, 0)));
}

void Camera::PitchX(float _angle)
{
	// Builds a matrix that rotates AROUND an arbitrary axis.
	XMMATRIX rotationX = XMMatrixRotationAxis(XMLoadFloat3(& m_axisRight), _angle);
	
	// Transforms the up and forward basis vectors by above transformation matrix
	XMStoreFloat3(&m_axisUp, XMVector3TransformNormal(XMLoadFloat3(&m_axisUp), rotationX));
	XMStoreFloat3(&m_axisForward, XMVector3TransformNormal(XMLoadFloat3(&m_axisForward), rotationX));
}

void Camera::YawY(float _angle)
{
	// Builds a matrix that rotates AROUND an arbitrary axis.
	//XMMATRIX rotationY = XMMatrixRotationAxis(XMLoadFloat3(&m_axisUp), _angle);	// THIS IS LOCAL, NO-NO
	XMFLOAT3 globalUp(0, 1, 0);	// We want to yaw around the GLOBAL up
	XMMATRIX rotationY = XMMatrixRotationAxis(XMLoadFloat3(&globalUp), _angle);

	// Transforms all basis vectors by above transformation matrix
	XMStoreFloat3(&m_axisRight, XMVector3TransformNormal(XMLoadFloat3(&m_axisRight), rotationY));
	XMStoreFloat3(&m_axisUp, XMVector3TransformNormal(XMLoadFloat3(&m_axisUp), rotationY));
	XMStoreFloat3(&m_axisForward, XMVector3TransformNormal(XMLoadFloat3(&m_axisForward), rotationY));
}

void Camera::SetAspectRatio(float _ratio)
{
	m_aspectRatio = _ratio;
}

void Camera::UpdateViewMatrix()
{
	// We will orthonormalize the 3 basis vectors using a similar process to gram-Schmidt
	XMVECTOR right = XMLoadFloat3(&m_axisRight);
	XMVECTOR up = XMLoadFloat3(&m_axisUp);
	XMVECTOR forward = XMLoadFloat3(&m_axisForward);
	XMVECTOR pos = XMLoadFloat3(&m_vecPosition);

	// Normalize look vector
	forward = XMVector3Normalize(forward);

	// Compute the new "up" and also normalize it
	up = XMVector3Normalize(XMVector3Cross(forward, right));

	// Compute the new "right". Will already output as normalized 
	// because forward and up are both normalized
	right = XMVector3Cross(up, forward);

	// Grab all info(x,y,z, and 3x3 portion) for the new view matrix
	// Dot product returns result replicated across vector, so only need to one component
	float x = -XMVectorGetX(XMVector3Dot(pos, right));	
	float y = -XMVectorGetX(XMVector3Dot(pos, up));
	float z = -XMVectorGetX(XMVector3Dot(pos, forward));

	// Assemble axis
	XMStoreFloat3(&m_axisRight, right);
	XMStoreFloat3(&m_axisUp, up);
	XMStoreFloat3(&m_axisForward, forward);

	// Finally assemble the view matrix!!
	// Transposing 3x3 portion right here
	XMMATRIX temp(m_axisRight.x, m_axisUp.x, m_axisForward.x, 0.0f,
				  m_axisRight.y, m_axisUp.y, m_axisForward.y, 0.0f,
				  m_axisRight.z, m_axisUp.z, m_axisForward.z, 0.0f,
				  x, y, z, 1.0f);

	// Store new view matrix
	temp.r[2].m128_f32[3] = 0.0f;
	XMStoreFloat4x4(&m_vMatrix, temp);
}

void Camera::SetCamLens()
{
	XMStoreFloat4x4(&m_pMatrix,
		XMMatrixPerspectiveFovLH(m_verticalFOV, m_aspectRatio, m_nearZ, m_farZ));
}
