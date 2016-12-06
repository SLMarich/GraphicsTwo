#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"

#include "ModelLoader.h"

namespace GraphicsTwoProject
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		void StartTracking();
		void TrackingUpdate(float positionX);
		void StopTracking();
		bool IsTracking() { return m_tracking; }


	private:
		void Rotate(float radians);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		//Additional Model Resources
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> wireframeRasterState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>	linearSamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>	anisotropicSamplerState;

		//Skybox
		Microsoft::WRL::ComPtr<ID3D11Resource>		skyboxTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	skyboxShaderResourceView;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	skyboxInputLayout;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	skyboxVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	skyboxPixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		skyboxVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		skyboxIndexBuffer;
		uint32	skyboxIndexCount;
		DirectX::XMFLOAT4X4	skyboxModel;
		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;

		// System resources for cube geometry.
		ModelViewProjectionConstantBuffer	m_constantBufferData;
		uint32	m_indexCount;
		DirectX::XMFLOAT4X4	cubeModel;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;

		DirectX::XMFLOAT4X4 world, camera, projection, camera1, projection1;

		//Sceen adjusting bools
		bool toggledChecker = false;
		bool groundMove = false;
		bool geoShaderEnabled = false;
		bool wireframeEnabled = false;

		//CubeLight
		Microsoft::WRL::ComPtr<ID3D11Resource>		cubelightTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	cubelightShaderResourceView;
		DirectX::XMFLOAT4X4							cubelightModel;

		//Green Marble
		ModelLoader greenMarble_loader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	greenMarble_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>		greenMarbleDiffuseTexture;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>		greenMarbleNormalTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	greenMarbleDiffuseSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	greenMarbleNormalSRV;

		//Lighting
		Lighting sampleLight;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	lightConstantBuffer;
		unsigned int instanceTracer;
		bool instanceTracerSlower = false;

		//Instancing
		Microsoft::WRL::ComPtr<ID3D11Buffer>	instanceBuffer;
		unsigned int activeInstances;
		unsigned int instanceCount;
		unsigned int geoInstanceCount;
		std::vector<instancePositionStructure> instanceList;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	instanceVertexShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	instanceMatrixVertexShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	instanceInputLayout;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	instanceMatrixInputLayout;
		std::vector<geoInstanceStructure>			geoInstanceList;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	geoInstanceInputLayout;

		//pillarType1
		ModelLoader pillarType1_loader;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				pillarType1DiffuseTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	pillarType1DiffuseSRV;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				pillarType1NormalTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	pillarType1NormalSRV;
		unsigned int activePillarType1Instances;
		std::vector<instancePositionStructure> pillarType1InstanceList;

		//Geometry Shaders
		Microsoft::WRL::ComPtr<ID3D11HullShader>	sampleHullShader;
		Microsoft::WRL::ComPtr<ID3D11DomainShader>	sampleDomainShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	geoVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	geoSkyboxPixelShader;
		Microsoft::WRL::ComPtr<ID3D11HullShader>	geoHullShader;
		Microsoft::WRL::ComPtr<ID3D11DomainShader>	geoDomainShader;

		//Geo compatible cubeLight
		ModelLoader geoCubeLight;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		geoInstanceBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		cubeLightInstanceBuffer;
		unsigned int activeGeoCubeInstances;
		std::vector<instancePositionStructure>		geoCubeLightInstanceList;


		//7x7sphere
		ModelLoader glassSphere;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				glassSphereDiffuseTexture;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				glassSphereNormalTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	glassSphereDiffuseSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	glassSphereNormalSRV;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			glassSpherePixelShader;
		std::vector<geoInstanceStructure>					glassSphereInstanceList;
		unsigned int activeGlassSphereInstances;

		Microsoft::WRL::ComPtr<ID3D11DomainShader>	phongDomainShader;

		//Blend State
		Microsoft::WRL::ComPtr<ID3D11BlendState>			transparencyBlendState;

		bool instanceMatrixSortFunction(geoInstanceStructure const &a, geoInstanceStructure const &b);
	};
}