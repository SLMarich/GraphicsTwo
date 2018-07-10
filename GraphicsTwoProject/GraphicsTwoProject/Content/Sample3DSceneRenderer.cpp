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

bool Sample3DSceneRenderer::instanceMatrixSortFunction(geoInstanceStructure const &a, geoInstanceStructure const &b) {
	XMFLOAT3 posA(a.matrix._14, a.matrix._24, a.matrix._34);
	XMFLOAT3 posB(b.matrix._14, b.matrix._24, b.matrix._34);
	XMFLOAT3 posCamera(camera._41, camera._42, camera._43);
	float distA = sqrt(pow((posA.x - posCamera.x), 2) + pow((posA.y - posCamera.y), 2) + pow((posA.z - posCamera.z), 2));
	float distB = sqrt(pow((posB.x - posCamera.x), 2) + pow((posB.y - posCamera.y), 2) + pow((posB.z - posCamera.z), 2));
	return (distA < distB);
}

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

	DirectX::XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);

	DirectX::XMStoreFloat4x4(
		&projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
	);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 1.5f, -10.0f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, 1.0f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	DirectX::XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
	DirectX::XMStoreFloat4x4(&camera, XMMatrixLookAtRH(eye, at, up));
#pragma endregion

#pragma region MINIMAPVIEWPROJECTION
	float aspectRatio1 = (outputSize.Width*0.3f) / (outputSize.Height*0.3f);
	float fovAngleY1 = 70.0f * XM_PI / 180.0f;

	if (aspectRatio1 < 1.0f)
	{
		fovAngleY1 *= 2.0f;
	}

	XMMATRIX perspectiveMatrix1 = XMMatrixPerspectiveFovRH(
		fovAngleY1,
		aspectRatio1,
		0.01f,
		100.0f
	);

	XMFLOAT4X4 orientation1 = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix1 = XMLoadFloat4x4(&orientation1);

	DirectX::XMStoreFloat4x4(
		&projection1,
		XMMatrixTranspose(perspectiveMatrix1 * orientationMatrix1)
	);
	
	static const XMVECTORF32 eye1 = { 0.0f, 10.0f, 0.0f, 0.0f };
	static const XMVECTORF32 at1 = { 0.0f, -1.0f, 0.0f, 0.0f };
	static const XMVECTORF32 up1 = { 0.0f, 1.0f, 0.0f, 0.0f };

	DirectX::XMStoreFloat4x4(&camera1, XMMatrixLookAtRH(eye1, at1, up1));
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
	
	if (buttons['W'] || buttons['E']) {
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
			//newCam.r[3] = XMLoadFloat4(&XMFLOAT4(0.0f, 0.f, 0.0f, 1.0f));
			newCam.r[3].m128_f32[0] = 0.0f;
			newCam.r[3].m128_f32[1] = 0.0f;
			newCam.r[3].m128_f32[2] = 0.0f;
			newCam.r[3].m128_f32[3] = 1.0f;
			newCam = XMMatrixRotationX(-diffy*0.01f) * newCam * XMMatrixRotationY(-diffx*0.01f);
			newCam.r[3] = pos;
		}
	}

	//Stay above ground
	if (newCam.r[3].m128_f32[1] < -1.9f) {
		newCam.r[3].m128_f32[1] = -1.9f;
	}

	
	DirectX::XMStoreFloat4x4(&camera, newCam);
	DirectX::XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, newCam)));
	//XMStoreFloat4x4(&m_constantBufferData.view, newCam);
	//Move skybox to camera
	XMMATRIX camerapos = XMMatrixTranslation(camera._41, camera._42, camera._43);
	DirectX::XMStoreFloat4x4(&skyboxModel, XMMatrixTranspose(camerapos));

	mouse_move = false;
#pragma endregion

#pragma region CUBELIGHTUPDATE
	DirectX::XMStoreFloat4x4(&cubelightModel, DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&cubelightModel, XMMatrixMultiply(XMMatrixTranspose(XMMatrixRotationX((XM_2PI * 1.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))),
		XMMatrixMultiply(XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))),
			XMMatrixTranspose(XMMatrixRotationZ((XM_2PI * 1.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))))));
	//XMStoreFloat4x4(&cubelightModel, XMMatrixMultiply(XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))),
	//	XMMatrixTranspose(XMMatrixRotationZ((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	//XMStoreFloat4x4(&cubelightModel, XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))));
	//cubelightModel._24 = cubelightModel._24 - 2.0f;

	//XMStoreFloat4x4(&geoCubeLight.modelMatrix, XMLoadFloat4x4(&cubelightModel));
#pragma endregion

