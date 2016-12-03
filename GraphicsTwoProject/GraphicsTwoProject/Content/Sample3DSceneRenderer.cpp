﻿#include "pch.h"
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
	static const XMVECTORF32 eye = { 0.0f, 1.5f, -10.0f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, 1.0f, 0.0f, 0.0f };
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
	if (buttons['1'] && !groundMove && !toggledChecker) {
		groundMove = true;
		toggledChecker = true;
	}
	else if (buttons['1'] && groundMove && !toggledChecker) {
		groundMove = false;
		toggledChecker = true;
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

#pragma region CUBELIGHTUPDATE
	XMStoreFloat4x4(&cubelightModel, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&cubelightModel, XMMatrixMultiply(XMMatrixTranspose(XMMatrixRotationX((XM_2PI * 1.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))),
		XMMatrixMultiply(XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))),
			XMMatrixTranspose(XMMatrixRotationZ((XM_2PI * 1.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))))));
	//XMStoreFloat4x4(&cubelightModel, XMMatrixMultiply(XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))),
	//	XMMatrixTranspose(XMMatrixRotationZ((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	//XMStoreFloat4x4(&cubelightModel, XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))));
	//cubelightModel._24 = cubelightModel._24 - 2.0f;

	//XMStoreFloat4x4(&geoCubeLight.modelMatrix, XMLoadFloat4x4(&cubelightModel));
#pragma endregion

#pragma region GREENMARBLEUPDATE
	XMStoreFloat4x4(&greenMarble_loader.modelMatrix, XMMatrixIdentity());
#pragma endregion

#pragma region LIGHTINGUPDATES
	sampleLight.cameraPosition.x = camera._41;
	sampleLight.cameraPosition.y = camera._42;
	sampleLight.cameraPosition.z = camera._43;
	sampleLight.cameraPosition.w = camera._44;
	sampleLight.ambientTerm = DirectX::XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f);
	sampleLight.lightRange = 25.0f;

	sampleLight.pointLightColor = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	sampleLight.pointLightPosition = DirectX::XMFLOAT4(cubelightModel._14, cubelightModel._24, cubelightModel._34, cubelightModel._44);

	sampleLight.spotlightColor = DirectX::XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);
	sampleLight.spotlightConeRatio = 5.0f;
	//Move spotlight around
	if (!instanceTracerSlower)
		instanceTracer++;
	instanceTracerSlower = !instanceTracerSlower;
	if (instanceTracer >= activeInstances) {
		instanceTracer = 0;
	}

	XMFLOAT4X4 spotlightPosition;
	XMFLOAT4X4 spotlightDirection;
	//XMStoreFloat4x4(&spotlightPosition, XMMatrixMultiply(XMMatrixTranslation(15.0f, 0.0f, 15.0f), XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	//XMStoreFloat4x4(&spotlightDirection, XMMatrixMultiply(XMMatrixTranslation(0.5f, -1.0f, 0.5f), XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	XMStoreFloat4x4(&spotlightPosition, XMMatrixMultiply(XMMatrixTranslation(15.0f, 0.0f, 15.0f), XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 1.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	XMStoreFloat4x4(&spotlightDirection, XMMatrixMultiply(XMMatrixTranslation(0.5f, -1.0f, 0.5f), XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 1.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	//sampleLight.spotlightPosition = DirectX::XMFLOAT3(instanceList[instanceTracer].position.x, 0.0f, instanceList[instanceTracer].position.z);
	sampleLight.spotlightPosition = DirectX::XMFLOAT3(spotlightPosition._41, spotlightPosition._42, spotlightPosition._43);
	sampleLight.spotlightDirection = DirectX::XMFLOAT3(-spotlightDirection._41, -spotlightDirection._42, -spotlightDirection._43);

	//XMStoreFloat4x4(&sampleLight.pointRotationMatrix, XMMatrixMultiply(XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))),
	//	XMMatrixTranspose(XMMatrixRotationZ((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	XMStoreFloat4x4(&sampleLight.pointRotationMatrix, XMLoadFloat4x4(&cubelightModel));

	XMStoreFloat4x4(&sampleLight.pointRotationMatrix, XMMatrixInverse(&XMMatrixDeterminant(XMLoadFloat4x4(&sampleLight.pointRotationMatrix)), XMLoadFloat4x4(&sampleLight.pointRotationMatrix)));
