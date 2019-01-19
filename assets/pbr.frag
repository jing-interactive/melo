//
// This fragment shader defines a reference implementation for Physically Based Shading of
// a microfacet surface material defined by a glTF model.
//
// References:
// [1] Real Shading in Unreal Engine 4
//     http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [2] Physically Based Shading at Disney
//     http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [3] README.md - Environment Maps
//     https://github.com/KhronosGroup/glTF-WebGL-PBR/#environment-maps
// [4] "An Inexpensive BRDF Model for Physically based Rendering" by Christophe Schlick
//     https://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
// #extension GL_EXT_shader_texture_lod: enable
// #extension GL_OES_standard_derivatives : enable

#include "common.glsl"

uniform vec3 u_LightDirection/* = vec3(1.0, 1.0, 1.0)*/;
uniform vec3 u_LightColor/* = vec3(1.0, 1.0, 1.0)*/;

#ifdef HAS_IBL
    uniform samplerCube u_DiffuseEnvSampler;
    uniform samplerCube u_SpecularEnvSampler;
    uniform sampler2D u_brdfLUT;
#endif

#ifdef PBR_SPECCULAR_GLOSSINESS_WORKFLOW
    #ifdef HAS_DIFFUSEMAP
        uniform sampler2D u_DiffuseSampler;
    #endif

    #ifdef HAS_SPECGLOSSINESSMAP
        uniform sampler2D u_SpecularGlossinessSampler;
    #endif
    uniform vec4 u_SpecularGlossinessValues/* = vec4(1.0, 1.0, 1.0, 1.0)*/;
    uniform vec4 u_DiffuseFactor/* = vec4(1.0, 1.0, 1.0, 1.0)*/;
#else
    #ifdef HAS_BASECOLORMAP
        uniform sampler2D u_BaseColorSampler;
    #endif
    #ifdef HAS_METALROUGHNESSMAP
        uniform sampler2D u_MetallicRoughnessSampler;
    #endif
    uniform vec2 u_MetallicRoughnessValues/* = vec2(1.0, 1.0)*/;
    uniform vec4 u_BaseColorFactor/* = vec4(1.0, 1.0, 1.0, 1.0)*/;
#endif

#ifdef HAS_NORMALMAP
    uniform sampler2D u_NormalSampler;
    uniform float u_NormalScale        /* = 1.0*/;
#endif

#ifdef HAS_EMISSIVEMAP
    uniform sampler2D u_EmissiveSampler;
    uniform vec3 u_EmissiveFactor      /* = vec3(1.0, 1.0, 1.0)*/;
#endif

#ifdef HAS_OCCLUSIONMAP
    uniform sampler2D u_OcclusionSampler;
    uniform float u_OcclusionStrength  /* = 1.0*/;
#endif

uniform vec3 u_Camera/* = vec3(1.0, 1.0, 1.0)*/;

in vec3 v_Position;
in vec2 v_UV;
#ifdef HAS_NORMALS
    #ifdef HAS_TANGENTS
        in mat3 v_TBN;
    #else
        in vec3 v_Normal;
    #endif
#endif

out vec4    oColor;

