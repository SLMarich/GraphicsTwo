//Sampler state
SamplerState LinearSampler : register(s0);
//Cube texture
TextureCube boxTexture : register (t0);

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	//return float4(input.color, 1.0f);
	float4 color = boxTexture.Sample(LinearSampler, input.color);
	return float4(color.xyz, 1.0f);
	
	//return boxTexture.Sample(LinearSampler, input.color);
}