#pragma region GREENMARBLEUPDATE
	DirectX::XMStoreFloat4x4(&greenMarble_loader.modelMatrix, XMMatrixIdentity());
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
	//if (!instanceTracerSlower)
	//	instanceTracer++;
	//instanceTracerSlower = !instanceTracerSlower;
	//if (instanceTracer >= activeInstances) {
	//	instanceTracer = 0;
	//}

	XMFLOAT4X4 spotlightPosition;
	XMFLOAT4X4 spotlightDirection;
	//XMStoreFloat4x4(&spotlightPosition, XMMatrixMultiply(XMMatrixTranslation(15.0f, 0.0f, 15.0f), XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	//XMStoreFloat4x4(&spotlightDirection, XMMatrixMultiply(XMMatrixTranslation(0.5f, -1.0f, 0.5f), XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	DirectX::XMStoreFloat4x4(&spotlightPosition, XMMatrixMultiply(XMMatrixTranslation(15.0f, 0.0f, 15.0f), XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 1.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	DirectX::XMStoreFloat4x4(&spotlightDirection, XMMatrixMultiply(XMMatrixTranslation(0.5f, -1.0f, 0.5f), XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 1.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	//sampleLight.spotlightPosition = DirectX::XMFLOAT3(instanceList[instanceTracer].position.x, 0.0f, instanceList[instanceTracer].position.z);
	sampleLight.spotlightPosition = DirectX::XMFLOAT3(spotlightPosition._41, spotlightPosition._42, spotlightPosition._43);
	sampleLight.spotlightDirection = DirectX::XMFLOAT3(-spotlightDirection._41, -spotlightDirection._42, -spotlightDirection._43);

	//XMStoreFloat4x4(&sampleLight.pointRotationMatrix, XMMatrixMultiply(XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))),
	//	XMMatrixTranspose(XMMatrixRotationZ((XM_2PI * 2.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));
	DirectX::XMStoreFloat4x4(&sampleLight.pointRotationMatrix, XMLoadFloat4x4(&cubelightModel));

	DirectX::XMStoreFloat4x4(&sampleLight.pointRotationMatrix, XMMatrixInverse(&XMMatrixDeterminant(XMLoadFloat4x4(&sampleLight.pointRotationMatrix)), XMLoadFloat4x4(&sampleLight.pointRotationMatrix)));
#pragma endregion

#pragma region INSTANCEUPDATES
#pragma region GROUNDUPDATES
	//float infiniteGroundX = camera._41, infiniteGroundZ = camera._43;
	float infiniteGroundX = 0.0f, infiniteGroundZ = 0.0f;
	infiniteGroundX = floorf(infiniteGroundX*0.25f)*4.0f;
	infiniteGroundZ = floorf(infiniteGroundZ*0.25f)*4.0f;

	activeInstances = 900;
	for (unsigned int i = 0; i < 30; i++) {
		for (unsigned int j = 0; j < 30; j++) {
			instanceList[i * 30 + j].position = DirectX::XMFLOAT3((float)(i + i) - 30.0f + infiniteGroundX, -2.0f, (float)(j + j) - 30.0f + infiniteGroundZ);
			XMMATRIX groundRotation = XMMatrixMultiply(XMMatrixTranslation(instanceList[i * 30 + j].position.x, instanceList[i * 30 + j].position.y, instanceList[i * 30 + j].position.z), XMLoadFloat4x4(&cubelightModel));
			DirectX::XMStoreFloat4x4(&instanceList[i * 30 + j].rotation, XMMatrixIdentity());
			if (groundMove){
				DirectX::XMStoreFloat4x4(&instanceList[i * 30 + j].rotation, XMMatrixTranspose(groundRotation));
				//instanceList[i * 30 + j].position = XMFLOAT3(groundRotation.r[3].m128_f32[0], groundRotation.r[3].m128_f32[1], groundRotation.r[3].m128_f32[2]);
				instanceList[i * 30 + j].position = XMFLOAT3(instanceList[i * 30 + j].position.x *2.0f, instanceList[i * 30 + j].position.y, instanceList[i * 30 + j].position.z*2.0f);
			}
		}
	}
	for (unsigned int i = 11; i < 20; i++) {
		for (unsigned int j = 11; j < 20; j++) {
			instanceList[30*i+j] = instanceList[0];
		}
	}
	instanceList[15*30+15] = instanceList[15*30+15+1];
#pragma endregion
#pragma region PILLARINSTANCEUPDATES
	activePillarType1Instances = 450;
	for (unsigned int i = 0; i < 15; i++) {
		for (unsigned int j = 0; j < 15; j++) {
			pillarType1InstanceList[i*15+j] = instanceList[i*60+j*2];
			//pillarType1InstanceList[i * 15 + j].position = DirectX::XMFLOAT3(pillarType1InstanceList[i * 15 + j].position.x + 0.5f, pillarType1InstanceList[i * 15 + j].position.y, pillarType1InstanceList[i * 15 + j].position.z + 0.5f);
			pillarType1InstanceList[i * 15 + j].position = DirectX::XMFLOAT3(pillarType1InstanceList[i * 15 + j].position.x, pillarType1InstanceList[i * 15 + j].position.y, pillarType1InstanceList[i * 15 + j].position.z);

			pillarType1InstanceList[(i * 15 + j) + 450] = pillarType1InstanceList[i * 15 + j];
			pillarType1InstanceList[(i * 15 + j) + 450].position = XMFLOAT3(pillarType1InstanceList[(i * 15 + j) + 450].position.x, pillarType1InstanceList[(i * 15 + j) + 450].position.y + 3.25f/*2.857f*/, pillarType1InstanceList[(i * 15 + j) + 450].position.z);
		}
	}

	for (unsigned int i = 5; i < 11; i++) {
		for (unsigned int j = 5; j < 11; j++) {
			pillarType1InstanceList[i * 15 + j] = pillarType1InstanceList[0];
		}
	}
	//pillarType1InstanceList[0].position = DirectX::XMFLOAT3(-5.0f, 0.0f, -5.0f);
	//pillarType1InstanceList[1].position = DirectX::XMFLOAT3(5.0f, 0.0f, -5.0f);
	//pillarType1InstanceList[2].position = DirectX::XMFLOAT3(-5.0f, 0.0f, 5.0f);
	//pillarType1InstanceList[3].position = DirectX::XMFLOAT3(5.0f, 0.0f, 5.0f);