// Encapsulate the various inputs used by the various functions in the shading equation
// We store values in this struct to simplify the integration of alternative implementations
// of the shading terms, outlined in the Readme.MD Appendix.
struct PBRInfo
{
    float NdotL;                  // cos angle between normal and light direction
    float NdotV;                  // cos angle between normal and view direction
    float NdotH;                  // cos angle between normal and half vector
    float LdotH;                  // cos angle between light direction and half vector
    float VdotH;                  // cos angle between view direction and half vector
    float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
    float metalness;              // metallic value at the surface
    vec3 reflectance0;            // full reflectance color (normal incidence angle)
    vec3 reflectance90;           // reflectance color at grazing angle
    float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
    vec3 diffuseColor;            // color contribution from diffuse lighting
    vec3 specularColor;           // color contribution from specular lighting
};

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getNormal()
{
    // Retrieve the tangent space matrix
#ifndef HAS_TANGENTS
    vec3 pos_dx = dFdx(v_Position);
    vec3 pos_dy = dFdy(v_Position);
    vec3 tex_dx = dFdx(vec3(v_UV, 0.0));
    vec3 tex_dy = dFdy(vec3(v_UV, 0.0));
    vec3 t = (tex_dy.t * pos_dx - tex_dx.t * pos_dy) / (tex_dx.s * tex_dy.t - tex_dy.s * tex_dx.t);

    #ifdef HAS_NORMALS
        vec3 ng = normalize(v_Normal);
    #else
        vec3 ng = cross(pos_dx, pos_dy);
    #endif

    t = normalize(t - ng * dot(ng, t));
    vec3 b = normalize(cross(ng, t));
    mat3 tbn = mat3(t, b, ng);
#else // HAS_TANGENTS
    mat3 tbn = v_TBN;
#endif

#ifdef HAS_NORMALMAP
    vec3 n = texture(u_NormalSampler, v_UV).rgb;
    n = normalize(tbn * ((2.0 * n - 1.0) * vec3(u_NormalScale, u_NormalScale, 1.0)));
#else
    // The tbn matrix is linearly interpolated, so we need to re-normalize
    vec3 n = normalize(tbn[2].xyz);
#endif

    return n;
}

#ifdef HAS_IBL
    // Calculation of the lighting contribution from an optional Image Based Light source.
    // Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
    // See our README.md on Environment Maps [3] for additional discussion.
    vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection)
    {
        float mipCount = 9.0; // resolution of 512x512
        // retrieve a scale and bias to F0. See [1], Figure 3
        vec3 brdf = SRGBtoLINEAR(texture(u_brdfLUT, vec2(pbrInputs.NdotV, 1.0 - pbrInputs.perceptualRoughness))).rgb;
        vec3 diffuseLight = SRGBtoLINEAR(texture(u_DiffuseEnvSampler, n)).rgb;

    #ifdef HAS_TEX_LOD
        float lod = (pbrInputs.perceptualRoughness * mipCount);
        vec3 specularLight = SRGBtoLINEAR(textureLod(u_SpecularEnvSampler, reflection, lod)).rgb;
    #else
        vec3 specularLight = SRGBtoLINEAR(texture(u_SpecularEnvSampler, reflection)).rgb;
    #endif

        vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;
        vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);

        return diffuse + specular;
    }
#endif

// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
vec3 diffuse(PBRInfo pbrInputs)
{
    return pbrInputs.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbrInputs)
{
    return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs)
{
    float NdotL = pbrInputs.NdotL;
    float NdotV = pbrInputs.NdotV;
    float r = pbrInputs.alphaRoughness;

    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs)
{
    float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
    float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
    return roughnessSq / (M_PI * f * f);
}

void main()
{
    float perceptualRoughness;
    float metallic;
    vec3 diffuseColor;
    vec4 baseColor;

#ifdef PBR_SPECCULAR_GLOSSINESS_WORKFLOW
    // Values from specular glossiness workflow are converted to metallic roughness
    #ifdef HAS_SPECGLOSSINESSMAP
        vec3 specular = SRGBtoLINEAR(texture(u_SpecularGlossinessSampler, v_UV)).rgb;
        perceptualRoughness = 1.0 - texture(u_SpecularGlossinessSampler, v_UV).a;
    #else
        vec3 specular = u_SpecularGlossinessValues.rgb;
        perceptualRoughness = 1.0 - u_SpecularGlossinessValues.a;
    #endif

    const float epsilon = 1e-6;

    #ifdef HAS_DIFFUSEMAP
        vec4 baseDiff = SRGBtoLINEAR(texture(u_DiffuseSampler, v_UV)) * u_DiffuseFactor;
    #else
        vec4 baseDiff = u_DiffuseFactor;
    #endif

    float maxSpecular = max(max(specular.r, specular.g), specular.b);

    // Convert metallic value from specular glossiness inputs
    metallic = convertMetallic(baseDiff.rgb, specular, maxSpecular);

    vec3 baseColorDiffusePart = baseDiff.rgb * ((1.0 - maxSpecular) / (1 - c_MinRoughness) / max(1 - metallic, epsilon)) * u_DiffuseFactor.rgb;
    vec3 baseColorSpecularPart = specular - (vec3(c_MinRoughness) * (1 - metallic) * (1 / max(metallic, epsilon))) * u_SpecularGlossinessValues.rgb;
    baseColor = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), baseDiff.a);