#pragma endregion

#pragma region INSTANCEUPDATES
	activeInstances = 900;
	for (unsigned int i = 0; i < 30; i++) {
		for (unsigned int j = 0; j < 30; j++) {
			instanceList[i * 30 + j].position = DirectX::XMFLOAT3((float)(i + i) - 30.0f, -2.0f, (float)(j + j) - 30.0f);
			XMMATRIX groundRotation = XMMatrixMultiply(XMMatrixTranslation(instanceList[i * 30 + j].position.x, instanceList[i * 30 + j].position.y, instanceList[i * 30 + j].position.z), XMLoadFloat4x4(&cubelightModel));
			XMStoreFloat4x4(&instanceList[i * 30 + j].rotation, XMMatrixIdentity());
			if (groundMove){
				XMStoreFloat4x4(&instanceList[i * 30 + j].rotation, XMMatrixTranspose(groundRotation));
				instanceList[i * 30 + j].position = XMFLOAT3(groundRotation.r[3].m128_f32[0], groundRotation.r[3].m128_f32[1], groundRotation.r[3].m128_f32[2]);
			}
		}
	}
	instanceList[15*30+15] = instanceList[15*30+15+1];

	activePillarType1Instances = 450;
	for (unsigned int i = 0; i < 15; i++) {
		for (unsigned int j = 0; j < 15; j++) {
			pillarType1InstanceList[i*15+j] = instanceList[i*60+j*2];
			pillarType1InstanceList[i * 15 + j].position = DirectX::XMFLOAT3(pillarType1InstanceList[i * 15 + j].position.x+0.5f, pillarType1InstanceList[i * 15 + j].position.y, pillarType1InstanceList[i * 15 + j].position.z + 0.5f);
		}
	}
	//pillarType1InstanceList[0].position = DirectX::XMFLOAT3(-5.0f, 0.0f, -5.0f);
	//pillarType1InstanceList[1].position = DirectX::XMFLOAT3(5.0f, 0.0f, -5.0f);
	//pillarType1InstanceList[2].position = DirectX::XMFLOAT3(-5.0f, 0.0f, 5.0f);
	//pillarType1InstanceList[3].position = DirectX::XMFLOAT3(5.0f, 0.0f, 5.0f);

	activeGeoCubeInstances = 1;
	geoCubeLightInstanceList[0].position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	//XMStoreFloat4x4(&geoCubeLightInstanceList[0].rotation, XMMatrixIdentity());
	//geoCubeLightInstanceList[1].position = XMFLOAT3(0.0f, 1.0f, 0.0f);
	//XMStoreFloat4x4(&geoCubeLightInstanceList[1].rotation, XMMatrixIdentity());
	//geoCubeLightInstanceList[2].position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	//XMStoreFloat4x4(&geoCubeLightInstanceList[0].rotation, XMMatrixTranspose(XMLoadFloat4x4(&cubelightModel)));
	XMStoreFloat4x4(&geoCubeLightInstanceList[0].rotation, /*XMMatrixTranspose(*/XMLoadFloat4x4(&cubelightModel));
	//XMStoreFloat4x4(&geoCubeLightInstanceList[0].rotation, XMMatrixIdentity());

	XMStoreFloat4x4(&geoInstanceList[0].matrix,
		XMMatrixTranspose(
		XMMatrixMultiply(
			XMMatrixTranspose(XMLoadFloat4x4(&cubelightModel)),
			XMMatrixTranslation(0.0f,0.0f,0.0f))));