#pragma endregion
#pragma region GEOCUBEINSTANCEUPDATES
	activeGeoCubeInstances = 1;
	//geoCubeLightInstanceList[0].position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	//XMStoreFloat4x4(&geoCubeLightInstanceList[0].rotation, /*XMMatrixTranspose(*/XMLoadFloat4x4(&cubelightModel));
	//geoCubeLightInstanceList[1].position = XMFLOAT3(3.0f, 1.0f, 0.0f);
	//XMStoreFloat4x4(&geoCubeLightInstanceList[1].rotation, XMMatrixIdentity());
	//geoCubeLightInstanceList[2].position = XMFLOAT3(3.0f, 2.0f, 0.0f);
	//XMStoreFloat4x4(&geoCubeLightInstanceList[2].rotation, XMMatrixTranspose(XMLoadFloat4x4(&cubelightModel)));
	//XMStoreFloat4x4(&geoCubeLightInstanceList[0].rotation, XMMatrixIdentity());
	//XMStoreFloat4x4(&geoCubeLightInstanceList[0].rotation, XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&geoInstanceList[0].matrix,
		XMMatrixTranspose(
			XMMatrixMultiply(
				XMMatrixTranspose(XMLoadFloat4x4(&cubelightModel)),
				XMMatrixTranslation(0.0f, 0.0f, 0.0f))));
#pragma endregion
#pragma region GLASSSPHEREINSTANCEUPDATES
	activeGlassSphereInstances = 14;
	
	XMFLOAT4X4 glassSphereMover;
	DirectX::XMStoreFloat4x4(&glassSphereMover, XMMatrixMultiply(XMMatrixTranslation(15.0f, 0.0f, 15.0f), XMMatrixTranspose(XMMatrixRotationY((XM_2PI * 8.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width)))));

	DirectX::XMStoreFloat4x4(&glassSphereInstanceList[0].matrix,
		XMMatrixTranspose(
			XMMatrixMultiply(
				XMMatrixTranspose(XMLoadFloat4x4(&cubelightModel)),
				XMMatrixMultiply(XMMatrixTranslation(glassSphereMover._41*.1f, glassSphereMover._43*.1f + 5.0f, glassSphereMover._41*.1f),
					XMMatrixRotationY((XM_2PI * 4.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))))));
	DirectX::XMStoreFloat4x4(&glassSphereInstanceList[1].matrix,
		XMMatrixTranspose(
			XMMatrixMultiply(
				XMMatrixTranspose(XMLoadFloat4x4(&cubelightModel)),
				XMMatrixMultiply(XMMatrixTranslation(glassSphereMover._43*.1f, glassSphereMover._41*.1f + 5.0f, -glassSphereMover._43*.1f),
					XMMatrixRotationY((XM_2PI * 4.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))))));
	DirectX::XMStoreFloat4x4(&glassSphereInstanceList[2].matrix,
		XMMatrixTranspose(
			XMMatrixMultiply(
				XMMatrixTranspose(XMLoadFloat4x4(&cubelightModel)),
				XMMatrixMultiply(XMMatrixTranslation(-glassSphereMover._41*.1f, -glassSphereMover._43*.1f + 5.0f, -glassSphereMover._41*.1f),
					XMMatrixRotationY((XM_2PI * 4.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))))));
	DirectX::XMStoreFloat4x4(&glassSphereInstanceList[3].matrix,
		XMMatrixTranspose(
			XMMatrixMultiply(
				XMMatrixTranspose(XMLoadFloat4x4(&cubelightModel)),
				XMMatrixMultiply(XMMatrixTranslation(-glassSphereMover._43*.1f, -glassSphereMover._41*.1f + 5.0f, glassSphereMover._43*.1f),
					XMMatrixRotationY((XM_2PI * 4.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))))));

	for (unsigned int i = 4; i < activeGlassSphereInstances; i++) {
		DirectX::XMStoreFloat4x4(&glassSphereInstanceList[i].matrix,
			XMMatrixTranspose(
				XMMatrixMultiply(
					XMMatrixTranspose(XMLoadFloat4x4(&cubelightModel)),
					XMMatrixMultiply(XMMatrixTranslation(5.0f, 5.0f*(float)(i-4), 5.0f),
						XMMatrixRotationY((XM_2PI * 0.0f * 65.0f * (float)(timer.GetTotalSeconds()) / m_deviceResources->GetOutputSize().Width))))));
	}

	geoInstanceStructure sortTemp;
	for (unsigned int i = 0; i < activeGlassSphereInstances - 1; i++) {
		for (unsigned int j = 0; j < activeGlassSphereInstances - 1; j++) {
			if (instanceMatrixSortFunction(glassSphereInstanceList[j], glassSphereInstanceList[j + 1])) {
				sortTemp = glassSphereInstanceList[j];
				glassSphereInstanceList[j] = glassSphereInstanceList[j + 1];
				glassSphereInstanceList[j + 1] = sortTemp;
			}
		}
	}
#pragma endregion
#pragma region SQUAREPOOLINSTANCEUPDATES
	activeSquarePoolInstances = 1;

	//XMStoreFloat4x4(&squarePoolInstanceList[0].matrix, XMMatrixTranspose(XMMatrixTranslation(-41.0f, -4.0f, 0.0f)));
	DirectX::XMStoreFloat4x4(&squarePoolInstanceList[0].matrix, XMMatrixTranspose(XMMatrixTranslation(0.0f, -4.0f, 0.0f)));
#pragma endregion
#pragma region POOLWATERUPDATES
	poolWaterOffsetTimer--;
	if (!poolWaterOffsetTimer) {
		poolWaterVOffset = -poolWaterVOffset;
		poolWaterOffsetTimer = 200;
	}
	for (unsigned int i = 0; i < squarePoolWater.materialCount; i++) {
		for (unsigned int j = 0; j < squarePoolWater.modelMaterialFaceVerts[i].size(); j++) {
			squarePoolWater.modelMaterialFaceVerts[i][j].uv.x += poolWaterUOffset;
			squarePoolWater.modelMaterialFaceVerts[i][j].uv.y += poolWaterVOffset;
		}
	}
