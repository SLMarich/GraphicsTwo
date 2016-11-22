#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

#include "DDSTextureLoader.h"
#include <thread>

using namespace GraphicsTwoProject;

using namespace DirectX;
using namespace Windows::Foundation;

extern char buttons[256];
extern bool mouse_move;
extern float diffx;
extern float diffy;
extern bool left_click;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
#pragma region VIEWPROJECTION
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);

	XMStoreFloat4x4(
		&projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
	);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 0.0f, -3.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 1.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
	XMStoreFloat4x4(&camera, XMMatrixLookAtRH(eye, at, up));
#pragma endregion
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(radians);
	}

#pragma region CAMERAUPDATE
	XMMATRIX newCam = XMLoadFloat4x4(&camera);
	
	if (buttons['W']) {
		//Moves the camera in the direction it is facing
		newCam.r[3] = newCam.r[3] + newCam.r[2] * -(float)timer.GetElapsedSeconds()*5.0f;
	}
	if (buttons['A']) {
		//Strafes the camera to the left
		newCam.r[3] = newCam.r[3] + newCam.r[0] * -(float)timer.GetElapsedSeconds()*5.0f;
	}
	if (buttons['S']) {
		//Moves the camera opposite the direction it is facing
		newCam.r[3] = newCam.r[3] + newCam.r[2] * (float)timer.GetElapsedSeconds()*5.0f;
	}
	if (buttons['D']) {
		//Strafes the camera to the right
		newCam.r[3] = newCam.r[3] + newCam.r[0] * (float)timer.GetElapsedSeconds()*5.0f;
	}
	if (buttons['C']) {
		newCam = XMMatrixMultiply(newCam, XMMatrixTranslation(0.0f, -5.0f*(float)timer.GetElapsedSeconds(), 0.0f));
	}
	if (buttons[' ']) {
		newCam = XMMatrixMultiply(newCam, XMMatrixTranslation(0.0f, 5.0f*(float)timer.GetElapsedSeconds(), 0.0f));
	}
	
	
	if (mouse_move)
	{
		// Updates the application state once per frame.
		if (left_click)
		{
			XMVECTOR pos = newCam.r[3];
			newCam.r[3] = XMLoadFloat4(&XMFLOAT4(0, 0, 0, 1));
			newCam = XMMatrixRotationX(-diffy*0.01f) * newCam * XMMatrixRotationY(-diffx*0.01f);
			newCam.r[3] = pos;
		}
	}
	
	XMStoreFloat4x4(&camera, newCam);
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, newCam)));
	//XMStoreFloat4x4(&m_constantBufferData.view, newCam);
	//Move skybox to camera
	XMMATRIX camerapos = XMMatrixTranslation(camera._41, camera._42, camera._43);
	DirectX::XMStoreFloat4x4(&skyboxModel, XMMatrixTranspose(camerapos));

	mouse_move = false;
#pragma endregion

}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&cubeModel, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void Sample3DSceneRenderer::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking()
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
#pragma region STATESETUP
	//Set rasterizer state
	m_deviceResources->GetD3DDeviceContext()->RSSetState(m_rasterizerState.Get());
#pragma endregion

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

#pragma region SKYBOXDRAW
	// Prepare the constant buffer to send it to the graphics device.
	//XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMLoadFloat4x4(&skyboxModel)));
	m_constantBufferData.model = skyboxModel;
	
	context->UpdateSubresource1(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
		);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		skyboxVertexBuffer.GetAddressOf(),
		&stride,
		&offset
		);

	context->IASetIndexBuffer(
		skyboxIndexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
		);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(skyboxInputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		skyboxVertexShader.Get(),
		nullptr,
		0
		);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
		);

	//Set pixel shader sampler states
	context->PSSetSamplers(0, 1, linearSamplerState.GetAddressOf());
	//Assign pixel shader resources
	context->PSSetShaderResources(0, 1, skyboxShaderResourceView.GetAddressOf());
	// Attach our pixel shader.
	context->PSSetShader(
		skyboxPixelShader.Get(),
		nullptr,
		0
		);

	// Draw the objects.
	context->DrawIndexed(
		skyboxIndexCount,
		0,
		0
		);

	//Clear depth buffer after skybox is drawn
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, 1.0f, 0);
#pragma endregion

#pragma region CUBEDRAW
	// Prepare the constant buffer to send it to the graphics device.
	XMStoreFloat4x4(&m_constantBufferData.model,XMMatrixTranspose(XMLoadFloat4x4(&cubeModel)));
	context->UpdateSubresource1(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
	);

	// Each vertex is one instance of the VertexPositionColor struct.
	stride = sizeof(VertexPositionColor);
	offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
	);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
	);

	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
	);
#pragma endregion
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
#pragma region RASTERIZER
	//Set up Rasterizer
	D3D11_RASTERIZER_DESC rasterizerDesc;
	rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = TRUE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);
#pragma endregion

#pragma region THREADLOADING
	//Load textures in threads
	std::thread skyboxTextureThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\SkyboxOcean.dds",
		skyboxTexture.GetAddressOf(),
		skyboxShaderResourceView.GetAddressOf(), 0);

#pragma endregion

