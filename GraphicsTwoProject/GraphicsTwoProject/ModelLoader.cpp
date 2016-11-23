#include "pch.h"
#include "ModelLoader.h"
#include "Common\DirectXHelper.h"


ModelLoader::ModelLoader()
{
}

ModelLoader::~ModelLoader()
{
	vertexIndices.clear();
	uvIndices.clear();
	normalIndicies.clear();
	temp_vertices.clear();
	temp_normals.clear();
	temp_materials.clear();
	materialFaceCount.clear();

	inputLayout.Reset();
	for (unsigned int i = 0; i < vertexBuffers.size(); i++) {
		vertexBuffers[i].Reset();
	}
	vertexBuffers.clear();
	for (unsigned int i = 0; i < indexBuffers.size(); i++) {
		indexBuffers[i].Reset();
	}
	indexBuffers.clear();
	vertexShader.Reset();
	pixelShader.Reset();
	modelVertices.clear();

	for (unsigned int i = 0; i < diffuseTextures.size(); i++) {
		diffuseTextures[i].Reset();
	}
	diffuseTextures.clear();
	for (unsigned int i = 0; i < normalTextures.size(); i++) {
		normalTextures[i].Reset();
	}
	normalTextures.clear();
	for (unsigned int i = 0; i < specularTextures.size(); i++) {
		specularTextures[i].Reset();
	}
	specularTextures.clear();
	for (unsigned int i = 0; i < shaderResourceViews.size(); i++) {
		shaderResourceViews[i].Reset();
	}
	shaderResourceViews.clear();
	verts.clear();
	uvs.clear();
	normals.clear();
	indices.clear();

	modelFaces.clear();
	modelMaterialFaceVerts.clear();
}

//bool ModelLoader::loadOBJ(const char * path, std::vector<DirectX::XMFLOAT3> &out_vertices, std::vector<DirectX::XMFLOAT2> &out_uvs, std::vector<DirectX::XMFLOAT3> &out_normals)
//{
//	FILE * file;
//	//file = fopen(path, "r");
//	errno_t openCheck;
//	openCheck = fopen_s(&file, path, "r");
//	if (file == NULL) {
//		return false;
//	}
//	while (true) {
//		char lineHeader[128];
//		//read the first word of the line
//		int res = fscanf_s(file, "%s", lineHeader, _countof(lineHeader));
//		if (res == EOF) {
//			break;
//		}
//		//else: parse lineHeader
//
//		//Geometric verticies
//		if (strcmp(lineHeader, "v") == 0) {
//			DirectX::XMFLOAT3 vertex;
//			fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
//			temp_vertices.push_back(vertex);
//		}
//		//Texture verticies (uv coordinates)
//		else if (strcmp(lineHeader, "vt") == 0) {
//			DirectX::XMFLOAT2 uv;
//			fscanf_s(file, "%f %f\n", &uv.x, &uv.y);
//			temp_uvs.push_back(uv);
//		}
//		//Vertex normals
//		else if (strcmp(lineHeader, "vn") == 0) {
//			DirectX::XMFLOAT3 normal;
//			fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
//			temp_normals.push_back(normal);
//		}
//		//faces
//		else if (strcmp(lineHeader, "f") == 0) {
//			std::string vertex1, vertex2, vertex3;
//			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
//			int matches = fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d*\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
//			if (matches != 9) {
//				return false;
//			}
//			vertexIndices.push_back(vertexIndex[0]);
//			vertexIndices.push_back(vertexIndex[1]);
//			vertexIndices.push_back(vertexIndex[2]);
//			uvIndices.push_back(uvIndex[0]);
//			uvIndices.push_back(uvIndex[1]);
//			uvIndices.push_back(uvIndex[2]);
//			normalIndicies.push_back(normalIndex[0]);
//			normalIndicies.push_back(normalIndex[1]);
//			normalIndicies.push_back(normalIndex[2]);
//		}
//	}
//
//	//For each vertex of each triangle
//	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
//		unsigned int vertexIndex = vertexIndices[i];
//		DirectX::XMFLOAT3 vertex = temp_vertices[vertexIndex - 1];
//		out_vertices.push_back(vertex);
//	}
//	for (unsigned int i = 0; i < uvIndices.size(); i++) {
//		unsigned int uvIndex = uvIndices[i];
//		DirectX::XMFLOAT2 uv = temp_uvs[uvIndex - 1];
//		out_uvs.push_back(uv);
//	}
//	for (unsigned int i = 0; i < normalIndicies.size(); i++) {
//		unsigned int normalIndex = normalIndicies[i];
//		DirectX::XMFLOAT3 normal = temp_normals[normalIndex - 1];
//		out_normals.push_back(normal);
//	}
//	return true;
//}