#pragma endregion

#pragma region GEOMETRYSHADERUPDATES
	if (buttons['2'] && !geoShaderEnabled && !toggledChecker) {
		geoShaderEnabled = true;
		toggledChecker = true;
	}
	else if (buttons['2'] && geoShaderEnabled && !toggledChecker) {
		geoShaderEnabled = false;
		toggledChecker = true;
	}

#pragma endregion

	if (buttons['4'] && !wireframeEnabled) {
		wireframeEnabled = true;
		toggledChecker = true;
	}
	else if (buttons['5'] && wireframeEnabled) {
		wireframeEnabled = false;
		toggledChecker = true;
	}

	if (!buttons['1'] && !buttons['2'] && !buttons['4'] && !buttons['5']) {
		toggledChecker = false;
	}
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
	if (!wireframeEnabled)
		m_deviceResources->GetD3DDeviceContext()->RSSetState(m_rasterizerState.Get());
	else
		m_deviceResources->GetD3DDeviceContext()->RSSetState(wireframeRasterState.Get());
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

#pragma region CUBELIGHTDRAW
#pragma region NOGEOSHADER
	if (!geoShaderEnabled) {
		// Prepare the constant buffer to send it to the graphics device.
		//XMStoreFloat4x4(&m_constantBufferData.model,XMMatrixTranspose(XMLoadFloat4x4(&cubeModel)));
		XMStoreFloat4x4(&m_constantBufferData.model, DirectX::XMLoadFloat4x4(&cubelightModel));
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

		//Add SRVs
		context->PSSetShaderResources(0, 1, cubelightShaderResourceView.GetAddressOf());
		// Attach our pixel shader.
		context->PSSetShader(
			skyboxPixelShader.Get(),
			nullptr,
			0
		);

		// Draw the objects.
		context->DrawIndexed(
			m_indexCount,
			0,
			0
		);
	}
