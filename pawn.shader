#shader vertex

#version 330 core

in vec3 inPosition;
in vec3 inColor;
in vec3 inNormal;

out vec3 fragColor;
out vec3 fragPos;

uniform mat4 transform;

//uniform mat4 modelViewProjection;

void main() {
    gl_Position = transform * vec4(inPosition, 1.0);
    fragPos = inPosition;
    fragColor = inColor;
}

#shader fragment

#version 330 core

in vec3 fragColor;
in vec3 fragPos;

out vec4 fragOutput;

void main() {
    float gradient = length(fragPos) / length(vec3(1.0));

    fragOutput = vec4(fragColor * (1.0 - gradient), 1.0);
}