#pragma endregion
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
	DirectX::XMStoreFloat4x4(&cubeModel, XMMatrixTranspose(XMMatrixRotationY(radians)));
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
	D3D11_VIEWPORT views[2];
	views[0] = m_deviceResources->GetScreenViewport();
	views[1] = m_deviceResources->GetScreenViewport1();
	for (unsigned int viewTracker = 0; viewTracker < 2; viewTracker++) {
		m_deviceResources->GetD3DDeviceContext()->RSSetViewports(1, &views[viewTracker]);
		//if (viewTracker == 3) {
		//	//Set camera view
		//	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, XMLoadFloat4x4(&camera))));
		//	//Set camera projection
		//	XMStoreFloat4x4(&m_constantBufferData.projection, XMLoadFloat4x4(&projection));
		//	m_deviceResources->GetD3DDeviceContext()->OMSetRenderTargets(1, squarePoolWaterRefractionRTV.GetAddressOf(), baseDSV.Get());
		//}
		//if (viewTracker == 4) {
		//	//Set camera view
		//	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, XMLoadFloat4x4(&camera))));
		//	//Set camera projection
		//	XMStoreFloat4x4(&m_constantBufferData.projection, XMLoadFloat4x4(&projection));
		//	m_deviceResources->GetD3DDeviceContext()->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, 1.0f, 0);
		//	m_deviceResources->GetD3DDeviceContext()->OMSetRenderTargets(1, squarePoolWaterReflectionRTV.GetAddressOf(), baseDSV.Get());
		//}
		if (viewTracker == 0) {
			//Set camera view
			DirectX::XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, XMLoadFloat4x4(&camera))));
			//Set camera projection
			DirectX::XMStoreFloat4x4(&m_constantBufferData.projection, XMLoadFloat4x4(&projection));
			
			//For reflections
			//ID3D11RenderTargetView* rtvs[2];
			//m_deviceResources->GetD3DDeviceContext()->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, 1.0f, 0);
			//m_deviceResources->GetD3DDeviceContext()->OMSetRenderTargets(1, baseRTV.GetAddressOf(), baseDSV.Get());
		}
#pragma region STATESETUP
		//Set rasterizer state
		if (!wireframeEnabled)
			m_deviceResources->GetD3DDeviceContext()->RSSetState(m_rasterizerState.Get());
		else
			m_deviceResources->GetD3DDeviceContext()->RSSetState(wireframeRasterState.Get());
#pragma endregion

		if (viewTracker == 1) {
			//Set camera view
			//XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(XMLoadFloat4x4(&camera1)), XMLoadFloat4x4(&camera1))));
			DirectX::XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, XMMatrixMultiply(XMMatrixRotationX((XM_2PI * 4.0f * -180.0f / (m_deviceResources->GetOutputSize().Width*0.3f))),
				XMMatrixTranslation(camera._41, 15.0f, camera._43)))));
			//Set camera projection
			DirectX::XMStoreFloat4x4(&m_constantBufferData.projection, XMLoadFloat4x4(&projection1));
			//XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixIdentity());

			//Activate wireframe mode for minimap
			m_deviceResources->GetD3DDeviceContext()->RSSetState(wireframeRasterState.Get());
		}

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
			DirectX::XMStoreFloat4x4(&m_constantBufferData.model, DirectX::XMLoadFloat4x4(&cubelightModel));
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
				DirectX::XMStoreFloat4x4(&m_constantBufferData.model, DirectX::XMMatrixIdentity());
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
				strides[1] = sizeof(geoInstanceStructure);
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
			DirectX::XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMLoadFloat4x4(&greenMarble_loader.modelMatrix)));
			context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
			context->UpdateSubresource1(lightConstantBuffer.Get(), 0, NULL, &sampleLight, 0, 0, 0);
			context->UpdateSubresource1(instanceBuffer.Get(), 0, NULL, instanceList.data(), 0, 0, 0);
			context->UpdateSubresource1(greenMarble_loader.vertexBuffers[i].Get(), 0, NULL, greenMarble_loader.modelMaterialFaceVerts[i].data(), 0, 0, 0);

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
			DirectX::XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixIdentity());
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

