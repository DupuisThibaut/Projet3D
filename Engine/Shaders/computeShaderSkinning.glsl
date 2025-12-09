#version 450

layout(local_size_x = 256) in;

struct Vertex {
    vec4 position;
    vec4 normal;
};

layout(std430, binding = 12) readonly buffer MeshVertices {
    Vertex verticesIn[];
};

layout(std430, binding = 13) readonly buffer BonesIDs {
    ivec4 boneIDs[];
};

layout(std430, binding = 14) readonly buffer Weights {
    vec4 weights[];
};

layout(std430, binding = 15) readonly buffer BonesBuffer {
    mat4 boneMatrices[];
};

layout(std430, binding = 4) writeonly buffer OutAnimVerts {
    Vertex verticesOut[];
};

struct World{
	mat4 modelMat;
	mat4 invModelMatrix;
	mat3 normalMat;
	vec4 testSphere;
};
layout(std430,binding=8)buffer Worlds{World worlds[];};

uniform mat4 view;
uniform mat4 projection;

void main() {
    uint id = gl_GlobalInvocationID.x;

    // Lire depuis le buffer d'entrée
    vec3 pos = verticesIn[id].position.xyz;
    vec3 nor = verticesIn[id].normal.xyz;

    ivec4 ids = boneIDs[id];
    vec4  ws   = weights[id];

    vec4 skinnedPos = vec4(0.0);
    vec3 skinnedNor = vec3(0.0);
    float sumW = 0.0;

    vec4 totalPosition;
    vec3 totalNormal;

    for(int i = 0; i < 4; i++) {
        int bid = ids[i];
        float ww = ws[i];
        if (bid < 0 || ww <= 0.0) continue;

        mat4 B = boneMatrices[bid];

        skinnedPos += (B * vec4(pos, 1.0)) * ww;
        skinnedNor += (mat3(B) * nor) * ww;
    }
    if (sumW > 0.0) {
        totalPosition = skinnedPos;
        totalNormal   = normalize(skinnedNor);
    } else {
        totalPosition = vec4(pos,1.0); // fallback T-pose
        totalNormal   = vertexNormal_modelspace;
    }

    verticesOut[id].position.xyz = vec3(worlds[0].modelMat*totalPosition);
    verticesOut[id].position.w=pos.w;
    verticesOut[id].normal   = mat3(transpose(worlds[0].invModelMatrix))*totalNormal;
    verticesOut[id].normal.w=nor.w;
}