//bool ModelLoader::loadMaterialOBJ(const char * path, std::vector<DirectX::XMFLOAT3>& out_vertices, std::vector<DirectX::XMFLOAT2>& out_uvs, std::vector<DirectX::XMFLOAT3>& out_normals)
//{
//	FILE * file;
//	//file = fopen(path, "r");
//	errno_t openCheck;
//	openCheck = fopen_s(&file, path, "r");
//	if (file == NULL) {
//		return false;
//	}
//
//	char currMaterialName[128];
//	materialNameCount currMatNameCount;
//	materialCount = 0;
//	currMatNameCount.faceCount = 0;
//	currMatNameCount.materialName = "empty count";
//	while (true) {
//
//		char lineHeader[128];
//		//read the first word of the line
//		int res = fscanf_s(file, "%s", lineHeader, _countof(lineHeader));
//		if (res == EOF) {
//			break;
//		}
//		//else: parse lineHeader
//
//		//Geometric verticies
//		if (strcmp(lineHeader, "v") == 0) {
//			DirectX::XMFLOAT3 vertex;
//			fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
//			temp_vertices.push_back(vertex);
//		}
//		//Texture verticies (uv coordinates)
//		else if (strcmp(lineHeader, "vt") == 0) {
//			DirectX::XMFLOAT2 uv;
//			fscanf_s(file, "%f %f\n", &uv.x, &uv.y);
//			temp_uvs.push_back(uv);
//		}
//		//Vertex normals
//		else if (strcmp(lineHeader, "vn") == 0) {
//			DirectX::XMFLOAT3 normal;
//			fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
//			temp_normals.push_back(normal);
//		}
//		//faces
//		else if (strcmp(lineHeader, "f") == 0) {
//			//std::string vertex1, vertex2, vertex3;
//			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
//			int matches = fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d*\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
//			if (matches != 9) {
//				return false;
//			}
//			vertexIndices.push_back(vertexIndex[0]);
//			vertexIndices.push_back(vertexIndex[1]);
//			vertexIndices.push_back(vertexIndex[2]);
//			uvIndices.push_back(uvIndex[0]);
//			uvIndices.push_back(uvIndex[1]);
//			uvIndices.push_back(uvIndex[2]);
//			normalIndicies.push_back(normalIndex[0]);
//			normalIndicies.push_back(normalIndex[1]);
//			normalIndicies.push_back(normalIndex[2]);
//			temp_materials.push_back(currMaterialName);
//			currMatNameCount.faceCount++;
//		}
//		//Material names
//		else if (strcmp(lineHeader, "usemtl") == 0) {
//			fscanf_s(file, "%s\n", &currMaterialName, _countof(currMaterialName));
//			materialCount++;
//			materialFaceCount.push_back(currMatNameCount);
//			currMatNameCount.materialName = currMaterialName;
//			currMatNameCount.faceCount = 0;
//		}
//	}
//
//	//Push the last material name and face count
//	materialFaceCount.push_back(currMatNameCount);
//
//	//For each vertex of each triangle
//	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
//		unsigned int vertexIndex = vertexIndices[i];
//		DirectX::XMFLOAT3 vertex = temp_vertices[vertexIndex - 1];
//		out_vertices.push_back(vertex);
//	}
//	for (unsigned int i = 0; i < uvIndices.size(); i++) {
//		unsigned int uvIndex = uvIndices[i];
//		DirectX::XMFLOAT2 uv = temp_uvs[uvIndex - 1];
//		out_uvs.push_back(uv);
//	}
//	for (unsigned int i = 0; i < normalIndicies.size(); i++) {
//		unsigned int normalIndex = normalIndicies[i];
//		DirectX::XMFLOAT3 normal = temp_normals[normalIndex - 1];
//		out_normals.push_back(normal);
//	}
//	return true;
//}