#pragma endregion
#pragma region GEOSHADERACTIVE
	else {
		for (unsigned int i = 0; i < geoCubeLight.materialCount; i++) {
			//XMStoreFloat4x4(&m_constantBufferData.model, DirectX::XMLoadFloat4x4(&cubelightModel));
			XMStoreFloat4x4(&m_constantBufferData.model, DirectX::XMMatrixIdentity());
			//XMStoreFloat4x4(&m_constantBufferData.model, DirectX::XMLoadFloat4x4(&geoCubeLight.modelMatrix));
			//context->UpdateSubresource1(geoInstanceBuffer.Get(), 0, NULL, geoCubeLightInstanceList.data(), 0, 0, 0);
			context->UpdateSubresource1(cubeLightInstanceBuffer.Get(), 0, NULL, geoInstanceList.data(), 0, 0, 0);
			context->UpdateSubresource1(
				m_constantBuffer.Get(),
				0,
				NULL,
				&m_constantBufferData,
				0,
				0,
				0
			);

			unsigned int strides[2];
			unsigned int offsets[2];
			ID3D11Buffer* bufferPointers[2];
			strides[0] = sizeof(VertexUVNormTanBi);
			strides[1] = sizeof(instancePositionStructure);
			offsets[0] = 0;
			offsets[1] = 0;
			bufferPointers[0] = geoCubeLight.vertexBuffers[i].Get();
			//bufferPointers[1] = geoInstanceBuffer.Get();
			bufferPointers[1] = cubeLightInstanceBuffer.Get();
			//context->IASetVertexBuffers(0, 1, plain_loader.vertexBuffers[i].GetAddressOf(), &stride, &offset);
			context->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);
			context->IASetIndexBuffer(geoCubeLight.indexBuffers[i].Get(), DXGI_FORMAT_R16_UINT/*Each index is one 16-bit unsigned short.*/, 0);

			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

			//context->IASetInputLayout(instanceInputLayout.Get());
			context->IASetInputLayout(geoInstanceInputLayout.Get());

			// Attach our vertex shader.
			context->VSSetShader(
				geoVertexShader.Get(),
				nullptr,
				0
			);
			//Attach Hull and Domain Shaders
			context->HSSetShader(
				geoHullShader.Get(),
				nullptr,
				0
			);
			context->DSSetShader(
				geoDomainShader.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->DSSetConstantBuffers1(
				0,
				1,
				m_constantBuffer.GetAddressOf(),
				nullptr,
				nullptr
			);

			//Add SRVs
			context->PSSetShaderResources(0, 1, cubelightShaderResourceView.GetAddressOf());
			// Attach our pixel shader.
			context->PSSetShader(
				geoSkyboxPixelShader.Get(),
				nullptr,
				0
			);

			// Draw the objects.
			context->DrawIndexedInstanced(
				geoCubeLight.modelMaterialFaceVerts[i].size(),
				activeGeoCubeInstances,
				0,
				0,
				0
			);

			context->HSSetShader(nullptr, nullptr, 0);
			context->DSSetShader(nullptr, nullptr, 0);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
	}
#pragma endregion

#pragma endregion

#pragma region GREENMARBLEDRAW
	for (unsigned int i = 0; i < greenMarble_loader.materialCount; i++) {
		//m_constantBufferData.model = greenMarbleModel;
		XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMLoadFloat4x4(&greenMarble_loader.modelMatrix)));
		context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
		context->UpdateSubresource1(lightConstantBuffer.Get(), 0, NULL, &sampleLight, 0, 0, 0);
		context->UpdateSubresource1(instanceBuffer.Get(), 0, NULL, instanceList.data(), 0, 0, 0);

		unsigned int strides[2];
		unsigned int offsets[2];
		ID3D11Buffer* bufferPointers[2];
		strides[0] = sizeof(VertexUVNormTanBi);
		strides[1] = sizeof(instancePositionStructure);
		offsets[0] = 0;
		offsets[1] = 0;
		bufferPointers[0] = greenMarble_loader.vertexBuffers[i].Get();
		bufferPointers[1] = instanceBuffer.Get();
		//context->IASetVertexBuffers(0, 1, plain_loader.vertexBuffers[i].GetAddressOf(), &stride, &offset);
		context->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);
		context->IASetIndexBuffer(greenMarble_loader.indexBuffers[i].Get(), DXGI_FORMAT_R16_UINT/*Each index is one 16-bit unsigned short.*/, 0);


		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(instanceInputLayout.Get());

		// Attach our vertex shader.
		context->VSSetShader(
			instanceVertexShader.Get(),
			nullptr,
			0
		);

		// Send the constant buffer to the graphics device.
		context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);

		ID3D11ShaderResourceView* greenMarbleViews[] = { greenMarbleDiffuseSRV.Get(), greenMarbleNormalSRV.Get(), cubelightShaderResourceView.Get() };
		context->PSSetShaderResources(0, 3, greenMarbleViews);

		//Set samplers
		ID3D11SamplerState* greenMarbleSamplers[] = { anisotropicSamplerState.Get(), linearSamplerState.Get() };
		context->PSSetSamplers(0, 2, greenMarbleSamplers);

		// Attach our pixel shader.
		context->PSSetShader(
			greenMarble_pixelShader.Get(),
			nullptr,
			0
		);

		//Attach constant buffer

		context->PSSetConstantBuffers(0, 1, lightConstantBuffer.GetAddressOf());

		// Draw the objects.
		context->DrawIndexedInstanced(greenMarble_loader.modelMaterialFaceVerts[i].size(), activeInstances, 0, 0, 0);

	}
#pragma endregion

