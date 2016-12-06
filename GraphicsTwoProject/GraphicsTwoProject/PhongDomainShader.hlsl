cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

struct DS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 norm : NORMAL;
	float4 worldPos : W_POS;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
};

// Output control point
struct HS_CONTROL_POINT_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 norm : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float4x4 instanceMatrix : INSTANCE;
};

// Output patch constant data.
struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor; // e.g. would be [4] for a quad domain
	float InsideTessFactor : SV_InsideTessFactor; // e.g. would be Inside[2] for a quad domain
												  // TODO: change/add other stuff
};

#define NUM_CONTROL_POINTS 3

[domain("tri")]	//Must match Hull Shader domain
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT Output;

	// Insert code to compute Output here
	Output.pos = float4(patch[0].pos*domain.x + patch[1].pos*domain.y + patch[2].pos*domain.z);

	//Project point onto planes, then interpolate
	float dist1 = dot((Output.pos.xyz - normalize(patch[0].norm)), normalize(patch[0].norm));
	float dist2 = dot((Output.pos.xyz - normalize(patch[1].norm)), normalize(patch[1].norm));
	float dist3 = dot((Output.pos.xyz - normalize(patch[2].norm)), normalize(patch[2].norm));
	float3 project1 = Output.pos.xyz - normalize(patch[0].norm)*dist1;
	float3 project2 = Output.pos.xyz - normalize(patch[1].norm)*dist2;
	float3 project3 = Output.pos.xyz - normalize(patch[2].norm)*dist3;
	Output.pos = float4(project1*domain.x + project2*domain.y + project3*domain.z, 1.0f);// Output.pos.w);

																						 //Adjust for instance
	Output.pos = mul(Output.pos, patch[0].instanceMatrix);
	//Translate for instance
	//Output.pos.x += patch[0].instancePosition.x;
	//Output.pos.y += patch[0].instancePosition.y;
	//Output.pos.z += patch[0].instancePosition.z;
	//Move into projected space
	//Output.pos = mul(Output.pos, model);
	Output.worldPos = Output.pos;
	Output.pos = mul(Output.pos, view);
	Output.pos = mul(Output.pos, projection);

	Output.uv = patch[0].uv*domain.x + patch[1].uv*domain.y + patch[2].uv*domain.z;

	//Calculate normal against world, then normalize the final value
	Output.norm = patch[0].norm*domain.x + patch[1].norm*domain.y + patch[2].norm * domain.z;
	Output.norm = mul(Output.norm, (float3x3)model);
	Output.norm = normalize(Output.norm);
	//Calculate tangent ector against world and then normalize
	Output.tangent = patch[0].tangent*domain.x + patch[1].tangent*domain.y + patch[2].tangent*domain.z;
	Output.tangent = mul(Output.tangent, (float3x3)model);
	Output.tangent = normalize(Output.tangent);
	//Calculate the binroal vector against the world and then normalize
	Output.binormal = patch[0].binormal*domain.x + patch[1].binormal*domain.y + patch[2].binormal*domain.z;
	Output.binormal = mul(Output.binormal, (float3x3)model);
	Output.binormal = normalize(Output.binormal);

	//Output.worldPos = patch[0].worldPos*domain.x + patch[1].worldPos*domain.y + patch[2].worldPos*domain.z;


	return Output;
}