#pragma region SKYBOXLOADING
	//Skybox
	auto loadSkyboxVSTask = DX::ReadDataAsync(L"SkyboxVertexShader.cso");
	auto loadSkyboxPSTask = DX::ReadDataAsync(L"SkyboxPixelShader.cso");

	D3D11_SAMPLER_DESC linearSamplerDesc;
	linearSamplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	linearSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	linearSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	linearSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	linearSamplerDesc.MinLOD = -FLT_MAX;
	linearSamplerDesc.MaxLOD = FLT_MAX;
	linearSamplerDesc.MipLODBias = 0.0f;
	linearSamplerDesc.MaxAnisotropy = 1;
	linearSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	linearSamplerDesc.BorderColor[0] = 1.0f;
	linearSamplerDesc.BorderColor[1] = 1.0f;
	linearSamplerDesc.BorderColor[2] = 1.0f;
	linearSamplerDesc.BorderColor[3] = 1.0f;

	m_deviceResources->GetD3DDevice()->CreateSamplerState(&linearSamplerDesc, linearSamplerState.GetAddressOf());

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createSkyboxVSTask = loadSkyboxVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&skyboxVertexShader
				)
			);

		static const D3D11_INPUT_ELEMENT_DESC skyboxVertexDesc [] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				skyboxVertexDesc,
				ARRAYSIZE(skyboxVertexDesc),
				&fileData[0],
				fileData.size(),
				&skyboxInputLayout
				)
			);
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createSkyboxPSTask = loadSkyboxPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&skyboxPixelShader
				)
			);

		//CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer) , D3D11_BIND_CONSTANT_BUFFER);
		//DX::ThrowIfFailed(
		//	m_deviceResources->GetD3DDevice()->CreateBuffer(
		//		&constantBufferDesc,
		//		nullptr,
		//		&m_constantBuffer
		//		)
		//	);
	});

	// Once both shaders are loaded, create the mesh.
	auto createSkyboxTask = (createSkyboxPSTask && createSkyboxVSTask).then([this] () {

		// Load mesh vertices. Each vertex has a position and a color.
		static const VertexPositionColor skyboxVertices[] = 
		{
			{XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f)},
			{XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
			{XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f)},
			{XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f)},
			{XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
			{XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f)},
			{XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f)},
			{XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
		};

		D3D11_SUBRESOURCE_DATA skyboxVertexBufferData = {0};
		skyboxVertexBufferData.pSysMem = skyboxVertices;
		skyboxVertexBufferData.SysMemPitch = 0;
		skyboxVertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC skyboxVertexBufferDesc(sizeof(skyboxVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&skyboxVertexBufferDesc,
				&skyboxVertexBufferData,
				&skyboxVertexBuffer
				)
			);

		// Load mesh indices.
		static const unsigned short skyboxIndices [] =
		{
			0,2,1, // -x
			1,2,3,

			4,5,6, // +x
			5,7,6,

			0,1,5, // -y
			0,5,4,

			2,6,7, // +y
			2,7,3,

			0,4,6, // -z
			0,6,2,

			1,3,7, // +z
			1,7,5,
		};

		skyboxIndexCount = ARRAYSIZE(skyboxIndices);

		D3D11_SUBRESOURCE_DATA skyboxIndexBufferData = {0};
		skyboxIndexBufferData.pSysMem = skyboxIndices;
		skyboxIndexBufferData.SysMemPitch = 0;
		skyboxIndexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC skyboxIndexBufferDesc(sizeof(skyboxIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&skyboxIndexBufferDesc,
				&skyboxIndexBufferData,
				&skyboxIndexBuffer
				)
			);
	});
#pragma endregion

#pragma region SAMPLECUBE
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
			)
		);
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShader
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
	});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask).then([this]() {

		// Load mesh vertices. Each vertex has a position and a color.
		static const VertexPositionColor cubeVertices[] =
		{
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
			{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
			{ XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = cubeVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
			)
		);

		// Load mesh indices. Each trio of indices represents
		// a triangle to be rendered on the screen.
		// For example: 0,2,1 means that the vertices with indexes
		// 0, 2 and 1 from the vertex buffer compose the 
		// first triangle of this mesh.
		static const unsigned short cubeIndices[] =
		{
			0,1,2, // -x
			1,3,2,

			4,6,5, // +x
			5,6,7,

			0,5,1, // -y
			0,4,5,

			2,7,6, // +y
			2,3,7,

			0,6,4, // -z
			0,2,6,

			1,7,3, // +z
			1,5,7,
		};

		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = cubeIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexBuffer
			)
		);
	});

	XMStoreFloat4x4(&cubeModel, XMMatrixIdentity());
	//XMStoreFloat4x4(&cubeModel, XMMatrixTranslation(0.0f,-1.0f, 0.0f));
#pragma endregion

#pragma region THREADJOINING
	skyboxTextureThread.join();
#pragma endregion

	// Once the cube is loaded, the object is ready to be rendered.
	createCubeTask.then([this] () {
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();
	m_constantBuffer.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();

	m_rasterizerState.Reset();

	skyboxTexture.Reset();
	skyboxShaderResourceView.Reset();
	skyboxInputLayout.Reset();
	skyboxVertexShader.Reset();
	skyboxPixelShader.Reset();
	skyboxVertexBuffer.Reset();
	skyboxIndexBuffer.Reset();

	linearSamplerState.Reset();
}