#pragma region PILLARTYPE1DRAW
	for (unsigned int i = 0; i < pillarType1_loader.materialCount; i++) {
		//m_constantBufferData.model = greenMarbleModel;
		//XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMLoadFloat4x4(&greenMarbleModel)));
		XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixIdentity());
		context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
		context->UpdateSubresource1(lightConstantBuffer.Get(), 0, NULL, &sampleLight, 0, 0, 0);
		context->UpdateSubresource1(instanceBuffer.Get(), 0, NULL, pillarType1InstanceList.data(), 0, 0, 0);

		unsigned int strides[2];
		unsigned int offsets[2];
		ID3D11Buffer* bufferPointers[2];
		strides[0] = sizeof(VertexUVNormTanBi);
		strides[1] = sizeof(instancePositionStructure);
		offsets[0] = 0;
		offsets[1] = 0;
		bufferPointers[0] = pillarType1_loader.vertexBuffers[i].Get();
		bufferPointers[1] = instanceBuffer.Get();
		//context->IASetVertexBuffers(0, 1, plain_loader.vertexBuffers[i].GetAddressOf(), &stride, &offset);
		context->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);
		context->IASetIndexBuffer(pillarType1_loader.indexBuffers[i].Get(), DXGI_FORMAT_R16_UINT/*Each index is one 16-bit unsigned short.*/, 0);


		//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(instanceInputLayout.Get());

		// Attach our vertex shader.
		context->VSSetShader(
			instanceVertexShader.Get(),
			nullptr,
			0
		);

		// Send the constant buffer to the graphics device.
		context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);

		ID3D11ShaderResourceView* pillarType1Views[] = { pillarType1DiffuseSRV.Get(), pillarType1NormalSRV.Get(), cubelightShaderResourceView.Get() };
		context->PSSetShaderResources(0, 3, pillarType1Views);

		//Set samplers
		ID3D11SamplerState* pillarType1Samplers[] = { anisotropicSamplerState.Get(), linearSamplerState.Get() };
		context->PSSetSamplers(0, 2, pillarType1Samplers);

		// Attach our pixel shader.
		context->PSSetShader(
			greenMarble_pixelShader.Get(),
			nullptr,
			0
		);

		//Attach constant buffer

		context->PSSetConstantBuffers(0, 1, lightConstantBuffer.GetAddressOf());

		// Draw the objects.
		context->DrawIndexedInstanced(pillarType1_loader.modelMaterialFaceVerts[i].size(), activePillarType1Instances, 0, 0, 0);
	}
#pragma endregion

}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
#pragma region RASTERIZER
	//Set up Rasterizer
	D3D11_RASTERIZER_DESC rasterizerDesc;
	rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
	//rasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = TRUE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, m_rasterizerState.GetAddressOf());
	rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
	m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterizerDesc, wireframeRasterState.GetAddressOf());
#pragma endregion

#pragma region THREADLOADING
	//Load textures in threads
	
	//Skybox Cubemap
	std::thread skyboxTextureThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\SkyboxOcean.dds",
		skyboxTexture.GetAddressOf(),
		skyboxShaderResourceView.GetAddressOf(), 0);

	//CubeLight Cubemap
	std::thread cubeLightDiffuseThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\CubeLight\\Spot Light Cube Map.dds",
		cubelightTexture.GetAddressOf(),
		cubelightShaderResourceView.GetAddressOf(), 0);

	//Green Marble Diffuse
	std::thread greenMarbleDiffuseThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\CubeLight\\Green Marble Tiles.dds",
		(ID3D11Resource**)greenMarbleDiffuseTexture.GetAddressOf(),
		greenMarbleDiffuseSRV.GetAddressOf(), 0);

	//Green Marble Normal Map
	std::thread greenMarbleNormalThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\CubeLight\\greenMarbleTielsNormal.dds",
		(ID3D11Resource**)greenMarbleNormalTexture.GetAddressOf(),
		greenMarbleNormalSRV.GetAddressOf(),0);

	//PillarType1
	std::thread pillarType1DiffuseThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\Pillar\\SeemlessMarble1.dds",
		(ID3D11Resource**)pillarType1DiffuseTexture.GetAddressOf(),
		pillarType1DiffuseSRV.GetAddressOf(), 0);
	std::thread pillarType1NormalThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\Pillar\\SeemlessMarble1Normal.dds",
		(ID3D11Resource**)pillarType1NormalTexture.GetAddressOf(),
		pillarType1NormalSRV.GetAddressOf(),0);

