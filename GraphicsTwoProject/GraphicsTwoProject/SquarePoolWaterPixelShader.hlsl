texture2D diffuseTexture : register(t0);
texture2D normalTexture : register(t1);
//Cube texture
TextureCube boxTexture : register (t2);


SamplerState sampleFilter : register(s0);
SamplerState linearFilter : register(s1);

cbuffer	lightBuffer : register(b0) {
	float4	pointLightColor;
	float4	pointLightPosition;
	float4	ambientTerm;
	float4	cameraPosition;

	float4	spotlightColor;
	float3	spotlightDirection;
	float	spotlightConeRatio;
	float3	spotlightPosition;
	float	lightRange;

	float4x4	pointRotationMatrix;

};

// Per-pixel color data passed through the pixel shader.
struct GroundPixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 norm : NORMAL;
	float4 worldPos : W_POS;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
};

// A pass-through function for the (interpolated) color data.
float4 main(GroundPixelShaderInput input) : SV_TARGET
{
	float4 specularIntensity;
float3 reflection;
float4 specular;
float4 specularColor;
float4 specularDirection = -normalize(cameraPosition - input.worldPos);
float specularPower = 32.0f;
float3 directionalLightDirection = float3(spotlightPosition.x*0.5f, 0.5f, spotlightPosition.z*0.5f);

float4 textureColor = diffuseTexture.Sample(sampleFilter, input.uv);

if (textureColor.w < 0.5f) {
	discard;
}
float3 mappedNorm = normalTexture.Sample(sampleFilter, input.uv);
mappedNorm = (mappedNorm*2.0f) - 1.0f;
//float3 crossed = cross(input.norm, input.tangent);
//mappedNorm = (mappedNorm.x*input.tangent) + (mappedNorm.y*crossed) + (mappedNorm.z*input.norm);
mappedNorm = (mappedNorm.x*input.tangent) + (mappedNorm.y*input.binormal) + (mappedNorm.z*input.norm);
mappedNorm = normalize(mappedNorm);

//Point light calculation
//Determine direction
float3 pointLightTextureDirection = normalize(pointLightPosition.xyz - input.worldPos.xyz);
float3 pointLightDirection = normalize(pointLightPosition.xyz - input.worldPos.xyz);
//Invert light direction
pointLightTextureDirection = mul(-pointLightTextureDirection, pointRotationMatrix);
pointLightDirection = -pointLightDirection;
//Determine distance
float vertex_length = length(pointLightDirection);
//Get magnitude
pointLightDirection /= vertex_length;
//Find range attenuation
float rangeAttenuation = 1.0f - saturate(vertex_length / lightRange);
rangeAttenuation *= rangeAttenuation;
//Get ratio between direction and normal
//Adding normal texture
//float pointLightIntensity = dot(pointLightDirection, -input.norm);
float pointLightIntensity = dot(pointLightDirection, -mappedNorm);
pointLightIntensity = saturate(pointLightIntensity);
reflection = normalize(2 * pointLightIntensity*(-mappedNorm)/*bumpNormal*/ - pointLightDirection);
specular = pow(saturate(dot(reflection, specularDirection)), specularPower);
float4 newPointLightColor = boxTexture.Sample(linearFilter, pointLightTextureDirection);
float4 pointLightResult = newPointLightColor*rangeAttenuation*pointLightIntensity;// +specular;
pointLightResult += specular * newPointLightColor;

float directionalLightRatio = saturate(dot(directionalLightDirection, mappedNorm));
//Add color
float4 directionalResult = directionalLightRatio * float4(0.5f, 0.5f, 0.3f, 1.0f);
//Give directional specular
float directionalLightIntensity = dot(directionalLightDirection, -mappedNorm);
directionalLightIntensity = saturate(directionalLightIntensity);
//Add specular
reflection = normalize(2.0f * directionalLightIntensity*(-mappedNorm) - directionalLightDirection);
specular = pow(saturate(dot(reflection, specularDirection)), specularPower);
//Add the specular component last to the output color
directionalResult = directionalResult + specular;

float4 sampled = textureColor * .75f;// *saturate(ambientTerm + ambientTerm * 5);
									 //sampled.a = clamp(sampled.a - 0.05, 0, 1);
sampled.r = sampled.r*sampled.a;
sampled.g = sampled.g*sampled.a;
sampled.b = sampled.b*sampled.a;

sampled = sampled*saturate(ambientTerm + directionalResult + pointLightResult);

//return float4(sampled.xyz, textureColor.w);
return float4(sampled.xyz, 0.25f);
}
