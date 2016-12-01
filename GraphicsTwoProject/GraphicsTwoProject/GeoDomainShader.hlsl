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
	float4 worldPos : W_POS;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
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
	Output.pos *= 5.0f;
	//Project point onto planes, then interpolate
	float dist1 = dot((Output.pos - normalize(patch[0].norm)), normalize(patch[0].norm));
	float dist2 = dot((Output.pos - normalize(patch[1].norm)), normalize(patch[1].norm));
	float dist3 = dot((Output.pos - normalize(patch[2].norm)), normalize(patch[2].norm));
	float4 project1 = Output.pos - (float4)(normalize(patch[0].norm),0.0f*domain.x)*dist1;
	float4 project2 = Output.pos - (float4)(normalize(patch[1].norm),0.0f*domain.y)*dist1;
	float4 project3 = Output.pos - (float4)(normalize(patch[2].norm),0.0f*domain.z)*dist1;
	Output.pos = float4(project1*domain.x + project2*domain.y + project3*domain.z);
	//float3 lerpNorm = lerp(patch[0].norm, patch[1].norm, patch[2].norm);
	//Output.pos = Output.pos + Output.pos;
	//Output.pos = lerp(project1+(float4)(patch[0].norm,0.0f), project2+(float4)(patch[1].norm,0.0f), project3+(float4)(patch[2].norm,0.0f));
	//Output.pos = float4((patch[0].pos+(float4)(patch[0].norm,0.0f))*domain.x + (patch[1].pos+(float4)(patch[0].norm,0.0f))*domain.y + (patch[2].pos+(float4)(patch[0].norm,0.0f))*domain.z);

	Output.uv = patch[0].uv*domain.x + patch[1].uv*domain.y + patch[2].uv*domain.z;
	Output.norm = patch[0].norm*domain.x + patch[1].norm*domain.y + patch[2].norm * domain.z;
	//Output.worldPos = float4(patch[0].pos*domain.x + patch[1].pos*domain.y + patch[2].pos*domain.z);
	Output.worldPos = patch[0].worldPos*domain.x + patch[1].worldPos*domain.y + patch[2].worldPos*domain.z;
	Output.tangent = patch[0].tangent*domain.x + patch[1].tangent*domain.y + patch[2].tangent*domain.z;
	Output.binormal = patch[0].binormal*domain.x + patch[1].binormal*domain.y + patch[2].binormal*domain.z;
	//Output.pos += (float4)(Output.worldPos.x, Output.worldPos.y, Output.worldPos.z, 0.0f);
	//Output.pos += (float4)(1.0f, -2.0f, 1.0f, 0.0f);
	return Output;
}
