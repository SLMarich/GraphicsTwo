// Per-vertex data used as input to the vertex shader.
struct InstanceVertexShaderInput
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD0;
	float3 norm : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float4x4 instanceMatrix : INSTANCE;
};

// Control point data to be passed to the Hull Shader
struct VS_CONTROL_POINT_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 norm : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float4x4 instanceMatrix : INSTANCE;
};

// Simple shader to do vertex processing on the GPU.
VS_CONTROL_POINT_OUTPUT main(InstanceVertexShaderInput input)
{
	VS_CONTROL_POINT_OUTPUT output;
	float4 pos = float4(input.pos, 1.0f);
	output.pos = pos;

	//Send uv coords through
	output.uv = input.uv;

	output.norm = input.norm;
	output.tangent = input.tangent;
	output.binormal = input.binormal;

	output.instanceMatrix = input.instanceMatrix;

	return output;
}