#else
    #ifdef HAS_METALROUGHNESSMAP
        // Metallic and Roughness material properties are packed together
        // In glTF, these factors can be specified by fixed scalar values
        // or from a metallic-roughness map
        perceptualRoughness = u_MetallicRoughnessValues.y;
        metallic = u_MetallicRoughnessValues.x;    
        // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
        // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
        vec4 mrSample = texture(u_MetallicRoughnessSampler, v_UV);
        perceptualRoughness = mrSample.g * perceptualRoughness;
        metallic = mrSample.b * metallic;
    #endif
        perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
        metallic = clamp(metallic, 0.0, 1.0);

        // The albedo may be defined from a base texture or a flat color
    #ifdef HAS_BASECOLORMAP
        baseColor = SRGBtoLINEAR(texture(u_BaseColorSampler, v_UV)) * u_BaseColorFactor;
    #else
        baseColor = u_BaseColorFactor;
    #endif
#endif
    vec3 f0 = vec3(0.04);
    diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
    diffuseColor *= 1.0 - metallic;

    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness [2].
    float alphaRoughness = perceptualRoughness * perceptualRoughness;
        
    vec3 specularColor = mix(f0, baseColor.rgb, metallic);

    // Compute reflectance.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    // For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
    // For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 specularEnvironmentR0 = specularColor.rgb;
    vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

    vec3 n = getNormal();                             // normal at surface point
    vec3 v = normalize(u_Camera - v_Position);        // Vector from surface point to camera
    vec3 l = normalize(u_LightDirection);             // Vector from surface point to light
    vec3 h = normalize(l+v);                          // Half vector between both l and v
    vec3 reflection = -normalize(reflect(v, n));

    float NdotL = clamp(dot(n, l), 0.001, 1.0);
    float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

    PBRInfo pbrInputs = PBRInfo(
        NdotL,
        NdotV,
        NdotH,
        LdotH,
        VdotH,
        perceptualRoughness,
        metallic,
        specularEnvironmentR0,
        specularEnvironmentR90,
        alphaRoughness,
        diffuseColor,
        specularColor
    );

    // Calculate the shading terms for the microfacet specular shading model
    vec3 F = specularReflection(pbrInputs);
    float G = geometricOcclusion(pbrInputs);
    float D = microfacetDistribution(pbrInputs);

    // Calculation of analytical lighting contribution
    vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
    vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
    vec3 color = NdotL * u_LightColor * (diffuseContrib + specContrib);

    // Calculate lighting contribution from image based lighting source (IBL)
#ifdef HAS_IBL
    color += getIBLContribution(pbrInputs, n, reflection);
#endif

    // Apply optional PBR terms for additional (optional) shading
#ifdef HAS_OCCLUSIONMAP
    float ao = texture(u_OcclusionSampler, v_UV).r;
    color = mix(color, color * ao, u_OcclusionStrength);
#endif

#ifdef HAS_EMISSIVEMAP
    vec3 emissive = SRGBtoLINEAR(texture(u_EmissiveSampler, v_UV)).rgb * u_EmissiveFactor;
    color += emissive;
#endif

    // oColor = vec4(ao, v_UV,1);
    oColor = vec4(pow(color,vec3(1.0/2.2)), baseColor.a);
}
