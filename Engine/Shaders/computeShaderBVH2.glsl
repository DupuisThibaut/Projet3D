#version 450

struct BVH {
    float minx, miny, minz, nb;
    float maxx, maxy, maxz, prof;
    int left, right, start, count;
};

layout(std430, binding = 7) buffer BVHBuffer {
    BVH nodes[];
};

uniform float targetDepth;

layout(local_size_x = 128) in;

void main() {
    uint id = gl_GlobalInvocationID.x;
    BVH node = nodes[id];

    // Only nodes at target depth
    if (node.prof != targetDepth)
        return;

    // Skip leaf nodes
    if (node.left == -1 && node.right == -1)
        return;

    // Load children
    BVH leftNode  = nodes[node.left];
    BVH rightNode = nodes[node.right];

    vec3 minV = vec3(
        min(leftNode.minx, rightNode.minx),
        min(leftNode.miny, rightNode.miny),
        min(leftNode.minz, rightNode.minz)
    );

    vec3 maxV = vec3(
        max(leftNode.maxx, rightNode.maxx),
        max(leftNode.maxy, rightNode.maxy),
        max(leftNode.maxz, rightNode.maxz)
    );

    // Write back
    nodes[id].minx = minV.x;
    nodes[id].miny = minV.y;
    nodes[id].minz = minV.z;

    nodes[id].maxx = maxV.x;
    nodes[id].maxy = maxV.y;
    nodes[id].maxz = maxV.z;
}