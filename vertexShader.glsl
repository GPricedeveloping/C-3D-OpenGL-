#version 330 core
layout (location = 0) in vec3 inVertexPosition;
layout (location = 1) in vec3 inVertexNormal;
layout (location = 2) in vec2 inTextureCoordinate;

out vec3 fragmentPosition;
out vec3 fragmentVertexNormal;
out vec2 fragmentTextureCoordinate;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float bendStrength;

void main()
{
    // Apply bending effect
    vec3 modifiedPosition = inVertexPosition;
    modifiedPosition.y += bendStrength * sin(modifiedPosition.x);

    // Transform the modified position
    fragmentPosition = vec3(model * vec4(modifiedPosition, 1.0));
    gl_Position = projection * view * vec4(fragmentPosition, 1.0);

    // ✅ Fix: Use correct normal transformation
    vec3 modifiedNormal = normalize(mat3(transpose(inverse(model))) * inVertexNormal);
    fragmentVertexNormal = modifiedNormal;

    // Pass through other attributes
    fragmentTextureCoordinate = inTextureCoordinate;
}