#pragma region SQUAREPOOLDRAW
#pragma region SQUAREPOOLTILES
		for (unsigned int i = 0; i < squarePool.materialCount; i++) {
			//m_constantBufferData.model = greenMarbleModel;
			//XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMLoadFloat4x4(&greenMarbleModel)));
			DirectX::XMStoreFloat4x4(&m_constantBufferData.model, XMLoadFloat4x4(&squarePool.modelMatrix));
			context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
			context->UpdateSubresource1(lightConstantBuffer.Get(), 0, NULL, &sampleLight, 0, 0, 0);
			context->UpdateSubresource1(cubeLightInstanceBuffer.Get(), 0, NULL, squarePoolInstanceList.data(), 0, 0, 0);

			unsigned int strides[2];
			unsigned int offsets[2];
			ID3D11Buffer* bufferPointers[2];
			strides[0] = sizeof(VertexUVNormTanBi);
			strides[1] = sizeof(geoInstanceStructure);
			offsets[0] = 0;
			offsets[1] = 0;
			bufferPointers[0] = squarePool.vertexBuffers[i].Get();
			bufferPointers[1] = cubeLightInstanceBuffer.Get();
			//context->IASetVertexBuffers(0, 1, plain_loader.vertexBuffers[i].GetAddressOf(), &stride, &offset);
			context->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);
			context->IASetIndexBuffer(squarePool.indexBuffers[i].Get(), DXGI_FORMAT_R16_UINT/*Each index is one 16-bit unsigned short.*/, 0);


			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context->IASetInputLayout(instanceMatrixInputLayout.Get());

			// Attach our vertex shader.
			context->VSSetShader(
				instanceMatrixVertexShader.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);

			ID3D11ShaderResourceView* squarePoolViews[] = { squarePoolDiffuseSRV.Get(), squarePoolNormalSRV.Get(), cubelightShaderResourceView.Get(), squarePoolMossDiffuseSRV.Get(), squarePoolMossNormalSRV.Get() };
			context->PSSetShaderResources(0, 5, squarePoolViews);

			//Set samplers
			ID3D11SamplerState* squarePoolSamplers[] = { anisotropicSamplerState.Get(), linearSamplerState.Get() };
			context->PSSetSamplers(0, 2, squarePoolSamplers);

			// Attach our pixel shader.
			context->PSSetShader(
				squarePoolPixelShader.Get(),
				nullptr,
				0
			);

			//Attach constant buffer

			context->PSSetConstantBuffers(0, 1, lightConstantBuffer.GetAddressOf());

			// Draw the objects.
			context->DrawIndexedInstanced(squarePool.modelMaterialFaceVerts[i].size(), activeSquarePoolInstances, 0, 0, 0);
		}
#pragma endregion
#pragma region SQUAREPOOLWATER
		for (unsigned int i = 0; i < squarePoolWater.materialCount; i++) {
			//m_constantBufferData.model = greenMarbleModel;
			//XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMLoadFloat4x4(&greenMarbleModel)));
			DirectX::XMStoreFloat4x4(&m_constantBufferData.model, XMLoadFloat4x4(&squarePoolWater.modelMatrix));
			context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
			context->UpdateSubresource1(lightConstantBuffer.Get(), 0, NULL, &sampleLight, 0, 0, 0);
			context->UpdateSubresource1(cubeLightInstanceBuffer.Get(), 0, NULL, squarePoolInstanceList.data(), 0, 0, 0);
			context->UpdateSubresource1(squarePoolWater.vertexBuffers[i].Get(), 0, NULL, squarePoolWater.modelMaterialFaceVerts[i].data(), 0, 0, 0);

			unsigned int strides[2];
			unsigned int offsets[2];
			ID3D11Buffer* bufferPointers[2];
			strides[0] = sizeof(VertexUVNormTanBi);
			strides[1] = sizeof(geoInstanceStructure);
			offsets[0] = 0;
			offsets[1] = 0;
			bufferPointers[0] = squarePoolWater.vertexBuffers[i].Get();
			bufferPointers[1] = cubeLightInstanceBuffer.Get();
			//context->IASetVertexBuffers(0, 1, plain_loader.vertexBuffers[i].GetAddressOf(), &stride, &offset);
			context->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);
			context->IASetIndexBuffer(squarePoolWater.indexBuffers[i].Get(), DXGI_FORMAT_R16_UINT/*Each index is one 16-bit unsigned short.*/, 0);


			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context->IASetInputLayout(instanceMatrixInputLayout.Get());

			// Attach our vertex shader.
			context->VSSetShader(
				instanceMatrixVertexShader.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);

			//ID3D11ShaderResourceView* squarePoolWaterViews[] = { squarePoolWaterDiffuseSRV.Get(), squarePoolWaterNormalSRV.Get() };/*, cubelightShaderResourceView.Get(), squarePoolWaterDiffuseSRV.Get(), squarePoolWaterNormalSRV.Get() };*/
			ID3D11ShaderResourceView* squarePoolWaterViews[] = { squarePoolWaterDiffuseSRV.Get(), squarePoolWaterNormalSRV.Get(), cubelightShaderResourceView.Get() };// squarePoolWaterDiffuseSRV.Get(), squarePoolWaterNormalSRV.Get()};
			context->PSSetShaderResources(0, 3, squarePoolWaterViews);

			//Set samplers
			ID3D11SamplerState* squarePoolWaterSamplers[] = { anisotropicSamplerState.Get(), linearSamplerState.Get() };
			context->PSSetSamplers(0, 2, squarePoolWaterSamplers);

			// Attach our pixel shader.
			context->PSSetShader(
				//squarePoolPixelShader.Get(),
				squarePoolWaterPixelShader.Get(),
				nullptr,
				0
			);

			//Attach constant buffer

			context->PSSetConstantBuffers(0, 1, lightConstantBuffer.GetAddressOf());

			// Draw the objects.
			context->DrawIndexedInstanced(squarePoolWater.modelMaterialFaceVerts[i].size(), activeSquarePoolInstances, 0, 0, 0);
		}
#pragma endregion
#pragma endregion

