// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

// Per-vertex data used as input to the vertex shader.
struct InstanceVertexShaderInput
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD0;
	float3 norm : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float3 instancePosition : TEXCOORD1;
	float4x4 instanceRotation : ROTATION;
};

// Control point data to be passed to the Hull Shader
struct VS_CONTROL_POINT_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 norm : NORMAL;
	float4 worldPos : W_POS;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
};

// Simple shader to do vertex processing on the GPU.
VS_CONTROL_POINT_OUTPUT main(InstanceVertexShaderInput input)
{
	VS_CONTROL_POINT_OUTPUT output;
	float4 pos = float4(input.pos, 1.0f);
	//Rotate for instance
	pos = mul(pos, input.instanceRotation);
	//Account for instance position
	pos.x += input.instancePosition.x;
	pos.y += input.instancePosition.y;
	pos.z += input.instancePosition.z;

	// Transform the vertex position into projected space.
	pos = mul(pos, model);
	output.worldPos = pos;
	pos = mul(pos, view);
	pos = mul(pos, projection);
	output.pos = pos;

	//Send uv coords through
	output.uv = input.uv;
	//output.norm = input.norm;

	//Calculate normal against world, then normalize the final value
	output.norm = mul(input.norm, (float3x3)model);
	output.norm = normalize(output.norm);

	//Calculate tangent vector against world and then normalize
	output.tangent = mul(input.tangent, (float3x3)model);
	output.tangent = normalize(output.tangent);

	//Calculate binormal vector against world then normalize
	output.binormal = mul(input.binormal, (float3x3)model);
	output.binormal = normalize(output.binormal);

	return output;
}
