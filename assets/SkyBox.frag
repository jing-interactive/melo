#include "common.glsl"

uniform samplerCube uCubeMapTex;
uniform float       uExposure;
uniform float       uGamma;

in vec3     vDirection;

out vec4    oColor;

void main( void )
{
    vec3 color  = pow( texture( uCubeMapTex, vDirection ).rgb, vec3( 2.2f ) );
    
    // apply the tone-mapping
    color       = Uncharted2Tonemap( color * uExposure );
    // white balance
    color       = color * ( 1.0f / Uncharted2Tonemap( vec3( 20.0f ) ) );
    
    // gamma correction
    color       = pow( color, vec3( 1.0f / uGamma ) );
    oColor      = vec4( color, 1.0 );
}