#pragma region GLASSSPHEREDRAW
#pragma region NOGEOSHADER
		if (!geoShaderEnabled) {
			for (unsigned int i = 0; i < glassSphere.materialCount; i++) {
				// Prepare the constant buffer to send it to the graphics device.
				DirectX::XMStoreFloat4x4(&m_constantBufferData.model, DirectX::XMLoadFloat4x4(&glassSphere.modelMatrix));
				context->UpdateSubresource1(cubeLightInstanceBuffer.Get(), 0, NULL, glassSphereInstanceList.data(), 0, 0, 0);
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
				strides[1] = sizeof(geoInstanceStructure);
				offsets[0] = 0;
				offsets[1] = 0;
				bufferPointers[0] = glassSphere.vertexBuffers[i].Get();
				//bufferPointers[1] = geoInstanceBuffer.Get();
				bufferPointers[1] = cubeLightInstanceBuffer.Get();
				//context->IASetVertexBuffers(0, 1, plain_loader.vertexBuffers[i].GetAddressOf(), &stride, &offset);
				context->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);
				context->IASetIndexBuffer(glassSphere.indexBuffers[i].Get(), DXGI_FORMAT_R16_UINT/*Each index is one 16-bit unsigned short.*/, 0);

				context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				context->IASetInputLayout(instanceMatrixInputLayout.Get());

				// Attach our vertex shader.
				context->VSSetShader(
					instanceMatrixVertexShader.Get(),
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
				ID3D11ShaderResourceView* glassSphereViews[] = { glassSphereDiffuseSRV.Get(), glassSphereNormalSRV.Get() };
				context->PSSetShaderResources(0, 2, glassSphereViews);
				// Attach our pixel shader.
				context->PSSetShader(
					glassSpherePixelShader.Get(),
					nullptr,
					0
				);

				// Draw the objects.
				context->DrawIndexedInstanced(
					glassSphere.modelMaterialFaceVerts[i].size(),
					activeGlassSphereInstances,
					0,
					0,
					0
				);
			}
		}
#pragma endregion
#pragma region GEOSHADERACTIVE
		else {
			for (unsigned int i = 0; i < glassSphere.materialCount; i++) {
				DirectX::XMStoreFloat4x4(&m_constantBufferData.model, XMLoadFloat4x4(&glassSphere.modelMatrix));
				//XMStoreFloat4x4(&m_constantBufferData.model, DirectX::XMLoadFloat4x4(&geoCubeLight.modelMatrix));
				//context->UpdateSubresource1(geoInstanceBuffer.Get(), 0, NULL, geoCubeLightInstanceList.data(), 0, 0, 0);
				context->UpdateSubresource1(cubeLightInstanceBuffer.Get(), 0, NULL, glassSphereInstanceList.data(), 0, 0, 0);
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
				strides[1] = sizeof(geoInstanceStructure);
				offsets[0] = 0;
				offsets[1] = 0;
				bufferPointers[0] = glassSphere.vertexBuffers[i].Get();
				//bufferPointers[1] = geoInstanceBuffer.Get();
				bufferPointers[1] = cubeLightInstanceBuffer.Get();
				//context->IASetVertexBuffers(0, 1, plain_loader.vertexBuffers[i].GetAddressOf(), &stride, &offset);
				context->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);
				context->IASetIndexBuffer(glassSphere.indexBuffers[i].Get(), DXGI_FORMAT_R16_UINT/*Each index is one 16-bit unsigned short.*/, 0);

				context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

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
					phongDomainShader.Get(),
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
				ID3D11ShaderResourceView* glassSphereViews[] = { glassSphereDiffuseSRV.Get(), glassSphereNormalSRV.Get() };
				context->PSSetShaderResources(0, 2, glassSphereViews);
				// Attach our pixel shader.
				context->PSSetShader(
					glassSpherePixelShader.Get(),
					nullptr,
					0
				);

				// Draw the objects.
				context->DrawIndexedInstanced(
					glassSphere.modelMaterialFaceVerts[i].size(),
					activeGlassSphereInstances,
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

	}
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

#pragma region BLENDSTATE
	D3D11_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	//blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	//blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	//blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	//blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	//blendDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBlendState(&blendDesc, transparencyBlendState.GetAddressOf())
	);
	m_deviceResources->GetD3DDeviceContext()->OMSetBlendState(transparencyBlendState.Get(), NULL, 0xffffffff);
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
		L"Assets\\CubeLight\\greenMarbleTilesNormalMap50bias.dds",
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

	//glassSphere
	std::thread glassSphereDiffuseThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\7x7sphere\\glass03.dds",
		(ID3D11Resource**)glassSphereDiffuseTexture.GetAddressOf(),
		glassSphereDiffuseSRV.GetAddressOf(), 0);
	std::thread glassSphereNormalThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\7x7sphere\\glass03_55bias.dds",
		(ID3D11Resource**)glassSphereNormalTexture.GetAddressOf(),
		glassSphereNormalSRV.GetAddressOf(), 0);

	//squarePool
	std::thread squarePoolDiffuseThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\squarePool\\poolTile.dds",
		(ID3D11Resource**)squarePoolDiffuseTexture.GetAddressOf(),
		squarePoolDiffuseSRV.GetAddressOf(), 0);
	std::thread squarePoolNormalThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\squarePool\\poolTileNormal.dds",
		(ID3D11Resource**)squarePoolNormalTexture.GetAddressOf(),
		squarePoolNormalSRV.GetAddressOf(), 0);
	std::thread squarePoolMossDiffuseThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\squarePool\\seemlessMoss.dds",
		(ID3D11Resource**)squarePoolMossDiffuseTexture.GetAddressOf(),
		squarePoolMossDiffuseSRV.GetAddressOf(), 0);
	std::thread squarePoolMossNormalThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\squarePool\\seemlessMossNormal.dds",
		(ID3D11Resource**)squarePoolMossNormalTexture.GetAddressOf(),
		squarePoolMossNormalSRV.GetAddressOf(), 0);
	std::thread squarePoolWaterDiffuseThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\squarePool\\seamlessWater.dds",
		(ID3D11Resource**)squarePoolWaterDiffuseTexture.GetAddressOf(),
		squarePoolWaterDiffuseSRV.GetAddressOf(), 0);
	std::thread squarePoolWaterNormalThread(
		CreateDDSTextureFromFile, m_deviceResources->GetD3DDevice(),
		L"Assets\\squarePool\\seamlessWaterNormal.dds",
		(ID3D11Resource**)squarePoolWaterNormalTexture.GetAddressOf(),
		squarePoolWaterNormalSRV.GetAddressOf(), 0);

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
	anisotropicSamplerDesc.MaxAnisotropy = 8;
	anisotropicSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	anisotropicSamplerDesc.BorderColor[0] = 0;
	anisotropicSamplerDesc.BorderColor[1] = 0;
	anisotropicSamplerDesc.BorderColor[2] = 0;
	anisotropicSamplerDesc.BorderColor[3] = 0;
	anisotropicSamplerDesc.MinLOD = -D3D11_FLOAT32_MAX;
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

	DirectX::XMStoreFloat4x4(&cubeModel, XMMatrixIdentity());
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