void ModelLoader::calculateModelVectors()
{
	int faceCount, index;
	GraphicsTwoProject::VertexUVNormTanBi vertex1, vertex2, vertex3;

	//For each material in the model
	for (unsigned int j = 0; j < modelMaterialFaceVerts.size(); j++) {
		//Calculate the number of faces in the model
		faceCount = modelMaterialFaceVerts[j].size() / 3;
		//Initialize inex to model data
		index = 0;

		//Go through all the faces and caluclate the tangent, binormal, and normal vectors
		for (int i = 0; i < faceCount; i++) {
			vertex1.pos.x = modelMaterialFaceVerts[j][index].pos.x;
			vertex1.pos.y = modelMaterialFaceVerts[j][index].pos.y;
			vertex1.pos.z = modelMaterialFaceVerts[j][index].pos.z;
			vertex1.uv.x = modelMaterialFaceVerts[j][index].uv.x;
			vertex1.uv.y = modelMaterialFaceVerts[j][index].uv.y;
			vertex1.norm.x = modelMaterialFaceVerts[j][index].norm.x;
			vertex1.norm.y = modelMaterialFaceVerts[j][index].norm.y;
			vertex1.norm.z = modelMaterialFaceVerts[j][index].norm.z;
			index++;

			vertex2.pos.x = modelMaterialFaceVerts[j][index].pos.x;
			vertex2.pos.y = modelMaterialFaceVerts[j][index].pos.y;
			vertex2.pos.z = modelMaterialFaceVerts[j][index].pos.z;
			vertex2.uv.x = modelMaterialFaceVerts[j][index].uv.x;
			vertex2.uv.y = modelMaterialFaceVerts[j][index].uv.y;
			vertex2.norm.x = modelMaterialFaceVerts[j][index].norm.x;
			vertex2.norm.y = modelMaterialFaceVerts[j][index].norm.y;
			vertex2.norm.z = modelMaterialFaceVerts[j][index].norm.z;
			index++;

			vertex3.pos.x = modelMaterialFaceVerts[j][index].pos.x;
			vertex3.pos.y = modelMaterialFaceVerts[j][index].pos.y;
			vertex3.pos.z = modelMaterialFaceVerts[j][index].pos.z;
			vertex3.uv.x = modelMaterialFaceVerts[j][index].uv.x;
			vertex3.uv.y = modelMaterialFaceVerts[j][index].uv.y;
			vertex3.norm.x = modelMaterialFaceVerts[j][index].norm.x;
			vertex3.norm.y = modelMaterialFaceVerts[j][index].norm.y;
			vertex3.norm.z = modelMaterialFaceVerts[j][index].norm.z;
			index++;

			//Calculate the tangent and binormal of that face
			calculateTangentBinormal(&vertex1, &vertex2, &vertex3);
			//Calculate the new normal using the tangent and binormal
			//calculateNormal(&vertex1);

			//Store the normal, tangent, and binormal for this face back in the model
			//modelMaterialFaceVerts[j][index - 1].norm = vertex1.norm;
			//modelMaterialFaceVerts[j][index - 1].tangent = vertex1.tangent;
			//modelMaterialFaceVerts[j][index - 1].binormal = vertex1.binormal;
			//
			//modelMaterialFaceVerts[j][index - 2].norm = vertex1.norm;
			//modelMaterialFaceVerts[j][index - 2].tangent = vertex1.tangent;
			//modelMaterialFaceVerts[j][index - 2].binormal = vertex1.binormal;
			//
			//modelMaterialFaceVerts[j][index - 3].norm = vertex1.norm;
			//modelMaterialFaceVerts[j][index - 3].tangent = vertex1.tangent;
			//modelMaterialFaceVerts[j][index - 3].binormal = vertex1.binormal;
		}
	}
	return;
}

