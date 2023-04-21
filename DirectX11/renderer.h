#include "load_data_oriented.h"
#include <d3dcompiler.h>
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

	Microsoft::WRL::ComPtr<ID3D11Buffer>		vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		indexBuffer;
	//std::vector<PerInstanceData>				perInstanceData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		instanceBuffer;
	ID3D11Buffer*								CB_PerObjectBuffer;
	ID3D11Buffer*								CB_PerFrameBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;

	Camera viewCamera = Camera((float)m_windowWidth / m_windowHeight);
	CB_PerObject CB_currentPerObject;
	CB_PerFrame CB_currentPerFrame;

	// Create a GameManager for level loading/switching
	GameManager gameManager;

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
		gameManager.LoadLevel(0);

		InitializeGraphics();
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
		//for (size_t i = 0; i < gameManager.currentLevelData.levelTransforms.size(); i++)
		//{
		//	perInstanceData.push_back()
		//}

		//// Create the per-instance data
		//for (auto matrixTransform : gameManager.currentLevelData.levelTransforms)
		//{
		//	perInstanceData.push_back({ matrixTransform });
		//}

		//for (auto instance : gameManager.currentLevelData.levelInstances)
		//{
		//	for (size_t i = 0; i < model.transformCount; i++)
		//	{
		//		unsigned int matindex = gameManager.currentLevelData.levelModels
		//	}
		//}
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
		// TODO: SETUP ORIGINAL OBJ ATTRIBUTES

		// Setup original PerObject constant buffer structure
		CB_currentPerObject.vMatrix = viewCamera.GetViewMatrix();
		CB_currentPerObject.pMatrix = viewCamera.GetPerspectiveMatrix();

		// Setup original perFrame (lighting) constant buffer structure
		XMStoreFloat4(&CB_currentPerFrame.lightColor, XMLoadFloat4(&m_origSunlightColor));
		XMStoreFloat3(&CB_currentPerFrame.lightDirection, XMVector3Normalize(XMLoadFloat3(&m_originalSunlightDirection)));

		// Create the constant buffers
		CreateConstantBuffer(creator, &CB_currentPerObject, sizeof(CB_PerObject), &CB_PerObjectBuffer);
		CreateConstantBuffer(creator, &CB_currentPerFrame, sizeof(CB_PerFrame), &CB_PerFrameBuffer);
	}

	void CreateConstantBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes, ID3D11Buffer** buffer)
	{
		//D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		HRESULT hr = creator->CreateBuffer(&bDesc, nullptr, buffer);
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



		static float temp = 0.0001f;

		// Assignment2
		//for (size_t i = 0; i < FSLogo_meshcount; i++)
		//{
		//	// Drawing text - stationary
		//	if (i == 0)
		//		XMStoreFloat4x4(&CB_currentPerObject.wMatrix, XMMatrixIdentity());
		//	// Drawing logo, rotating
		//	else
		//		XMStoreFloat4x4(&CB_currentPerObject.wMatrix, XMMatrixRotationY( temp ));

		//	// Swap out constant buffer obj attributes for respective oject
		//	CB_currentPerObject.currOBJAttributes = FSLogo_materials[FSLogo_meshes[i].materialIndex].attrib;
		//	CB_GPU_Upload(curHandles);
		//	curHandles.context->DrawIndexed(FSLogo_meshes[i].indexCount,
		//		FSLogo_meshes[i].indexOffset, 0);
		//}

		//////////////// PRE-INSTANCING
		//for (size_t i = 0; i < gameManager.currentLevelData.levelModels.size(); i++)
		//{
		//	//TODO : FIX
		//	// Swap out matrices 
		//	//CB_currentPerObject.wMatrix = gameManager.currentLevelData.levelTransforms[gameManager.currentLevelData.level];
		//	XMStoreFloat4x4(&CB_currentPerObject.wMatrix, XMMatrixTranslation(i * 2, 0, 0));

		//	// Load material attributes
		//	CB_currentPerObject.currOBJAttributes = gameManager.currentLevelData.levelMaterials[i].attrib;
		//	CB_currentPerObject.currOBJAttributes = gameManager.currentLevelData.levelMaterials[gameManager.currentLevelData.levelMeshes[i].materialIndex].attrib ;

		//	CB_GPU_Upload(curHandles);

		//	for (size_t j = 0; j < gameManager.currentLevelData.levelModels[i].meshCount; j++)
		//	{
		//		unsigned int actualMeshIndex = j + gameManager.currentLevelData.levelModels[i].meshStart;
		//		const H2B::MESH* mesh = &gameManager.currentLevelData.levelMeshes[actualMeshIndex];

		//		curHandles.context->DrawIndexed(mesh->drawInfo.indexCount,
		//		mesh->drawInfo.indexOffset + gameManager.currentLevelData.levelModels[i].indexStart,
		//			gameManager.currentLevelData.levelModels[i].vertexStart);
		//	}
		//}




		//for (auto i : gameManager.currentLevelData.levelModels)
		//{
		//	// Set per instance data
		//	//XMStoreFloat4x4(&CB_currentPerObject.instanceData.wMatrix, )
		//	//curHandles.context->DrawIndexedInstanced(i.indexCount, )
		//}

		//CB_currentPerObject.instanceData.wMatrix

		CB_GPU_Upload(curHandles);

		for (auto instance : gameManager.currentLevelData.levelInstances)
		{
			Level_Data::LEVEL_MODEL* model = &gameManager.currentLevelData.levelModels[instance.modelIndex];
			H2B::MESH* mesh = &gameManager.currentLevelData.levelMeshes[instance.modelIndex];

			curHandles.context->DrawIndexedInstanced(model->indexCount, instance.transformCount,
				model->indexStart, model->vertexStart, instance.transformStart);

			/*curHandles.context->DrawIndexedInstanced(model->indexCount, instance.transformCount,
				model->indexStart, model->vertexStart,model->model->meshCount);*/
		}

		//for (auto instance : gameManager.currentLevelData.levelInstances)
		//{
		//	Level_Data::LEVEL_MODEL* model = &gameManager.currentLevelData.levelModels[instance.modelIndex];
		//	
		//	for (size_t meshIndex = model->meshStart; meshIndex < model->meshCount; meshIndex++)
		//	{


		//		curHandles.context->DrawIndexedInstanced(model->indexCount, instance.transformCount,
		//			model->indexStart, model->vertexStart, gameManager.currentLevelData.levelMeshes[meshIndex].drawInfo.indexOffset);
		//	}
		//}

		temp += 0.005f;

		ReleasePipelineHandles(curHandles);
	}

	void UpdateCamera(Clock& timer)
	{
		timer.now = timer.clock.now();
		timer.duration = std::chrono::duration_cast<std::chrono::milliseconds>(timer.now - timer.start);

		// Get Controller input
		float lStickXAxis= 0.0f;
		float lStickYAxis= 0.0f;
		float rStickXAxis= 0.0f;
		float rStickYAxis= 0.0f;
		float lShoulderBtn = 0.0f;
		float rShoulderBtn = 0.0f;
		bool isConnected = false;
		controllerInput.IsConnected(0, isConnected);

		// Only get the controlle states if the controller is connected
		if (isConnected)
		{
			controllerInput.GetState(0, G_LX_AXIS, lStickXAxis);
			controllerInput.GetState(0, G_LY_AXIS, lStickYAxis);
			controllerInput.GetState(0, G_RX_AXIS, rStickXAxis);
			controllerInput.GetState(0, G_RY_AXIS, rStickYAxis);
			controllerInput.GetState(0, G_LEFT_SHOULDER_BTN, lShoulderBtn);
			controllerInput.GetState(0, G_RIGHT_SHOULDER_BTN, rShoulderBtn);
		}

		// Movement
		// Walk Forward
		if (GetAsyncKeyState('W'))
			viewCamera.Walk(viewCamera.GetCamMoveSpeed());
		else if (lStickYAxis > 0)
			viewCamera.Walk(viewCamera.GetCamMoveSpeed() * lStickYAxis);

		// Walk Backward
		if (GetAsyncKeyState('S'))
			viewCamera.Walk(-1 * viewCamera.GetCamMoveSpeed());
		else if (lStickYAxis < 0)
			viewCamera.Walk(viewCamera.GetCamMoveSpeed() * lStickYAxis);

		// Strafe left
		if (GetAsyncKeyState('A'))
			viewCamera.Strafe(-1 * viewCamera.GetCamMoveSpeed());
		else if (lStickXAxis < 0)
			viewCamera.Strafe(viewCamera.GetCamMoveSpeed() * lStickXAxis);

		// Strafe right
		if (GetAsyncKeyState('D'))
			viewCamera.Strafe(1 * viewCamera.GetCamMoveSpeed());
		else if (lStickXAxis > 0)
			viewCamera.Strafe(viewCamera.GetCamMoveSpeed() * lStickXAxis);

		// Slide up
		if (GetAsyncKeyState('E'))
			viewCamera.Slide(1 * viewCamera.GetCamMoveSpeed());
		else if (rShoulderBtn == 1)
			viewCamera.Slide(viewCamera.GetCamMoveSpeed() * rShoulderBtn);

		// Slide down
		if (GetAsyncKeyState('Q'))
			viewCamera.Slide(-1 * viewCamera.GetCamMoveSpeed());
		else if (lShoulderBtn == 1)
			viewCamera.Slide(-viewCamera.GetCamMoveSpeed() * lShoulderBtn);

		// Rotation
		float dx = 0.0f;
		float dy = 0.0f;
		GReturn mouseReturn = kbmInput.GetMouseDelta(dx, dy);
		// If there was movement
		if (mouseReturn != GReturn::REDUNDANT)
		{
			viewCamera.PitchX(XMConvertToRadians( 0.1f * dy));
			viewCamera.YawY(XMConvertToRadians(0.1f * dx));
		}
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
		ID3D11Buffer* const constantBuffers[] = { CB_PerObjectBuffer, CB_PerFrameBuffer };
		handles.context->VSSetConstantBuffers(0, 1, &constantBuffers[0]);
		handles.context->PSSetConstantBuffers(0, 1, &constantBuffers[0]);
		handles.context->PSSetConstantBuffers(1, 1, &constantBuffers[1]);
	}

	void SetShaders(PipelineHandles handles)
	{
		handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
		handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
	}

	void ReleasePipelineHandles(PipelineHandles toRelease)
	{
		toRelease.depthStencil->Release();
		toRelease.targetView->Release();
		toRelease.context->Release();
	}

	/////////////////////////////////////////////////////////////////////////////

	void CB_GPU_Upload(Renderer::PipelineHandles& curHandles)
	{
		// Upload matrices to the GPU
		D3D11_MAPPED_SUBRESOURCE gpuBuffer;
		D3D11_MAPPED_SUBRESOURCE gpuBuffer2;

		// Disable GPU access to the constant buffer 
		HRESULT hr = curHandles.context->Map(CB_PerObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &gpuBuffer);
		memcpy(gpuBuffer.pData, &CB_currentPerObject, sizeof(CB_PerObject));
		curHandles.context->Unmap(CB_PerObjectBuffer, 0);

		// Disable GPU access to the constant buffer 
		hr = curHandles.context->Map(CB_PerFrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &gpuBuffer2);
		memcpy(gpuBuffer2.pData, &CB_currentPerFrame, sizeof(CB_PerFrame));
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
