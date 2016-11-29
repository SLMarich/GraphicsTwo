// Input control point
struct VS_CONTROL_POINT_OUTPUT
{
	float4 vPosition : SV_POSITION; // Control point position
	// TODO: change/add other stuff
	float3 clr : COLOR0; //CP color
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

// Patch Constant Function
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	// Insert code to compute Output here
	Output.EdgeTessFactor[0] =
		Output.EdgeTessFactor[1] =
		Output.EdgeTessFactor[2] =
		//Output.InsideTessFactor = 15; // e.g. could calculate dynamic tessellation factors instead
		Output.InsideTessFactor = 3;
	return Output;
}

[domain("tri")]						//Tesselator builds triangles
[partitioning("fractional_odd")]	//Divison type
[outputtopology("triangle_cw")]		//topology of output
[outputcontrolpoints(3)]			//number of control points sent to Domain Shader
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main( 
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HS_CONTROL_POINT_OUTPUT Output;

	// Insert code to compute Output here
	Output.vPosition = ip[i].vPosition;// +ip[i].vPosition;
	Output.clr = ip[i].clr;

	return Output;
}