void ModelLoader::calculateTangentBinormal(GraphicsTwoProject::VertexUVNormTanBi *vert1, GraphicsTwoProject::VertexUVNormTanBi *vert2, GraphicsTwoProject::VertexUVNormTanBi *vert3)
{
	DirectX::XMFLOAT3 vector1, vector2;
	DirectX::XMFLOAT2 tuVector, tvVector;
	float den;
	float length;

	//Calculate the two vector for this face
	vector1.x = vert2->pos.x - vert1->pos.x;
	vector1.y = vert2->pos.y - vert1->pos.y;
	vector1.z = vert2->pos.z - vert1->pos.z;

	vector2.x = vert3->pos.x - vert1->pos.x;
	vector2.y = vert3->pos.y - vert1->pos.y;
	vector2.z = vert3->pos.z - vert1->pos.z;

	//Calculate the texture coordinates
	tuVector.x = vert2->uv.x - vert1->uv.x;
	tvVector.x = vert2->uv.y - vert1->uv.y;

	tuVector.y = vert3->uv.x - vert1->uv.x;
	tvVector.y = vert3->uv.y - vert1->uv.y;

	//Calculate the denominator of the tangent/binormal equation
	den = 1.0f / (tuVector.x * tvVector.y - tuVector.y * tvVector.x);

	//Calculate the cross products and multiply by the coefficient to get the tangent and binormal
	vert1->tangent.x = (tvVector.y * vector1.x - tvVector.x * vector2.x) * den;
	vert1->tangent.y = (tvVector.y * vector1.y - tvVector.x * vector2.y) * den;
	vert1->tangent.z = (tvVector.y * vector1.z - tvVector.x * vector2.z) * den;

	vert1->binormal.x = (tuVector.x * vector2.x - tuVector.y * vector1.x) * den;
	vert1->binormal.y = (tuVector.x * vector2.y - tuVector.y * vector1.y) * den;
	vert1->binormal.z = (tuVector.x * vector2.z - tuVector.y * vector1.z) * den;

	//Calculate the length of this normal
	length = sqrt((vert1->tangent.x * vert1->tangent.x) + (vert1->tangent.y * vert1->tangent.y) + (vert1->tangent.z * vert1->tangent.z));
	//Normalize the normal and then store it
	vert1->tangent.x = vert1->tangent.x / length;
	vert1->tangent.y = vert1->tangent.y / length;
	vert1->tangent.z = vert1->tangent.z / length;

	//Calculate the length of binomal
	length = sqrt((vert1->binormal.x * vert1->binormal.x) + (vert1->binormal.y * vert1->binormal.y) + (vert1->binormal.z * vert1->binormal.z));
	//Normalize binormal and store it
	vert1->binormal.x = vert1->binormal.x / length;
	vert1->binormal.y = vert1->binormal.y / length;
	vert1->binormal.z = vert1->binormal.z / length;

	//Apply to other verts
	vert2->tangent = vert1->tangent;
	vert2->binormal = vert1->binormal;
	vert3->tangent = vert1->tangent;
	vert3->binormal = vert1->binormal;

	return;
}

