#include "common.glsl"

uniform sampler2D u_BaseColorSampler;
uniform vec4 u_BaseColorFactor = vec4(1.0, 1.0, 1.0, 1.0);
in vec2     v_UV;
out vec4    oColor;

void main()
{
    vec4 baseColor = SRGBtoLINEAR(texture(u_BaseColorSampler, v_UV)) * u_BaseColorFactor;
    oColor = vec4(pow(baseColor.rgb, vec3(1.0/2.2)), baseColor.a);
}