#pragma region INSTANCEMATRIXVERTEXSHADER
	auto loadInstanceMatrixVSTask = DX::ReadDataAsync(L"InstanceMatrixVertexShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createInstanceMatrixVSTask = loadInstanceMatrixVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				instanceMatrixVertexShader.GetAddressOf()
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC instanceMatrixVertexDesc[] =
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
				instanceMatrixVertexDesc,
				ARRAYSIZE(instanceMatrixVertexDesc),
				&fileData[0],
				fileData.size(),
				instanceMatrixInputLayout.GetAddressOf()
			)
		);
	});
#pragma endregion

#pragma region GREENMARBLE
	bool groundMarbleLoaded = greenMarble_loader.loadMaterialOBJ(m_deviceResources, "Assets\\CubeLight\\groundQuad.obj");

	for (unsigned int i = 0; i < greenMarble_loader.modelMaterialFaceVerts.size(); i++) {
		for (unsigned int j = 0; j < greenMarble_loader.modelMaterialFaceVerts[i].size(); j++) {
			greenMarble_loader.modelMaterialFaceVerts[i][j].tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
			greenMarble_loader.modelMaterialFaceVerts[i][j].binormal = XMFLOAT3(0.0f, 0.0f, 1.0f);
		}
	}

	if (groundMarbleLoaded) {
		// Load shaders asynchronously.
		auto loadGreenMarblePSTask = DX::ReadDataAsync(L"GreenMarblePixelShader.cso");

		// After the pixel shader file is loaded, create the shader and constant buffer.
		auto createGreenMarblePSTask = loadGreenMarblePSTask.then([this](const std::vector<byte>& fileData) {
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

	glassSphereInstanceList.resize(geoInstanceCount);

	squarePoolInstanceList.resize(geoInstanceCount);
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


	auto loadPhongDSTask = DX::ReadDataAsync(L"PhongDomainShader.cso");\

	auto createPhongDSTask = loadPhongDSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateDomainShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				phongDomainShader.GetAddressOf()
			)
		);
	});

#pragma endregion

#pragma region GEOCUBELIGHT
	bool geoCubeLightLoaded = geoCubeLight.loadMaterialOBJ(m_deviceResources, "Assets\\halfUnitCube\\halfUnitCube.obj");
#pragma endregion

#pragma region GLASSSPHERE
	bool glassSphereLoaded = glassSphere.loadMaterialOBJ(m_deviceResources, "Assets\\7x7sphere\\glassSphere.obj");
	DirectX::XMStoreFloat4x4(&glassSphere.modelMatrix, XMMatrixTranspose(XMMatrixTranslation(0.0f, 2.0f, 0.0f)));

	if (glassSphereLoaded) {
		auto loadGlassSpherePSTask = DX::ReadDataAsync(L"glassSpherePixelShader.cso");
		// After the pixel shader file is loaded, create the shader and constant buffer.
		auto createGlassSpherePSTask = loadGlassSpherePSTask.then([this](const std::vector<byte>& fileData) {
			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreatePixelShader(
					&fileData[0],
					fileData.size(),
					nullptr,
					glassSpherePixelShader.GetAddressOf()
				)
			);
		});
	}
#pragma endregion