void ModelLoader::calculateNormal(GraphicsTwoProject::VertexUVNormTanBi * vert1)
{
	float length;
	//Calculate the cross product of the tangent and binormal which will give you the normal vector
	vert1->norm.x = (vert1->tangent.y * vert1->binormal.z) - (vert1->tangent.z * vert1->binormal.y);
	vert1->norm.y = (vert1->tangent.z * vert1->binormal.x) - (vert1->tangent.x * vert1->binormal.z);
	vert1->norm.z = (vert1->tangent.x * vert1->binormal.y) - (vert1->tangent.y * vert1->binormal.x);

	//Calculate the length of the normal
	length = sqrt((vert1->norm.x * vert1->norm.x) + (vert1->norm.y * vert1->norm.y) + (vert1->norm.z * vert1->norm.z));

	//Normalize the normal
	vert1->norm.x = vert1->norm.x / length;
	vert1->norm.y = vert1->norm.y / length;
	vert1->norm.z = vert1->norm.z / length;

	return;
}

bool ModelLoader::loadMaterialOBJ(std::shared_ptr<DX::DeviceResources> deviceResources, const char *path)
{
	FILE * file;
	//file = fopen(path, "r");
	errno_t openCheck;
	openCheck = fopen_s(&file, path, "r");
	if (file == NULL) {
		return false;
	}

	char currMaterialName[128];
	materialNameCount currMatNameCount;
	materialCount = 0;
	currMatNameCount.faceCount = 0;
	currMatNameCount.materialName = "empty count";

	GraphicsTwoProject::VertexUVNormTanBi faceHolder;

	while (true) {

		char lineHeader[128];
		//read the first word of the line
		int res = fscanf_s(file, "%s", lineHeader, _countof(lineHeader));
		if (res == EOF) {
			break;
		}
		//else: parse lineHeader

		//Geometric verticies
		if (strcmp(lineHeader, "v") == 0) {
			DirectX::XMFLOAT3 vertex;
			fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		//Texture verticies (uv coordinates)
		else if (strcmp(lineHeader, "vt") == 0) {
			DirectX::XMFLOAT2 uv;
			fscanf_s(file, "%f %f\n", &uv.x, &uv.y);
			//uv.y = 1.0f - uv.y;
			temp_uvs.push_back(uv);
		}
		//Vertex normals
		else if (strcmp(lineHeader, "vn") == 0) {
			DirectX::XMFLOAT3 normal;
			fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		//faces
		else if (strcmp(lineHeader, "f") == 0) {
			//std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d*\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndicies.push_back(normalIndex[0]);
			normalIndicies.push_back(normalIndex[1]);
			normalIndicies.push_back(normalIndex[2]);
			temp_materials.push_back(currMaterialName);
			currMatNameCount.faceCount++;

			//Alternate tracking attempt: immidiately push the faces into a VertexUvNormal
			faceHolder.pos = temp_vertices[vertexIndex[0] - 1];
			faceHolder.uv = temp_uvs[uvIndex[0] - 1];
			faceHolder.norm = temp_normals[normalIndex[0] - 1];
			modelFaces.push_back(faceHolder);

			faceHolder.pos = temp_vertices[vertexIndex[1] - 1];
			faceHolder.uv = temp_uvs[uvIndex[1] - 1];
			faceHolder.norm = temp_normals[normalIndex[1] - 1];
			modelFaces.push_back(faceHolder);

			faceHolder.pos = temp_vertices[vertexIndex[2] - 1];
			faceHolder.uv = temp_uvs[uvIndex[2] - 1];
			faceHolder.norm = temp_normals[normalIndex[2] - 1];
			modelFaces.push_back(faceHolder);
		}
		//Material names
		else if (strcmp(lineHeader, "usemtl") == 0) {
			fscanf_s(file, "%s\n", &currMaterialName, _countof(currMaterialName));
			materialCount++;
			if (materialCount != 1) {
				materialFaceCount.push_back(currMatNameCount);
				modelMaterialFaceVerts.push_back(modelFaces);
			}
			currMatNameCount.materialName = currMaterialName;
			currMatNameCount.faceCount = 0;
			//modelFaces.empty();
			modelFaces.clear();
		}
	}

	//Push the last material name and face count
	materialFaceCount.push_back(currMatNameCount);
	modelMaterialFaceVerts.push_back(modelFaces);

	//Calculate model vectors for modelMaterialFaceVerts
	calculateModelVectors();

	//For each vertex of each triangle
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		DirectX::XMFLOAT3 vertex = temp_vertices[vertexIndex - 1];
		verts.push_back(vertex);
	}
	for (unsigned int i = 0; i < uvIndices.size(); i++) {
		unsigned int uvIndex = uvIndices[i];
		DirectX::XMFLOAT2 uv = temp_uvs[uvIndex - 1];
		uvs.push_back(uv);
	}
	for (unsigned int i = 0; i < normalIndicies.size(); i++) {
		unsigned int normalIndex = normalIndicies[i];
		DirectX::XMFLOAT3 normal = temp_normals[normalIndex - 1];
		normals.push_back(normal);
	}



	//Connect to Device Context
	unsigned int faceCountOffset = 0;
	GraphicsTwoProject::VertexUVNormTanBi* tempOakVertices;
	for (unsigned int i = 0; i < materialCount; i++) {
		tempOakVertices = new GraphicsTwoProject::VertexUVNormTanBi[materialFaceCount[i].faceCount];
		for (unsigned int j = 0; j < materialFaceCount[i].faceCount; j++) {
			tempOakVertices[j].pos = verts[j + faceCountOffset];
			tempOakVertices[j].uv = uvs[j + faceCountOffset];
			tempOakVertices[j].norm = normals[j + faceCountOffset];
			faceCountOffset++;
		}
		modelVertices.push_back(tempOakVertices);
	}

	std::vector<D3D11_SUBRESOURCE_DATA> vertexBufferData;
	D3D11_SUBRESOURCE_DATA tempVertBufferData;
	for (unsigned int i = 0; i < materialCount; i++) {
		//tempVertBufferData.pSysMem = modelVertices[i];
		//Attempting the faces method
		tempVertBufferData.pSysMem = modelMaterialFaceVerts[i].data();
		tempVertBufferData.SysMemPitch = 0;
		tempVertBufferData.SysMemSlicePitch = 0;
		vertexBufferData.push_back(tempVertBufferData);
	}

	std::vector<CD3D11_BUFFER_DESC> vertexBufferDesc;
	Microsoft::WRL::ComPtr<ID3D11Buffer> tempVertBuffer;
	for (unsigned int i = 0; i < materialCount; i++) {
		//CD3D11_BUFFER_DESC tempVertexBufferDesc(sizeof(GraphicsIIProject::VertexUVNormal)*(materialFaceCount[i].faceCount), D3D11_BIND_VERTEX_BUFFER);
		CD3D11_BUFFER_DESC tempVertexBufferDesc(sizeof(GraphicsTwoProject::VertexUVNormTanBi)*(modelMaterialFaceVerts[i].size()), D3D11_BIND_VERTEX_BUFFER);
		vertexBufferDesc.push_back(tempVertexBufferDesc);
		DX::ThrowIfFailed(
			deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc[i],
				//&tempVertexBufferDesc,
				&vertexBufferData[i],
				&tempVertBuffer
			)
		);
		vertexBuffers.push_back(tempVertBuffer);
	}

	// Load mesh indices.
	indices.resize(materialCount);
	for (unsigned int i = 0; i < materialCount; i++) {
		//indices[i].resize(materialFaceCount[i].faceCount);
		indices[i].resize(modelMaterialFaceVerts[i].size());
		//for (unsigned int j = 0; j < materialFaceCount[i].faceCount; j++) {
		for (unsigned int j = 0; j < modelMaterialFaceVerts[i].size(); j++) {
			indices[i][j] = j;
		}
	}

	indexCount = verts.size();
	indexBuffers.resize(materialCount);
	for (unsigned int i = 0; i < materialCount; i++) {
		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = indices[i].data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;

		//CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned short)*materialFaceCount[i].faceCount, D3D11_BIND_INDEX_BUFFER);
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned short)*modelMaterialFaceVerts[i].size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&indexBuffers[i]
			)
		);
	}

	modelMatrix = DirectX::XMFLOAT4X4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	//Load Textures

	return true;
}
