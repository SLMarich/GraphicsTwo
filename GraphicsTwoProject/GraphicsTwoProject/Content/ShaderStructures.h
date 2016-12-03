#pragma once

namespace GraphicsTwoProject
{
	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};

	//Send per vertex data (position, uv, and normal) to vertex shader
	struct VertexUVNormTanBi
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
		DirectX::XMFLOAT3 norm;
		DirectX::XMFLOAT3 tangent;
		DirectX::XMFLOAT3 binormal;
	};

	//Send per pixel data for lighting to pixel shader
	struct Lighting {
		DirectX::XMFLOAT4	pointLightColor;
		DirectX::XMFLOAT4	pointLightPosition;
		DirectX::XMFLOAT4	ambientTerm;
		DirectX::XMFLOAT4	cameraPosition;

		DirectX::XMFLOAT4	spotlightColor;
		DirectX::XMFLOAT3	spotlightDirection;
		float				spotlightConeRatio;
		DirectX::XMFLOAT3	spotlightPosition;
		float				lightRange;

		DirectX::XMFLOAT4X4	pointRotationMatrix;

	};

	struct instancePositionStructure {
		DirectX::XMFLOAT3	position;
		DirectX::XMFLOAT4X4	rotation;
	};
	struct geoInstanceStructure {
		DirectX::XMFLOAT4X4	matrix;
	};
}