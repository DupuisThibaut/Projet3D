#version 430 core

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_modelspace;
layout(location = 5) in ivec4 boneIDs;
layout(location = 6) in vec4 weights;

layout(std430, binding = 3) readonly buffer BonesBuffer {
    mat4 boneMatrices[];
};

uniform int isAnimated;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;
out vec2 ecran;

void main(){
    vec4 totalPosition;
    vec3 totalNormal;

    if (isAnimated == 1) {
        vec4 skinnedPosition = vec4(0.0);
        vec3 skinnedNormal   = vec3(0.0);
        float sumW = 0.0;

        for(int i = 0; i < 4; i++) {
            int id = boneIDs[i];
            float w = weights[i];
            sumW += w;
            if(id < 0 || w <= 0.0) continue;

            mat4 B = boneMatrices[id];
            skinnedPosition += (B * vec4(vertexPosition_modelspace,1.0)) * w;
            skinnedNormal   += (mat3(B) * vertexNormal_modelspace)       * w;
        }

        if (sumW > 0.0) {
            totalPosition = skinnedPosition;
            totalNormal   = normalize(skinnedNormal);
        } else {
            totalPosition = vec4(vertexPosition_modelspace,1.0); // fallback T-pose
            totalNormal   = vertexNormal_modelspace;
        }
    } else {
        totalPosition = vec4(vertexPosition_modelspace,1.0);
        totalNormal   = vertexNormal_modelspace;
    }

    FragPos   = vec3(model * totalPosition);
    Normal    = mat3(transpose(inverse(model))) * totalNormal;
    TexCoords = vertexUV;
    gl_Position = projection * view * model * totalPosition;
    ecran = gl_Position.xy / gl_Position.w * 0.5 + 0.5;
}
