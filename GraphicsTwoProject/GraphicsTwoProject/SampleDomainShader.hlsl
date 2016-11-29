struct DS_OUTPUT
{
	float4 vPosition  : SV_POSITION;
	// TODO: change/add other stuff
	float3 clr : COLOR0; //Color to pixel shader
};

// Output control point
struct HS_CONTROL_POINT_OUTPUT
{
	float4 vPosition : SV_POSITION; //Control point position
	float3 clr : COLOR0; //Control point color
};

// Output patch constant data.
struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor; // e.g. would be [4] for a quad domain
	float InsideTessFactor			: SV_InsideTessFactor; // e.g. would be Inside[2] for a quad domain
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

	Output.vPosition = float4(
		patch[0].vPosition*domain.x+patch[1].vPosition*domain.y+patch[2].vPosition*domain.z);
	Output.clr = float3(patch[0].clr*domain.x + patch[1].clr*domain.y + patch[2].clr*domain.z);
	return Output;
}
