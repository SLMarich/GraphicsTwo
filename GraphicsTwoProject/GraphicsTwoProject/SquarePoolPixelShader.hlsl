texture2D diffuseTexture : register(t0);
texture2D normalTexture : register(t1);
//Cube texture
TextureCube boxTexture : register (t2);
texture2D diffuseTexture2 : register(t3);
texture2D normalTexture2 : register(t4);



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
//float3 directionalLightDirection = float3(0.5f, 0.5f, 0.5f);

float4 textureColor = diffuseTexture.Sample(sampleFilter, input.uv);

if (textureColor.w < 0.5f) {
	discard;
}
float3 mappedNorm = normalTexture.Sample(sampleFilter, input.uv);
mappedNorm = (mappedNorm*2.0f) - 1.0f;
mappedNorm = (mappedNorm.x*input.tangent) + (mappedNorm.y*input.binormal) + (mappedNorm.z*input.norm);
mappedNorm = normalize(mappedNorm);

float3 upVector = float3(0.0f, 1.0f, 0.0f);
float secondTextureAlpha = mappedNorm.y;
if (input.worldPos.y > -2.0f) {
	secondTextureAlpha = 0.0f;
}
float4 secondTextureColor = diffuseTexture2.Sample(sampleFilter, input.uv);
textureColor = lerp(textureColor, secondTextureColor, secondTextureAlpha);

float3 mappedNorm2 = normalTexture2.Sample(sampleFilter, input.uv);
mappedNorm2 = (mappedNorm2*2.0f) - 1.0f;
mappedNorm2 = (mappedNorm2.x*input.tangent) + (mappedNorm2.y*input.binormal) + (mappedNorm2.z*input.norm);
mappedNorm2 = normalize(mappedNorm2);
mappedNorm = normalize(lerp(mappedNorm, mappedNorm2, secondTextureAlpha));
//mappedNorm = normalize(lerp(mappedNorm, mappedNorm2, 0.5f));
//mappedNorm = normalize(mappedNorm + mappedNorm2);

//Invert light direction for calculations
float3 invspotlightDirection = -spotlightDirection;

//Point light calculation
//Determine direction
float3 pointLightTextureDirection = normalize(pointLightPosition.xyz - input.worldPos.xyz);
float3 pointLightDirection = normalize(pointLightPosition.xyz - input.worldPos.xyz);
//Invert light direction
pointLightTextureDirection = mul(-pointLightTextureDirection,pointRotationMatrix);
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
float4 directionalResult = directionalLightRatio * float4(0.5f, 0.5f, 0.3f,1.0f);
//Give directional specular
float directionalLightIntensity = dot(directionalLightDirection, -mappedNorm);
directionalLightIntensity = saturate(directionalLightIntensity);
//Add specular
reflection = normalize(2.0f * directionalLightIntensity*(-mappedNorm) - directionalLightDirection);
specular = pow(saturate(dot(reflection, specularDirection)), specularPower);
//Add the specular component last to the output color
directionalResult = directionalResult + specular;

//Spot Lighting
//Get the spot light's noramlized direction
float3 spotlightNormDirection = normalize(spotlightPosition - input.worldPos.xyz);
//Get surface ratio
float surfaceRatio = saturate(dot(-spotlightNormDirection, invspotlightDirection));
//Adding normal texture
float spotlightRatio = saturate(dot(invspotlightDirection, -mappedNorm));
//Get inner and outter cones
float innerCone = cos(1.0f * (3.14515f / 180.0f));
float outterCone = cos(24.0f * (3.14515f / 180.0f));
//Attenuate
float spotlightAttenuation = 1.0f - saturate((innerCone - surfaceRatio) / (innerCone - outterCone));
//Get result
float4 spotlightResult = spotlightRatio * spotlightColor * spotlightAttenuation;
//spotLight specular
float spotlightIntensity = dot(spotlightDirection, -mappedNorm);
spotlightIntensity = saturate(spotlightIntensity);
reflection = normalize(2.0f * spotlightIntensity*mappedNorm - spotlightNormDirection);
specular = pow(saturate(dot(reflection, specularDirection)), specularPower);
specular = specular*spotlightIntensity;
spotlightResult += specular;


float4 sampled = textureColor * saturate(pointLightResult + ambientTerm + directionalResult + spotlightResult);
//sampled.a = clamp(sampled.a - 0.05, 0, 1);
sampled.r = sampled.r*sampled.a;
sampled.g = sampled.g*sampled.a;
sampled.b = sampled.b*sampled.a;

//clip(sampled.a - 0.02f);
//sampled.a = 1.0f;

return float4(sampled.xyz, textureColor.w);
}