#pragma endregion

#pragma region SAMPLERSTATES
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

	D3D11_SAMPLER_DESC anisotropicSamplerDesc;
	anisotropicSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	anisotropicSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	anisotropicSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	anisotropicSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	anisotropicSamplerDesc.MipLODBias = 0.0f;
	anisotropicSamplerDesc.MaxAnisotropy = 1;
	anisotropicSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	anisotropicSamplerDesc.BorderColor[0] = 0;
	anisotropicSamplerDesc.BorderColor[1] = 0;
	anisotropicSamplerDesc.BorderColor[2] = 0;
	anisotropicSamplerDesc.BorderColor[3] = 0;
	anisotropicSamplerDesc.MinLOD = 0;
	anisotropicSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT anisotropicSamplerCreated = m_deviceResources->GetD3DDevice()->CreateSamplerState(&anisotropicSamplerDesc, &anisotropicSamplerState);


#pragma endregion

#pragma region SKYBOXLOADING
	//Skybox
	auto loadSkyboxVSTask = DX::ReadDataAsync(L"SkyboxVertexShader.cso");
	auto loadSkyboxPSTask = DX::ReadDataAsync(L"SkyboxPixelShader.cso");

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
	auto loadVSTask = DX::ReadDataAsync(L"SampleHullVertexShader.cso");
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

#pragma region INSTANCEVERTSHADER
	auto loadInstanceVSTask = DX::ReadDataAsync(L"InstanceVertexShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createPlainVSTask = loadInstanceVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&instanceVertexShader
			)
		);
		
		static const D3D11_INPUT_ELEMENT_DESC instanceVertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT ,D3D11_INPUT_PER_VERTEX_DATA,0 },
			{ "TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
			{ "BINORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
			{ "TEXCOORD",1,DXGI_FORMAT_R32G32B32_FLOAT,1,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_INSTANCE_DATA,1 },
			{ "ROTATION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_INSTANCE_DATA,1 },
			{ "ROTATION",1,DXGI_FORMAT_R32G32B32A32_FLOAT,1,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_INSTANCE_DATA,1 },
			{ "ROTATION",2,DXGI_FORMAT_R32G32B32A32_FLOAT,1,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_INSTANCE_DATA,1 },
			{ "ROTATION",3,DXGI_FORMAT_R32G32B32A32_FLOAT,1,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_INSTANCE_DATA,1 },
		};
		
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				instanceVertexDesc,
				ARRAYSIZE(instanceVertexDesc),
				&fileData[0],
				fileData.size(),
				&instanceInputLayout
			)
		);
	});
#pragma endregion

#pragma region GREENMARBLE
	bool groundMarbleLoaded = greenMarble_loader.loadMaterialOBJ(m_deviceResources, "Assets\\CubeLight\\groundQuad.obj");

	if (groundMarbleLoaded) {
		// Load shaders asynchronously.
		auto loadGreenMarblePSTask = DX::ReadDataAsync(L"GreenMarblePixelShader.cso");

		// After the pixel shader file is loaded, create the shader and constant buffer.
		auto createGrenMarblePSTask = loadGreenMarblePSTask.then([this](const std::vector<byte>& fileData) {
			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreatePixelShader(
					&fileData[0],
					fileData.size(),
					nullptr,
					&greenMarble_pixelShader
				)
			);
		});

		CD3D11_BUFFER_DESC light2ConstantBufferDesc(sizeof(Lighting), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&light2ConstantBufferDesc,
				nullptr,
				&lightConstantBuffer
			)
		);
	}
