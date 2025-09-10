#include "shared_grid.hpp"

#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void SharedGrid::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_or_create_grid", "resolution"), &SharedGrid::get_or_create_grid);
}

Ref<ArrayMesh> SharedGrid::get_or_create_grid(int resolution) {
    if (grid_.is_valid()) return grid_;

    // Grille en [0..1]x[0..1], Z=0 (hauteur ajoutée ailleurs)
    PackedVector3Array verts;
    PackedVector3Array norms;
    PackedVector2Array uvs;
    PackedInt32Array  indices;

    const int N = resolution;
    verts.resize(N * N);
    norms.resize(N * N);
    uvs.resize(N * N);

    int i = 0;
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            const float fx = x / float(N - 1);
            const float fy = y / float(N - 1);
            verts[i] = Vector3(fx, 0.0f, fy);   // (u, 0, v)
            norms[i] = Vector3(0, 1, 0);
            uvs[i]   = Vector2(fx, fy);
            ++i;
        }
    }

    // triangles
    for (int y = 0; y < N - 1; ++y) {
        for (int x = 0; x < N - 1; ++x) {
            const int i0 =  y      * N + x;
            const int i1 =  y      * N + (x + 1);
            const int i2 = (y + 1) * N + x;
            const int i3 = (y + 1) * N + (x + 1);
            // tri 1
            indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
            // tri 2
            indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
        }
    }

    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = verts;
    arrays[Mesh::ARRAY_NORMAL] = norms;
    arrays[Mesh::ARRAY_TEX_UV] = uvs;
    arrays[Mesh::ARRAY_INDEX]  = indices;

    Ref<ArrayMesh> mesh = memnew(ArrayMesh);
    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
    grid_ = mesh;
    return grid_;
}
