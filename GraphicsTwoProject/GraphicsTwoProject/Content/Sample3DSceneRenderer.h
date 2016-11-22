#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"

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

		//Skybox
		Microsoft::WRL::ComPtr<ID3D11Resource>		skyboxTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	skyboxShaderResourceView;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	skyboxInputLayout;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	skyboxVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	skyboxPixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		skyboxVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		skyboxIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>	linearSamplerState;
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

		DirectX::XMFLOAT4X4 world, camera, projection;// , camera1, projection1;
	};
}

