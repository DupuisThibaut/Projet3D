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

void main() {
    uint id = gl_GlobalInvocationID.x;
    
    if(id == 0) {
        // Write a marker position to see if shader runs
        verticesOut[0].position = vec4(9999.0, 9999.0, 9999.0, 1.0);
    }

    // Lire depuis le buffer d'entrée
    vec3 pos = verticesIn[id].position.xyz;
    vec3 nor = verticesIn[id].normal.xyz;

    ivec4 ids = boneIDs[id];
    vec4  w   = weights[id];

    vec4 skinnedPos = vec4(0.0);
    vec3 skinnedNor = vec3(0.0);

    for(int i = 0; i < 4; i++) {
        int bid = ids[i];
        float ww = w[i];
        if (bid < 0 || ww <= 0.0) continue;

        mat4 B = boneMatrices[bid];

        skinnedPos += (B * vec4(pos, 1.0)) * ww;
        skinnedNor += (mat3(B) * nor) * ww;
    }

    verticesOut[id].position = skinnedPos;
    verticesOut[id].normal   = vec4(normalize(skinnedNor), 0.0);
}