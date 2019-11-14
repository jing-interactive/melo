uniform mat4    ciViewMatrix;
uniform mat4    ciProjectionMatrix;

in vec4         ciPosition;

out vec3        vDirection;

void main( void )
{
    vDirection  = vec3( ciPosition );
    mat4 rotMat = mat4(mat3(ciViewMatrix));
    gl_Position = ciProjectionMatrix * rotMat * ciPosition;
    gl_Position.w = gl_Position.z;
}
