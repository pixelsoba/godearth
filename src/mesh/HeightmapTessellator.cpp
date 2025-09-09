#include "HeightmapTessellator.hpp"
#include <math/Ellipsoid.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>

using namespace godot;

Ref<ArrayMesh> HeightmapTessellator::build_mesh(const PackedFloat32Array& heights,
                                                int n,
                                                double lat0, double lon0,
                                                double lat1, double lon1,
                                                Ellipsoid* ellipsoid)
{
    ERR_FAIL_COND_V(ellipsoid == nullptr, Ref<ArrayMesh>());
    ERR_FAIL_COND_V(n < 2, Ref<ArrayMesh>());
    ERR_FAIL_COND_V(heights.size() != n * n, Ref<ArrayMesh>());

    const int vert_count = n * n;

    PackedVector3Array vertices;
    PackedVector2Array uvs;
    vertices.resize(vert_count);
    uvs.resize(vert_count);

    // Génération sommets + UVs [0..1]
    for (int iy = 0; iy < n; ++iy) {
        const double t = (n == 1) ? 0.0 : double(iy) / double(n - 1);
        const double lat = Math::lerp(lat0, lat1, t);
        for (int ix = 0; ix < n; ++ix) {
            const double u = (n == 1) ? 0.0 : double(ix) / double(n - 1);
            const double lon = Math::lerp(lon0, lon1, u);

            const int idx = iy * n + ix;
            const float h = heights[idx];

            const Vector3 pos = ellipsoid->geodetic_to_3d(lat, lon, h);
            vertices.set(idx, pos);
            uvs.set(idx, Vector2((float)u, (float)t));
        }
    }

    // Indices (2 triangles par quad)
    PackedInt32Array indices;
    indices.resize((n - 1) * (n - 1) * 6);
    int ind = 0;
    for (int iy = 0; iy < n - 1; ++iy) {
        for (int ix = 0; ix < n - 1; ++ix) {
            const int v00 = iy * n + ix;
            const int v10 = (iy + 1) * n + ix;
            const int v01 = iy * n + (ix + 1);
            const int v11 = (iy + 1) * n + (ix + 1);
            // tri 1
            indices.set(ind++, v00);
            indices.set(ind++, v10);
            indices.set(ind++, v01);
            // tri 2
            indices.set(ind++, v10);
            indices.set(ind++, v11);
            indices.set(ind++, v01);
        }
    }

    // (Option) Normales lissées – simple accumulate/normalize    
    PackedVector3Array normals;
    normals.resize(vert_count);
    for (int i = 0; i < vert_count; ++i) normals.set(i, Vector3());

    const int tri_count = indices.size() / 3;
    for (int t = 0; t < tri_count; ++t) {
        const int i0 = indices[t*3 + 0];
        const int i1 = indices[t*3 + 1];
        const int i2 = indices[t*3 + 2];
        const Vector3 &p0 = vertices[i0];
        const Vector3 &p1 = vertices[i1];
        const Vector3 &p2 = vertices[i2];
        const Vector3 nrm = (p1 - p0).cross(p2 - p0);
        normals.set(i0, normals[i0] + nrm);
        normals.set(i1, normals[i1] + nrm);
        normals.set(i2, normals[i2] + nrm);
    }
    for (int i = 0; i < vert_count; ++i) {
        Vector3 n = normals[i];
        const real_t len = n.length();
        if (len > (real_t)0.0) n /= len;
        normals.set(i, n);
    }
    

    Ref<ArrayMesh> mesh = Ref<ArrayMesh>(memnew(ArrayMesh));

    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = vertices;
    arrays[Mesh::ARRAY_NORMAL] = normals;
    arrays[Mesh::ARRAY_TEX_UV] = uvs;
    arrays[Mesh::ARRAY_INDEX]  = indices;

    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
    return mesh;
}


void HeightmapTessellator::_bind_methods() {
    // build_mesh is declared static in the header; register as a static method.
    ClassDB::bind_static_method("HeightmapTessellator", D_METHOD("build_mesh", "heights", "n", "lat0", "lon0", "lat1", "lon1", "ellipsoid"), &HeightmapTessellator::build_mesh);
}