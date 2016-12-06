//Sampler state
SamplerState LinearSampler : register(s0);
//Cube texture
TextureCube boxTexture : register (t0);

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 skyPos : SKYPOS;
	float2 uv : TEXCOORD;
	float3 norm : NORMAL;
	float4 worldPos : W_POS;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
};

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 color = boxTexture.Sample(LinearSampler,input.skyPos);
	return float4(color.xyz, 1.0f);
	//return boxTexture.Sample(LinearSampler,input.skyPos);
	//return boxTexture.Sample(LinearSampler,(float3)input.norm);
}
