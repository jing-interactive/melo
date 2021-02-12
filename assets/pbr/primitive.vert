#include <animation.glsl>

in vec4 ciPosition;
out vec3 v_Position;

#ifdef HAS_NORMALS
in vec3 ciNormal;
#endif

#ifdef HAS_TANGENTS
in vec4 ciTangent;
#endif

#ifdef HAS_NORMALS
#ifdef HAS_TANGENTS
out mat3 v_TBN;
#else
out vec3 v_Normal;
#endif
#endif

#ifdef HAS_UV_SET1
in vec2 ciTexCoord0;
#endif

#ifdef HAS_UV_SET2
in vec2 ciTexCoord1;
#endif

out vec2 v_UVCoord1;
out vec2 v_UVCoord2;

#ifdef HAS_VERTEX_COLOR_VEC3
in vec3 ciColor;
out vec3 v_Color;
#endif

#ifdef HAS_VERTEX_COLOR_VEC4
in vec4 ciColor;
out vec4 v_Color;
#endif

uniform mat4 ciViewProjection;
uniform mat4 ciModelMatrix;
uniform mat4 ciNormalMatrix;

vec4 getPosition()
{
    vec4 pos = vec4(ciPosition.xyz, 1.0);

#ifdef USE_MORPHING
    pos += getTargetPosition();
#endif

#ifdef USE_SKINNING
    pos = getSkinningMatrix() * pos;
#endif

    return pos;
}

#ifdef HAS_NORMALS
vec3 getNormal()
{
    vec3 normal = ciNormal;

#ifdef USE_MORPHING
    normal += getTargetNormal();
#endif

#ifdef USE_SKINNING
    normal = mat3(getSkinningNormalMatrix()) * normal;
#endif

    return normalize(normal);
}
#endif

#ifdef HAS_TANGENTS
vec3 getTangent()
{
    vec3 tangent = ciTangent.xyz;

#ifdef USE_MORPHING
    tangent += getTargetTangent();
#endif

#ifdef USE_SKINNING
    tangent = mat3(getSkinningMatrix()) * tangent;
#endif

    return normalize(tangent);
}
#endif

void main()
{
    vec4 pos = ciModelMatrix * getPosition();
    v_Position = vec3(pos.xyz) / pos.w;

    #ifdef HAS_NORMALS
    #ifdef HAS_TANGENTS
        vec3 tangent = getTangent();
        vec3 normalW = normalize(vec3(ciNormalMatrix * vec4(getNormal(), 0.0)));
        vec3 tangentW = normalize(vec3(ciModelMatrix * vec4(tangent, 0.0)));
        vec3 bitangentW = cross(normalW, tangentW) * ciTangent.w;
        v_TBN = mat3(tangentW, bitangentW, normalW);
    #else // !HAS_TANGENTS
        v_Normal = normalize(vec3(ciNormalMatrix * vec4(getNormal(), 0.0)));
    #endif
    #endif // !HAS_NORMALS

    v_UVCoord1 = vec2(0.0, 0.0);
    v_UVCoord2 = vec2(0.0, 0.0);

    #ifdef HAS_UV_SET1
        v_UVCoord1 = ciTexCoord0;
    #endif

    #ifdef HAS_UV_SET2
        v_UVCoord2 = ciTexCoord1;
    #endif

    #if defined(HAS_VERTEX_COLOR_VEC3) || defined(HAS_VERTEX_COLOR_VEC4)
        v_Color = ciColor;
    #endif

    gl_Position = ciViewProjection * pos;
}
