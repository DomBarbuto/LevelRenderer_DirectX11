#include "load_data_oriented.h"
#include <d3dcompiler.h>
#include <commdlg.h>	// For open file dialog
#pragma comment(lib, "d3dcompiler.lib") //needed for runtime shader compilation. Consider compiling shaders before runtime 

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX11Surface d3d;
	GW::INPUT::GInput kbmInput;
	GW::INPUT::GController controllerInput;

	GW::AUDIO::GAudio gAudio;
	GW::AUDIO::GMusic gMusic;
	bool isMusicPlaying = false;

	Microsoft::WRL::ComPtr<ID3D11Buffer>		vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		instanceBuffer;
	ID3D11Buffer*								CB_PerSceneBuffer;
	ID3D11Buffer*								CB_PerObjectBuffer;
	ID3D11Buffer*								CB_PerFrameBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;

	// Rasterizer states
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterStateNormal;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterStateWireframe;

	// Create the camera
	Camera viewCamera = Camera((float)m_windowWidth / m_windowHeight);

	// Data sent to constant buffers
	CB_PerObject CB_currentPerObject;
	CB_PerFrame CB_currentPerFrame;
	CB_PerScene CB_currentPerScene;

	// Create a GameManager for level loading/switching
	GameManager gameManager;
	Clock flashlightBlockTimer;

	bool goingUp = true;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d)
	{
		win = _win;
		d3d = _d3d;

		// Keyboard and mouse input handle
		UNIVERSAL_WINDOW_HANDLE winHandle;
		win.GetWindowHandle(winHandle);
		kbmInput.Create(winHandle);

		// Gamepad controller input handle
		controllerInput.Create();

		// Load the first level
		gameManager.gameLevelPath = "../Levels/GameLevel.txt";
		gameManager.LoadLevel();

		// Setup audio
		GReturn ret = gAudio.Create();
		ret = gMusic.Create(gameManager.musicPath1, gAudio, 0.1f);
		gMusic.isPlaying(isMusicPlaying);
		if (!isMusicPlaying)
			gMusic.Play(true);

		InitializeGraphics();
	}

	GameManager* GetGameManager()
	{
		return &gameManager;
	}


