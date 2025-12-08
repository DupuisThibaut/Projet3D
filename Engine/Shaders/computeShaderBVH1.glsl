#version 450

struct BVH {
    float minx, miny, minz, nb;
    float maxx, maxy, maxz, prof;
    int left, right, start, count;
};

layout(std430, binding = 7) buffer BVHBuffer {
    BVH nodes[];
};

layout(std430, binding = 5) buffer TrianglesBuffer {
    uvec3 tri[];
};

layout(std430, binding = 4) buffer VertexBuffer {
    vec3 vertices[];
};

layout(local_size_x = 128) in;

void main() {
    uint id = gl_GlobalInvocationID.x;

    BVH node = nodes[id];

    // Skip non-leaf nodes
    if (node.left != -1 || node.right != -1)
        return;

    // Skip empty leaves
    if (node.count <= 0)
        return;

    vec3 minV = vec3( 1e30 );
    vec3 maxV = vec3(-1e30 );

    for (int i = 0; i < node.count; i++) {
        uvec3 t = tri[node.start + i];

        vec3 p0 = vertices[t.x];
        vec3 p1 = vertices[t.y];
        vec3 p2 = vertices[t.z];

        minV = min(minV, min(p0, min(p1, p2)));
        maxV = max(maxV, max(p0, max(p1, p2)));
    }

    // Write back
    nodes[id].minx = minV.x;
    nodes[id].miny = minV.y;
    nodes[id].minz = minV.z;

    nodes[id].maxx = maxV.x;
    nodes[id].maxy = maxV.y;
    nodes[id].maxz = maxV.z;
}