#pragma endregion

#pragma region INSTANCES
	instanceCount = 900;
	instanceList.resize(instanceCount);

	pillarType1InstanceList.resize(instanceCount);

	D3D11_BUFFER_DESC instanceBufferDesc;
	ZeroMemory(&instanceBufferDesc, sizeof(instanceBufferDesc));
	instanceBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	instanceBufferDesc.ByteWidth = sizeof(instancePositionStructure)*instanceCount;
	instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	instanceBufferDesc.CPUAccessFlags = 0;
	instanceBufferDesc.MiscFlags = 0;
	instanceBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA instanceData = { 0 };
	instanceData.pSysMem = instanceList.data();
	instanceData.SysMemPitch = 0;
	instanceData.SysMemSlicePitch = 0;

	m_deviceResources->GetD3DDevice()->CreateBuffer(&instanceBufferDesc, &instanceData, instanceBuffer.GetAddressOf());

	geoCubeLightInstanceList.resize(instanceCount);
	m_deviceResources->GetD3DDevice()->CreateBuffer(&instanceBufferDesc, &instanceData, geoInstanceBuffer.GetAddressOf());

	geoInstanceCount = 30;
	geoInstanceList.resize(geoInstanceCount);
	instanceBufferDesc.ByteWidth = sizeof(geoInstanceStructure)*geoInstanceCount;
	m_deviceResources->GetD3DDevice()->CreateBuffer(&instanceBufferDesc, &instanceData, cubeLightInstanceBuffer.GetAddressOf());

#pragma endregion

#pragma region PILLARTYPE1
	bool pillarType1Loaded = pillarType1_loader.loadMaterialOBJ(m_deviceResources, "Assets\\Pillar\\pillarType1.obj");

	if (pillarType1Loaded) {
		// Load shaders asynchronously.
		//auto loadPillerType1PSTask = DX::ReadDataAsync(L"PillarType1PixelShader.cso");

		// After the pixel shader file is loaded, create the shader and constant buffer.
		//auto createGrenMarblePSTask = loadGreenMarblePSTask.then([this](const std::vector<byte>& fileData) {
		//	DX::ThrowIfFailed(
		//		m_deviceResources->GetD3DDevice()->CreatePixelShader(
		//			&fileData[0],
		//			fileData.size(),
		//			nullptr,
		//			&greenMarble_pixelShader
		//		)
		//	);
		//});

		//CD3D11_BUFFER_DESC light2ConstantBufferDesc(sizeof(Lighting), D3D11_BIND_CONSTANT_BUFFER);
		//DX::ThrowIfFailed(
		//	m_deviceResources->GetD3DDevice()->CreateBuffer(
		//		&light2ConstantBufferDesc,
		//		nullptr,
		//		&lightConstantBuffer
		//	)
		//);
	}
#pragma endregion

