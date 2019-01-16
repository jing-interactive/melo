in vec4 ciPosition;
#ifdef HAS_NORMALS
    in vec4 ciNormal;
#endif
#ifdef HAS_TANGENTS
    in vec4 ciTangent;
#endif
#ifdef HAS_UV
    in vec2 ciTexCoord0;
#endif

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelMatrix;

out vec3 v_Position; // in world space
out vec2 v_UV;

uniform bool u_flipV;

#ifdef HAS_NORMALS
    #ifdef HAS_TANGENTS
        out mat3 v_TBN;
    #else
        out vec3 v_Normal;
    #endif
#endif


void main()
{
    vec4 pos = ciModelMatrix * ciPosition;
    v_Position = vec3(pos.xyz) / pos.w;

    #ifdef HAS_NORMALS
        #ifdef HAS_TANGENTS
            vec3 normalW = normalize(vec3(ciModelMatrix * vec4(ciNormal.xyz, 0.0)));
            vec3 tangentW = normalize(vec3(ciModelMatrix * vec4(ciTangent.xyz, 0.0)));
            vec3 bitangentW = cross(normalW, tangentW) * ciTangent.w;
            v_TBN = mat3(tangentW, bitangentW, normalW);
        #else // HAS_TANGENTS != 1
            v_Normal = normalize(vec3(ciModelMatrix * vec4(ciNormal.xyz, 0.0)));
        #endif
    #endif

    #ifdef HAS_UV
        v_UV = ciTexCoord0;
        if (u_flipV) {
            v_UV.t = 1.0 - v_UV.t;
        }
    #else
        v_UV = vec2(0.,0.);
    #endif

    gl_Position = ciModelViewProjection * ciPosition; // needs w for proper perspective correction
}
