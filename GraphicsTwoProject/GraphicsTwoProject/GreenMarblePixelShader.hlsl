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
//float3 directionalLightDirection = float3(spotlightPosition.x*0.5f, -0.5f, spotlightPosition.z*0.5f);
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

//float4 bumpMap = normalTexture.Sample(sampleFilter, input.uv);
////Adjust range of normal from (0, +1) to (-1, +1)
//bumpMap = (bumpMap * 2.0f) - 1.0f;
////Calculate the normal from the data in the bump map.
//float3 bumpNormal = (bumpMap.x * input.tangent) + (bumpMap.y * input.binormal) + (bumpMap.z * input.norm);
////Normalize the resulting normal
//bumpNormal = normalize(bumpNormal);

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
//Get point light result based on the color, attenuation, and intensity
//if (pointLightIntensity > 0.0f) {
//	//Determine the final diffue color based on the diffuse color and the amount of light intensity
//	specularColor = ambientTerm;
//	//Saturate the ambient and diffuse color
//	specularColor = saturate(specularColor);
//	//Calculate the reflection vector based on the light intesnity, normal vector, and light direction
//reflection = normalize(2 * pointLightIntensity*(-input.norm)/*bumpNormal*/ - pointLightDirection);
reflection = normalize(2 * pointLightIntensity*(-mappedNorm)/*bumpNormal*/ - pointLightDirection);
//	//Determine the amount of specular light based on the reflection vector, viewing direction, and specular power
	specular = pow(saturate(dot(reflection, specularDirection)), specularPower);
//	//Use the specular map to determine the intensity of a specular light at this pixel
//}
//	specularColor = specularColor*textureColor;
//	//Add the specular component last to the output color
//	specular = saturate(specularColor + specular);
//float4 pointLightResult = pointLightColor*rangeAttenuation*pointLightIntensity + specular;
//float3 pointLightResult = pointLightColor.xyz*rangeAttenuation*pointLightRatio;
	float4 newPointLightColor = boxTexture.Sample(linearFilter, pointLightTextureDirection);
	float4 pointLightResult = newPointLightColor*rangeAttenuation*pointLightIntensity;// +specular;
	//if (pointLightResult.x >= 0.05f || pointLightResult.y >= 0.05f || pointLightResult.z >= 0.05f) {
	pointLightResult += specular * newPointLightColor;
	//}


//Point Specular light caulcation
//specularDirection = -specularDirection;
//float3 specularHalfVector = normalize(pointLightDirection + specularDirection);
//float specularIntensity = pow(saturate(dot(input.norm, normalize(specularHalfVector))), 64.0f);
//Adding normal texture
//float specularIntensity = pow(saturate(dot(bumpNormal, normalize(specularHalfVector))), 64.0f);
//float4 specularResult = pointLightColor * rangeAttenuation * (specularIntensity * specularIntensity) * 2.0f;
//float3 specularResult = pointLightColor.xyz * rangeAttenuation * (specularIntensity * specularIntensity) * 2.0f;

//Directional Lighting
//Get ratio
//float directionalLightRatio = saturate(dot(float3(0.2f, 0.5f, -0.5f), input.norm.xyz));
//Adding normal texture
//float directionalLightRatio = saturate(dot(float3(-0.2f, -0.5f, 0.5f), bumpNormal));
//	Add movement
	//float directionalLightRatio = saturate(dot(directionalLightDirection, input.norm));
	float directionalLightRatio = saturate(dot(directionalLightDirection, mappedNorm));
	//Add color
float4 directionalResult = directionalLightRatio * float4(0.5f, 0.5f, 0.3f,1.0f);
//Give directional specular
//Sample pixel from specular map
//specularIntensity = specularTexture.Sample(sampleFilter, input.uv);
////Calculate the reflection vector based on the light intensity, normal vector, and light direction
//float directionalLightIntensity = dot(directionalLightDirection, -input.norm);
float directionalLightIntensity = dot(directionalLightDirection, -mappedNorm);
directionalLightIntensity = saturate(directionalLightIntensity);

//Add specular
//NOTE: DISABLED SPECULAR DUE TO AMOUNT
//reflection = normalize(2.0f * directionalLightIntensity*(-input.norm) - directionalLightDirection);
reflection = normalize(2.0f * directionalLightIntensity*(-mappedNorm) - directionalLightDirection);
specular = pow(saturate(dot(reflection, specularDirection)), specularPower);
//Add the specular component last to the output color
directionalResult = directionalResult + specular;

//Spot Lighting
//Get the spot light's noramlized direction
float3 spotlightNormDirection = normalize(spotlightPosition - input.worldPos.xyz);
//Get surface ratio
float surfaceRatio = saturate(dot(-spotlightNormDirection, invspotlightDirection));
//Get spotlight ratio
//float spotlightRatio = saturate(dot(invspotlightDirection, input.norm));
//Adding normal texture
//float spotlightRatio = saturate(dot(invspotlightDirection, -input.norm));
float spotlightRatio = saturate(dot(invspotlightDirection, -mappedNorm));
//Get inner and outter cones
float innerCone = cos(1.0f * (3.14515f / 180.0f));
float outterCone = cos(24.0f * (3.14515f / 180.0f));
//Attenuate
float spotlightAttenuation = 1.0f - saturate((innerCone - surfaceRatio) / (innerCone - outterCone));
//Get result
float4 spotlightResult = spotlightRatio * spotlightColor * spotlightAttenuation;
//spotLight specular
////Sample pixel from specular map
//specularIntensity = specularTexture.Sample(sampleFilter, input.uv);
////Calculate the reflection vector based on the light intensity, normal vector, and light direction
//float spotlightIntensity = dot(spotlightDirection, -input.norm);
float spotlightIntensity = dot(spotlightDirection, -mappedNorm);
spotlightIntensity = saturate(spotlightIntensity);
//reflection = normalize(2.0f * spotlightIntensity*input.norm - spotlightNormDirection);
reflection = normalize(2.0f * spotlightIntensity*mappedNorm - spotlightNormDirection);
////Determine the amount of specular light based on the reflection vector, viewing direction, and specular power
specular = pow(saturate(dot(reflection, specularDirection)), specularPower);
////Use the specular map to determine the intensity of a specular light at this pixel
specular = specular*spotlightIntensity;
////Add the specular component last to the output color
spotlightResult += specular;

//float4 sampled = diffuseTexture.Sample(oakFilter, input.uv) * saturate(pointLightResult + ambientTerm + specularResult + directionalResult + spotlightResult);
//float4 sampled = textureColor * saturate(pointLightResult + ambientTerm + specularResult + directionalResult + spotlightResult);
//Specular now added to each light

//float4 sampled = textureColor * saturate(pointLightResult + ambientTerm);

float4 sampled = textureColor * saturate(pointLightResult + ambientTerm + directionalResult + spotlightResult);
//sampled.a = clamp(sampled.a - 0.05, 0, 1);
sampled.r = sampled.r*sampled.a;
sampled.g = sampled.g*sampled.a;
sampled.b = sampled.b*sampled.a;

//clip(sampled.a - 0.02f);
//sampled.a = 1.0f;

//Toying with depth coloring
//float depth = 1.0f - (input.worldPos.z / input.worldPos.w);
//return float4(depth, depth, depth, 1.0f);

return float4(sampled.xyz, textureColor.w);
}
