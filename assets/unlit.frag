#include "common.glsl"

#ifdef HAS_BASECOLORMAP
    uniform sampler2D u_BaseColorSampler;
#endif
uniform vec4 u_BaseColorFactor;
in vec2     v_UV;
#ifdef HAS_COLOR
    in vec4 v_Color;
#endif

out vec4    oColor;

void main()
{
#ifdef HAS_BASECOLORMAP
    vec4 baseColor = SRGBtoLINEAR(texture(u_BaseColorSampler, v_UV)) * u_BaseColorFactor;
#else
    vec4 baseColor = v_Color * u_BaseColorFactor;
#endif
    oColor = vec4(pow(baseColor.rgb, vec3(1.0/2.2)), baseColor.a);
}