#pragma region SQUAREPOOL
	bool squarePoolLoaded = squarePool.loadMaterialOBJWithFaceNorms(m_deviceResources, "Assets\\squarePool\\squarePoolFaceNorms.obj");
	//bool squarePoolLoaded = squarePool.loadMaterialOBJ(m_deviceResources, "Assets\\squarePool\\squarePoolFaceNorms.obj");
	if (squarePoolLoaded) {
		auto loadSquarePoolPSTask = DX::ReadDataAsync(L"SquarePoolPixelShader.cso");
		// After the pixel shader file is loaded, create the shader and constant buffer.
		auto createSquarePoolPSTask = loadSquarePoolPSTask.then([this](const std::vector<byte>& fileData) {
			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreatePixelShader(
					&fileData[0],
					fileData.size(),
					nullptr,
					squarePoolPixelShader.GetAddressOf()
				)
			);
		});
	}
	bool squarePoolWaterLoaded = squarePoolWater.loadMaterialOBJ(m_deviceResources, "Assets\\squarePool\\squarePoolWater.obj");
	if (squarePoolWaterLoaded) {
		auto loadSquarePoolWaterPSTask = DX::ReadDataAsync(L"SquarePoolWaterPixelShader.cso");
		auto createSquarePoolWaterPSTask = loadSquarePoolWaterPSTask.then([this](const std::vector<byte>& fileData) {
			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreatePixelShader(
					&fileData[0],
					fileData.size(),
					nullptr,
					squarePoolWaterPixelShader.GetAddressOf()
				)
			);
		});
	}

	m_deviceResources->GetD3DDeviceContext()->OMGetRenderTargets(1, baseRTV.GetAddressOf(), baseDSV.GetAddressOf());

	D3D11_TEXTURE2D_DESC reflectionTextureDesc;
	ZeroMemory(&reflectionTextureDesc, sizeof(reflectionTextureDesc));
	reflectionTextureDesc.Width = 512;
	reflectionTextureDesc.Height = 512;
	reflectionTextureDesc.MipLevels = 1;
	reflectionTextureDesc.ArraySize = 1;
	reflectionTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	reflectionTextureDesc.SampleDesc.Count = 1;
	reflectionTextureDesc.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_QUALITY_LEVELS::D3D11_STANDARD_MULTISAMPLE_PATTERN;
	reflectionTextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
	reflectionTextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
	reflectionTextureDesc.MiscFlags = 0;

	m_deviceResources->GetD3DDevice()->CreateTexture2D(&reflectionTextureDesc, NULL, squarePoolWaterReflectionTexture.GetAddressOf());
	m_deviceResources->GetD3DDevice()->CreateTexture2D(&reflectionTextureDesc, NULL, squarePoolWaterRefractionTexture.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC refractionSRVDesc;
	ZeroMemory(&refractionSRVDesc, sizeof(refractionSRVDesc));
	refractionSRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	refractionSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	refractionSRVDesc.Texture2D.MipLevels = -1;	//-1 sets it to use all mip levels from most detailed
	refractionSRVDesc.Texture2D.MostDetailedMip = 0;
	m_deviceResources->GetD3DDevice()->CreateShaderResourceView((ID3D11Resource*)squarePoolWaterRefractionTexture.Get(), &refractionSRVDesc, squarePoolWaterRefractionSRV.GetAddressOf());
	m_deviceResources->GetD3DDevice()->CreateShaderResourceView((ID3D11Resource*)squarePoolWaterReflectionTexture.Get(), &refractionSRVDesc, squarePoolWaterReflectionSRV.GetAddressOf());

	D3D11_RENDER_TARGET_VIEW_DESC refractionRTVDesc;
	ZeroMemory(&refractionRTVDesc, sizeof(refractionRTVDesc));
	refractionRTVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	refractionRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	refractionSRVDesc.Texture2D.MipLevels = -1;
	refractionSRVDesc.Texture2D.MostDetailedMip = 0;
	m_deviceResources->GetD3DDevice()->CreateRenderTargetView((ID3D11Resource*)squarePoolWaterRefractionTexture.Get(), &refractionRTVDesc, squarePoolWaterRefractionRTV.GetAddressOf());
	m_deviceResources->GetD3DDevice()->CreateRenderTargetView((ID3D11Resource*)squarePoolWaterReflectionTexture.Get(), &refractionRTVDesc, squarePoolWaterReflectionRTV.GetAddressOf());
#pragma endregion

#pragma region THREADJOINING
	skyboxTextureThread.join();
	cubeLightDiffuseThread.join();
	greenMarbleDiffuseThread.join();
	greenMarbleNormalThread.join();
	pillarType1DiffuseThread.join();
	pillarType1NormalThread.join();
	glassSphereDiffuseThread.join();
	glassSphereNormalThread.join();
	squarePoolDiffuseThread.join();
	squarePoolNormalThread.join();
	squarePoolMossDiffuseThread.join();
	squarePoolMossNormalThread.join();
	squarePoolWaterDiffuseThread.join();
	squarePoolWaterNormalThread.join();
#pragma endregion

#pragma region MIPGENERATION
	//Unneeded, as mip levels are made in the resources before they're loaded
	//m_deviceResources->GetD3DDeviceContext()->GenerateMips(greenMarbleDiffuseSRV.Get());
	//m_deviceResources->GetD3DDeviceContext()->GenerateMips(greenMarbleNormalSRV.Get());
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
	transparencyBlendState.Reset();

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
	instanceMatrixVertexShader.Reset();
	instanceMatrixInputLayout.Reset();

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

	phongDomainShader.Reset();

	//GeoCubeLight
	geoCubeLight.~ModelLoader();
	geoInstanceBuffer.Reset();
	geoCubeLightInstanceList.clear();
	cubeLightInstanceBuffer.Reset();

	//glassSphere
	glassSphere.~ModelLoader();
	glassSphereDiffuseSRV.Reset();
	glassSphereNormalSRV.Reset();
	glassSphereDiffuseTexture.Reset();
	glassSphereNormalTexture.Reset();
	glassSpherePixelShader.Reset();
	glassSphereInstanceList.clear();

	//squarePool
	squarePool.~ModelLoader();
	squarePoolDiffuseSRV.Reset();
	squarePoolNormalSRV.Reset();
	squarePoolDiffuseTexture.Reset();
	squarePoolNormalTexture.Reset();
	squarePoolMossDiffuseSRV.Reset();
	squarePoolMossNormalSRV.Reset();
	squarePoolMossDiffuseTexture.Reset();
	squarePoolMossNormalTexture.Reset();
	squarePoolPixelShader.Reset();
	squarePoolInstanceList.clear();
	//squarePoolWater
	squarePoolWaterDiffuseSRV.Reset();
	squarePoolWaterNormalSRV.Reset();
	squarePoolWaterDiffuseTexture.Reset();
	squarePoolWaterNormalTexture.Reset();
	squarePoolWaterPixelShader.Reset();
	//water RTVs
	squarePoolWaterReflectionRTV.Reset();
	squarePoolWaterReflectionTexture.Reset();
	squarePoolWaterRefractionRTV.Reset();
	squarePoolWaterRefractionTexture.Reset();
}