#pragma region GEOMETRYSHADER
	//Sample Hull and Domain shaders
	auto loadSampleHSTask = DX::ReadDataAsync(L"SampleHullShader.cso");
	auto loadSampleDSTask = DX::ReadDataAsync(L"SampleDomainShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createSampleHSTask = loadSampleHSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateHullShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				sampleHullShader.GetAddressOf()
			)
		);
	});
	
	auto createSampleDSTask = loadSampleDSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateDomainShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				sampleDomainShader.GetAddressOf()
			)
		);
	});

	//Model Loaded Hull and Domain shaders, with matching vertex shader
	auto loadGeoVSTask = DX::ReadDataAsync(L"GeoVertexShader.cso");
	auto loadGeoSkyboxPSTask = DX::ReadDataAsync(L"GeoSkyboxPixelShader.cso");
	auto loadGeoHSTask = DX::ReadDataAsync(L"geoHullShader.cso");
	auto loadGeoDSTask = DX::ReadDataAsync(L"geoDomainShader.cso");


	auto createGeoVSTask = loadGeoVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				geoVertexShader.GetAddressOf()
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC geoInstanceVertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT ,D3D11_INPUT_PER_VERTEX_DATA,0 },
			{ "TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
			{ "BINORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
			{ "INSTANCE",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_INSTANCE_DATA,1 },
			{ "INSTANCE",1,DXGI_FORMAT_R32G32B32A32_FLOAT,1,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_INSTANCE_DATA,1 },
			{ "INSTANCE",2,DXGI_FORMAT_R32G32B32A32_FLOAT,1,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_INSTANCE_DATA,1 },
			{ "INSTANCE",3,DXGI_FORMAT_R32G32B32A32_FLOAT,1,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_INSTANCE_DATA,1 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				geoInstanceVertexDesc,
				ARRAYSIZE(geoInstanceVertexDesc),
				&fileData[0],
				fileData.size(),
				&geoInstanceInputLayout
			)
		);
	});

	auto createGeoSkyboxPSTask = loadGeoSkyboxPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				geoSkyboxPixelShader.GetAddressOf()
			)
		);
	});

	auto createGeoHSTask = loadGeoHSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateHullShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				geoHullShader.GetAddressOf()
			)
		);
	});

	auto createGeoDSTask = loadGeoDSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateDomainShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				geoDomainShader.GetAddressOf()
			)
		);
	});
#pragma endregion

#pragma region GEOCUBELIGHT
	bool geoCubeLightLoaded = geoCubeLight.loadMaterialOBJ(m_deviceResources, "Assets\\halfUnitCube\\halfUnitCube.obj");
#pragma endregion

#pragma region THREADJOINING
	skyboxTextureThread.join();
	cubeLightDiffuseThread.join();
	greenMarbleDiffuseThread.join();
	greenMarbleNormalThread.join();
	pillarType1DiffuseThread.join();
	pillarType1NormalThread.join();
#pragma endregion

	// Once the cube is loaded, the object is ready to be rendered.
	createCubeTask.then([this] () {
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	//Base program cleanup
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();
	m_constantBuffer.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();

	//States cleanup
	m_rasterizerState.Reset();
	wireframeRasterState.Reset();
	linearSamplerState.Reset();
	anisotropicSamplerState.Reset();

	//Skybox cleanup
	skyboxTexture.Reset();
	skyboxShaderResourceView.Reset();
	skyboxInputLayout.Reset();
	skyboxVertexShader.Reset();
	skyboxPixelShader.Reset();
	skyboxVertexBuffer.Reset();
	skyboxIndexBuffer.Reset();

	//CubeLight
	cubelightTexture.Reset();
	cubelightShaderResourceView.Reset();

	//Green Marble
	greenMarble_loader.~ModelLoader();
	greenMarble_pixelShader.Reset();
	greenMarbleDiffuseTexture.Reset();
	greenMarbleNormalTexture.Reset();
	greenMarbleDiffuseSRV.Reset();
	greenMarbleNormalSRV.Reset();

	//Lighting cleanup
	lightConstantBuffer.Reset();

	//Instancing cleanup
	instanceBuffer.Reset();
	instanceVertexShader.Reset();
	instanceList.clear();
	geoInstanceList.clear();
	instanceInputLayout.Reset();
	geoInstanceInputLayout.Reset();

	//Pillar type 1
	pillarType1DiffuseSRV.Reset();
	pillarType1DiffuseTexture.Reset();
	pillarType1InstanceList.clear();

	//Geometry Shader
	sampleHullShader.Reset();
	sampleDomainShader.Reset();
	geoVertexShader.Reset();
	geoSkyboxPixelShader.Reset();
	geoHullShader.Reset();
	geoDomainShader.Reset();

	//GeoCubeLight
	geoCubeLight.~ModelLoader();
	geoInstanceBuffer.Reset();
	geoCubeLightInstanceList.clear();
	cubeLightInstanceBuffer.Reset();
}