private:
	//Constructor helper functions 
	void InitializeGraphics()
	{
		ID3D11Device* creator = nullptr;
		d3d.GetDevice((void**)&creator);
		
		InitializeVertexBuffer(creator);
		InitializeIndexBuffer(creator);
		InitializeInstanceBuffer(creator);
		InitializeConstantBuffer(creator);
		InitializeRenderStates(creator);
		InitializePipeline(creator);
		
		// free temporary handle
		creator->Release();
	}

	void InitializeVertexBuffer(ID3D11Device* creator)
	{
		CreateVertexBuffer(creator, gameManager.currentLevelData.levelVertices.data(), sizeof(H2B::VERTEX) * gameManager.currentLevelData.levelVertices.size());
	}

	void CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE);
		creator->CreateBuffer(&bDesc, &bData, vertexBuffer.GetAddressOf());
	}

	void InitializeIndexBuffer(ID3D11Device* creator)
	{
		CreateIndexBuffer(creator, gameManager.currentLevelData.levelIndices.data(), sizeof(UINT) * gameManager.currentLevelData.levelIndices.size());
	}

	void CreateIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE);
		creator->CreateBuffer(&bDesc, &bData, indexBuffer.GetAddressOf());
	}

	void InitializeInstanceBuffer(ID3D11Device* creator)
	{
		CreateInstanceBuffer(creator, gameManager.currentLevelData.levelTransforms.data(), sizeof(XMFLOAT4X4) * gameManager.currentLevelData.levelTransforms.size());
	}

	void CreateInstanceBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		creator->CreateBuffer(&bDesc, &bData, instanceBuffer.GetAddressOf());
	}

	void InitializeConstantBuffer(ID3D11Device* creator)
	{
		PipelineHandles currHandles = GetCurrentPipelineHandles();

		// Setup original PerObject constant buffer structure
		CB_currentPerObject.vMatrix = viewCamera.GetViewMatrix();
		CB_currentPerObject.pMatrix = viewCamera.GetPerspectiveMatrix();
		XMStoreFloat4(&CB_currentPerObject.materialIndex, XMVectorReplicate(0.0f));

		// Setup original perFrame (lighting) constant buffer structure
		XMStoreFloat4(&CB_currentPerFrame.dirLight_Color, XMLoadFloat4(&m_origSunlightColor));
		XMStoreFloat3(&CB_currentPerFrame.dirLight_Direction, XMVector3Normalize(XMLoadFloat3(&m_originalSunlightDirection)));
		for (size_t i = 0; i < gameManager.currentLevelData.levelPointLights.size(); i++)
		{
			CB_currentPerFrame.pointLights[i] = gameManager.currentLevelData.levelPointLights[i];
		}
		for (size_t i = 0; i < gameManager.currentLevelData.levelSpotLights.size(); i++)
		{
			CB_currentPerFrame.spotLights[i] = gameManager.currentLevelData.levelSpotLights[i];
		}
		// Flashlight
		CB_currentPerFrame.cameraFlashlight = gameManager.cameraFlashlight;
		CB_currentPerFrame.flashlightPowerOn.x = gameManager.flashlightPowerOn; // bool

		// Setup per scene buffer
		for (size_t i = 0; i < gameManager.currentLevelData.levelAttributes.size(); i++)
		{
			CB_currentPerScene.currOBJAttributes[i] = gameManager.currentLevelData.levelAttributes[i];
		}
		CB_currentPerScene.numPointLights = gameManager.currentLevelData.levelPointLights.size();
		CB_currentPerScene.numSpotLights = gameManager.currentLevelData.levelSpotLights.size();

		// Create the constant buffers
		CreateConstantBuffer(creator, sizeof(CB_PerObject), &CB_PerObjectBuffer, D3D11_USAGE_DYNAMIC);
		CreateConstantBuffer(creator, sizeof(CB_PerFrame), &CB_PerFrameBuffer, D3D11_USAGE_DYNAMIC);
		CreateConstantBuffer(creator, sizeof(CB_PerScene), &CB_PerSceneBuffer, D3D11_USAGE_DYNAMIC);

		// UPLOAD PER-SCENE CONSTANT BUFFER
		CB_GPU_UPLOAD_PER_SCENE(currHandles);
	}

	void CreateConstantBuffer(ID3D11Device* creator, unsigned int sizeInBytes, ID3D11Buffer** buffer, D3D11_USAGE usageFlag)
	{
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_CONSTANT_BUFFER, usageFlag, D3D11_CPU_ACCESS_WRITE);
		HRESULT hr = creator->CreateBuffer(&bDesc, nullptr, buffer);
	}

	void InitializeRenderStates(ID3D11Device* creator)
	{
		CD3D11_RASTERIZER_DESC normalDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
		//ZeroMemory(&normalDesc, sizeof(D3D11_RASTERIZER_DESC));
		normalDesc.FillMode = D3D11_FILL_SOLID;
		//normalDesc.CullMode = D3D11_CULL_NONE;
		HRESULT res = creator->CreateRasterizerState(&normalDesc, rasterStateNormal.GetAddressOf());

		CD3D11_RASTERIZER_DESC wireframeDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
		//ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
		wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
		//wireframeDesc.CullMode = D3D11_CULL_NONE;
		res = creator->CreateRasterizerState(&wireframeDesc, rasterStateWireframe.GetAddressOf());
	}

	void InitializePipeline(ID3D11Device* creator)
	{
		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(creator, compilerFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(creator, compilerFlags);
		CreateVertexInstancedInputLayout(creator, vsBlob);
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string vertexShaderSource = ReadFileIntoString("../Shaders/VertexShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

		HRESULT compilationResult = 
			D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
				vsBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreateVertexShader(vsBlob->GetBufferPointer(),
				vsBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return vsBlob;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string pixelShaderSource = ReadFileIntoString("../Shaders/PixelShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

		HRESULT compilationResult =
			D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
				psBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreatePixelShader(psBlob->GetBufferPointer(),
				psBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return psBlob;
	}

	void CreateVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
	{
		D3D11_INPUT_ELEMENT_DESC format[] = {
			{
				"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
				D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
			},
			{
				"UVCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
				D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
			},
			{
				"NORMDIR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
				D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
			}
		};
		creator->CreateInputLayout(format, ARRAYSIZE(format),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			vertexFormat.GetAddressOf());
	}

	void CreateVertexInstancedInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
	{
		D3D11_INPUT_ELEMENT_DESC format[] = {
			// PER VERTEX DATA
			{
				"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
				D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
			},
			{
				"UVCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
				D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
			},
			{
				"NORMDIR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
				D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
			},

			// PER INSTANCE DATA
			{
				"WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,
				D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
			},
			{
				"WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,
				D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
			},
			{
				"WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,
				D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
			},
			{
				"WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,
				D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1
			}
		};
		creator->CreateInputLayout(format, ARRAYSIZE(format),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			vertexFormat.GetAddressOf());
	}

public:
	void Render()
	{
		PipelineHandles curHandles = GetCurrentPipelineHandles();
		SetUpPipeline(curHandles);

		// Update the viewport for the scene - Fixes weird bug 
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		curHandles.context->RSSetViewports(1, &viewport);

		// Update view
		viewCamera.UpdateViewMatrix();
		CB_currentPerObject.vMatrix = viewCamera.GetViewMatrix();
		CB_currentPerObject.pMatrix = viewCamera.GetPerspectiveMatrix();

		// Update flashlight spot light position and orientation
		gameManager.cameraFlashlight.transform = viewCamera.GetCameraWorldMatrix();
		std::cout << "Flashlight: " << CB_currentPerFrame.flashlightPowerOn.x << std::endl;

		// Update constant buffer flashlight
		CB_currentPerFrame.cameraFlashlight = gameManager.cameraFlashlight;
		CB_currentPerFrame.flashlightPowerOn.x = gameManager.flashlightPowerOn;	// On or off

		 static float temp = 0.0001f;
		
		// Upload per frame constant buffer (lighting changes)
		CB_currentPerFrame.time = temp;
		CB_GPU_UPLOAD_PER_FRAME(curHandles);

		// Draw via GPU instancing 
		for (auto instance : gameManager.currentLevelData.levelInstances)
		{
			Level_Data::LEVEL_MODEL* model
				= &gameManager.currentLevelData.levelModels[instance.modelIndex];

			// Call draw indexed instanced for each mesh of each model
			for (size_t meshIndex = 0; meshIndex < model->meshCount; meshIndex++)
			{
				H2B::MESH* mesh =
					&gameManager.currentLevelData.levelMeshes[model->meshStart + meshIndex];

				// Pick which material to use
				CB_currentPerObject.materialIndex.x = model->materialStart + mesh->materialIndex;

				// Upload per-object constant buffer
				CB_GPU_UPLOAD_PER_OBJECT(curHandles);
				curHandles.context->DrawIndexedInstanced(mesh->drawInfo.indexCount, instance.transformCount,
					model->indexStart + mesh->drawInfo.indexOffset, model->vertexStart, instance.transformStart);
			}
		}


		// DELETE. THIS IS MAKING THE SPOTLIGHTS ROTATE AT THIS MOMENT, BUT MUST DO BETTER
		if (temp < 2 && goingUp)
		{
			goingUp = true;
			temp += 0.01f;
		}
		else if( (temp >=2 && goingUp) || !goingUp)
		{
			goingUp = false;
			temp -= 0.01f;
			if (temp <= 0)
				goingUp = true;
		}

		ReleasePipelineHandles(curHandles);
	}

	void UpdateCamera(double deltaTime)
	{
		// Get Controller input
		float lStickXAxis= 0.0f;
		float lStickYAxis= 0.0f;
		float rStickXAxis= 0.0f;
		float rStickYAxis= 0.0f;
		float lShoulderBtn = 0.0f;
		float rShoulderBtn = 0.0f;
		float rTriggerAxis = 0.0f;
		float southButton = 0.0f;
		bool isConnected = false;
		controllerInput.IsConnected(0, isConnected);

		// Only get the controller states if the controller is connected
		if (isConnected)
		{
			controllerInput.GetState(0, G_LX_AXIS, lStickXAxis);
			controllerInput.GetState(0, G_LY_AXIS, lStickYAxis);
			controllerInput.GetState(0, G_RX_AXIS, rStickXAxis);
			controllerInput.GetState(0, G_RY_AXIS, rStickYAxis);
			controllerInput.GetState(0, G_LEFT_SHOULDER_BTN, lShoulderBtn);
			controllerInput.GetState(0, G_RIGHT_SHOULDER_BTN, rShoulderBtn);
			controllerInput.GetState(0, G_RIGHT_TRIGGER_AXIS, rTriggerAxis);
			controllerInput.GetState(0, G_SOUTH_BTN, southButton);
		}
		
		// Movement {if: keyboard input, else if: controller input}
		// Walk Forward
		if (GetAsyncKeyState('W'))
			viewCamera.Walk(viewCamera.GetCamMoveSpeed() * deltaTime);
		else if (lStickYAxis > 0)
			viewCamera.Walk(viewCamera.GetCamMoveSpeed() * lStickYAxis * deltaTime);

		// Walk Backward
		if (GetAsyncKeyState('S'))
			viewCamera.Walk(-1 * viewCamera.GetCamMoveSpeed() * deltaTime);
		else if (lStickYAxis < 0)
			viewCamera.Walk(viewCamera.GetCamMoveSpeed() * lStickYAxis * deltaTime);

		// Strafe left
		if (GetAsyncKeyState('A'))
			viewCamera.Strafe(-1 * viewCamera.GetCamMoveSpeed() * deltaTime);
		else if (lStickXAxis < 0)
			viewCamera.Strafe(viewCamera.GetCamMoveSpeed() * lStickXAxis * deltaTime);

		// Strafe right
		if (GetAsyncKeyState('D'))
			viewCamera.Strafe(1 * viewCamera.GetCamMoveSpeed() * deltaTime);
		else if (lStickXAxis > 0)
			viewCamera.Strafe(viewCamera.GetCamMoveSpeed() * lStickXAxis * deltaTime);

		// Slide up
		if (GetAsyncKeyState('E'))
			viewCamera.Slide(1 * viewCamera.GetCamMoveSpeed() * deltaTime);
		else if (rShoulderBtn == 1)
			viewCamera.Slide(viewCamera.GetCamMoveSpeed() * rShoulderBtn * deltaTime);

		// Slide down
		if (GetAsyncKeyState('Q'))
			viewCamera.Slide(-1 * viewCamera.GetCamMoveSpeed() * deltaTime);
		else if (lShoulderBtn == 1)
			viewCamera.Slide(-viewCamera.GetCamMoveSpeed() * lShoulderBtn * deltaTime);

		// Movement Speed boost
		if (GetAsyncKeyState(VK_LSHIFT))
			viewCamera.SetCamMoveSpeed(m_camMoveSpeedOG * 3);
		else
			viewCamera.SetCamMoveSpeed(m_camMoveSpeedOG);
		if(rTriggerAxis > 0)
			viewCamera.SetCamMoveSpeed(m_camMoveSpeedOG * 3);

		if (!gameManager.canUseFlash && flashlightBlockTimer.GetMSElapsed() > 250)
		{
			gameManager.canUseFlash = true;
		}
		
		// Camera flashlight toggle - flashlight is off on startup
		// Turn On
		if (gameManager.canUseFlash && !gameManager.flashlightPowerOn && 
		   (GetAsyncKeyState('F') || southButton == 1))
		{
			gameManager.flashlightPowerOn = true;
			gameManager.canUseFlash = false;
			flashlightBlockTimer.Restart();
		}
		// Turn off
		else if (gameManager.canUseFlash && gameManager.flashlightPowerOn &&
				(GetAsyncKeyState('F') || southButton == 1))
		{
			gameManager.flashlightPowerOn = false;
			gameManager.canUseFlash = false;
			flashlightBlockTimer.Restart();
		}


		// Rotation
		// MOUSE
		float dx = 0.0f;
		float dy = 0.0f;
		GReturn mouseReturn = kbmInput.GetMouseDelta(dx, dy);
		// If there was mouse movement
		if (mouseReturn != GReturn::REDUNDANT)
		{
			viewCamera.PitchX(XMConvertToRadians( 0.1f * dy * viewCamera.GetMouseYSensitivity()));
			viewCamera.YawY(XMConvertToRadians(0.1f * dx * viewCamera.GetMouseXSensitivity()));
		}
		// CONTROLLER
		else
		{
			// Look left/right
			if(rStickXAxis != 0)
				viewCamera.YawY(XMConvertToRadians(0.1f * rStickXAxis * viewCamera.GetRightStickXSensitivity()));
			// Look up/down
			if (rStickYAxis != 0)
				viewCamera.PitchX(XMConvertToRadians(-0.1f * rStickYAxis * viewCamera.GetRightStickYSensitivity()));
		}
	}

private:
	struct PipelineHandles
	{
		ID3D11DeviceContext* context;
		ID3D11RenderTargetView* targetView;
		ID3D11DepthStencilView* depthStencil;
	};
	//Render helper functions
	PipelineHandles GetCurrentPipelineHandles()
	{
		PipelineHandles retval;
		d3d.GetImmediateContext((void**)&retval.context);
		d3d.GetRenderTargetView((void**)&retval.targetView);
		d3d.GetDepthStencilView((void**)&retval.depthStencil);
		return retval;
	}

	void SetUpPipeline(PipelineHandles handles)
	{
		SetRenderTargets(handles);
		SetVertexBuffers(handles);
		SetIndexBuffer(handles);
		SetConstantBuffers(handles);
		SetShaders(handles);
		SetRasterizerState(handles, rasterStateNormal.Get());

		handles.context->IASetInputLayout(vertexFormat.Get());
		handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void SetRenderTargets(PipelineHandles handles)
	{
		ID3D11RenderTargetView* const views[] = { handles.targetView };
		handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);
	}

	void SetVertexBuffers(PipelineHandles handles)
	{
		const UINT strides[] = {sizeof(H2B::VERTEX), sizeof(PerInstanceData)};
		const UINT offsets[] = { 0, 0 };
		ID3D11Buffer* const buffs[] = { vertexBuffer.Get(), instanceBuffer.Get()};
		handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	}

	void SetIndexBuffer(PipelineHandles handles)
	{
		const UINT offsets[] = { 0 };
		handles.context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	}

	void SetConstantBuffers(PipelineHandles handles)
	{
		ID3D11Buffer* const constantBuffers[] = { CB_PerObjectBuffer, CB_PerFrameBuffer, CB_PerSceneBuffer };
		handles.context->VSSetConstantBuffers(0, 1, &constantBuffers[0]);
		
		handles.context->PSSetConstantBuffers(0, 1, &constantBuffers[0]);
		handles.context->PSSetConstantBuffers(1, 1, &constantBuffers[1]);
		handles.context->PSSetConstantBuffers(2, 1, &constantBuffers[2]);
	}

	void SetShaders(PipelineHandles handles)
	{
		handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
		handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
	}

	void SetRasterizerState(PipelineHandles handles, ID3D11RasterizerState* newState)
	{
		handles.context->RSSetState(newState);
	}

	void ReleasePipelineHandles(PipelineHandles toRelease)
	{
		toRelease.depthStencil->Release();
		toRelease.targetView->Release();
		toRelease.context->Release();
	}

	/////////////////////////////////////////////////////////////////////////////

	// UPDATE PER-SCENE CONSTANT BUFFER
	void CB_GPU_UPLOAD_PER_SCENE(Renderer::PipelineHandles& curHandles)
	{
		// Upload matrices to the GPU
		D3D11_MAPPED_SUBRESOURCE gpuBuffer;

		// Disable GPU access to the constant buffer 
		HRESULT hr = curHandles.context->Map(CB_PerSceneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &gpuBuffer);
		memcpy(gpuBuffer.pData, &CB_currentPerScene, sizeof(CB_PerScene));
		curHandles.context->Unmap(CB_PerSceneBuffer, 0);
	}

	// UPDATE PER-OBJECT AND PER-FRAME CONSTANT BUFFER
	void CB_GPU_UPLOAD_PER_OBJECT(Renderer::PipelineHandles& curHandles)
	{
		// Upload matrices to the GPU
		D3D11_MAPPED_SUBRESOURCE gpuBuffer;

		// Disable GPU access to the constant buffer 
		HRESULT hr = curHandles.context->Map(CB_PerObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &gpuBuffer);
		memcpy(gpuBuffer.pData, &CB_currentPerObject, sizeof(CB_PerObject));
		curHandles.context->Unmap(CB_PerObjectBuffer, 0);
	}

	// UPDATE PER-OBJECT AND PER-FRAME CONSTANT BUFFER
	void CB_GPU_UPLOAD_PER_FRAME(Renderer::PipelineHandles& curHandles)
	{
		// Upload matrices to the GPU
		D3D11_MAPPED_SUBRESOURCE gpuBuffer;

		// Disable GPU access to the constant buffer 
		HRESULT hr = curHandles.context->Map(CB_PerFrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &gpuBuffer);
		memcpy(gpuBuffer.pData, &CB_currentPerFrame, sizeof(CB_PerFrame));
		curHandles.context->Unmap(CB_PerFrameBuffer, 0);
	}

	void DrawGrid(Renderer::PipelineHandles& curHandles)
	{
		// Draw Grid Rows
		curHandles.context->Draw((m_gridDensity + 1) * 2, 0);
		// Draw Grid Cols
		curHandles.context->Draw((m_gridDensity + 1) * 2, ((m_gridDensity + 1) * 2));
	}

public:
	~Renderer()
	{
		// ComPtr will auto release so nothing to do here yet
	}
};
