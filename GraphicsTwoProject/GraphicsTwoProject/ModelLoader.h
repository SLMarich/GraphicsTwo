#pragma once

#include "..\Common\DeviceResources.h"
#include "..\Content\ShaderStructures.h"
#include "..\Common\StepTimer.h"

#include <vector>
//#include "pch.h"

struct materialNameCount {
	std::string materialName;
	unsigned int faceCount;
};

class ModelLoader
{
public:
	ModelLoader();
	~ModelLoader();
	//bool loadOBJ(const char *path, std::vector<DirectX::XMFLOAT3> &out_vertices, std::vector<DirectX::XMFLOAT2> &out_uvs, std::vector<DirectX::XMFLOAT3> &out_normals);
	//bool loadMaterialOBJ(const char *path, std::vector<DirectX::XMFLOAT3> &out_vertices, std::vector<DirectX::XMFLOAT2> &out_uvs, std::vector<DirectX::XMFLOAT3> &out_normals);

	void calculateModelVectors();
	void calculateTangentBinormal(GraphicsTwoProject::VertexUVNormTanBi *vert1, GraphicsTwoProject::VertexUVNormTanBi *vert2, GraphicsTwoProject::VertexUVNormTanBi *vert3);
	void calculateNormal(GraphicsTwoProject::VertexUVNormTanBi *vert1);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndicies;
	std::vector<DirectX::XMFLOAT3>	temp_vertices;
	std::vector<DirectX::XMFLOAT2>	temp_uvs;
	std::vector<DirectX::XMFLOAT3>	temp_normals;
	std::vector<std::string>		temp_materials;
	std::vector<materialNameCount>	materialFaceCount;
	unsigned int					materialCount;

	//Things to link to device
	Microsoft::WRL::ComPtr<ID3D11InputLayout>			inputLayout;
	std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>>	vertexBuffers;
	std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>>	indexBuffers;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>			vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>			pixelShader;
	//Microsoft::WRL::ComPtr<ID3D11Buffer>				constantBuffer;
	uint32 indexCount;
	DirectX::XMFLOAT4X4 modelMatrix;
	std::vector<GraphicsTwoProject::VertexUVNormTanBi*> modelVertices;
	//Textures
	std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>>	diffuseTextures;
	std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>>	normalTextures;
	std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>>	specularTextures;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> shaderResourceViews;
	std::vector<DirectX::XMFLOAT3> verts;
	std::vector<DirectX::XMFLOAT2> uvs;
	std::vector<DirectX::XMFLOAT3> normals;
	std::vector<std::vector<unsigned short>> indices;
	bool loadMaterialOBJ(std::shared_ptr<DX::DeviceResources> deviceResources, const char* path);
	std::vector<GraphicsTwoProject::VertexUVNormTanBi> modelFaces;
	std::vector<std::vector<GraphicsTwoProject::VertexUVNormTanBi>> modelMaterialFaceVerts;
};