#include "TetrahedronTessellator.hpp"
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <cmath>

using namespace godot;

TetrahedronTessellator::TetrahedronTessellator() {}
TetrahedronTessellator::~TetrahedronTessellator() {}

void TetrahedronTessellator::_bind_methods() {
    ClassDB::bind_method(D_METHOD("create_mesh", "subdivisions", "radius"), &TetrahedronTessellator::create_mesh);
}

Vector3 TetrahedronTessellator::normalize_to_radius(const Vector3 &v, real_t radius) {
    Vector3 n = v.normalized();
    return n * radius;
}

void TetrahedronTessellator::subdivide_triangle(const Vector3 &a, const Vector3 &b, const Vector3 &c, int depth, real_t radius, std::vector<Vector3> &vertices, std::vector<int> &indices) {
    if (depth == 0) {
        int start = (int)vertices.size();
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
        indices.push_back(start);
        indices.push_back(start + 1);
        indices.push_back(start + 2);
        return;
    }

    Vector3 ab = normalize_to_radius((a + b) * 0.5, radius);
    Vector3 bc = normalize_to_radius((b + c) * 0.5, radius);
    Vector3 ca = normalize_to_radius((c + a) * 0.5, radius);

    subdivide_triangle(a, ab, ca, depth - 1, radius, vertices, indices);
    subdivide_triangle(ab, b, bc, depth - 1, radius, vertices, indices);
    subdivide_triangle(ca, bc, c, depth - 1, radius, vertices, indices);
    subdivide_triangle(ab, bc, ca, depth - 1, radius, vertices, indices);
}

Dictionary TetrahedronTessellator::create_mesh(int subdivisions, real_t radius) {
    // Start with a tetrahedron inscribed in a sphere of given radius
    std::vector<Vector3> vertices;
    std::vector<int> indices;

    Vector3 v0 = Vector3(1, 1, 1);
    Vector3 v1 = Vector3(-1, -1, 1);
    Vector3 v2 = Vector3(-1, 1, -1);
    Vector3 v3 = Vector3(1, -1, -1);

    // Subdivide each face
    subdivide_triangle(normalize_to_radius(v0, radius), normalize_to_radius(v2, radius), normalize_to_radius(v1, radius), subdivisions, radius, vertices, indices);
    subdivide_triangle(normalize_to_radius(v0, radius), normalize_to_radius(v1, radius), normalize_to_radius(v3, radius), subdivisions, radius, vertices, indices);
    subdivide_triangle(normalize_to_radius(v0, radius), normalize_to_radius(v3, radius), normalize_to_radius(v2, radius), subdivisions, radius, vertices, indices);
    subdivide_triangle(normalize_to_radius(v1, radius), normalize_to_radius(v2, radius), normalize_to_radius(v3, radius), subdivisions, radius, vertices, indices);

    PackedVector3Array packed_vertices;
    PackedInt32Array packed_indices;
    packed_vertices.resize((int)vertices.size());
    for (int i = 0; i < (int)vertices.size(); ++i) {
        packed_vertices.set(i, vertices[(size_t)i]);
    }
    packed_indices.resize((int)indices.size());
    for (int i = 0; i < (int)indices.size(); ++i) {
        packed_indices.set(i, indices[(size_t)i]);
    }

    Dictionary res;
    res["vertices"] = packed_vertices;
    res["indices"] = packed_indices;
    return res;
}
