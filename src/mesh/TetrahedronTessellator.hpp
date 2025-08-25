#pragma once

#include "AbstractTessellator.hpp"
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <vector>

using namespace godot;

class TetrahedronTessellator : public AbstractTessellator {
    GDCLASS(TetrahedronTessellator, AbstractTessellator);

protected:
    static void _bind_methods();

public:
    TetrahedronTessellator();
    ~TetrahedronTessellator();

    Dictionary create_mesh(int subdivisions, real_t radius) override;

private:
    void subdivide_triangle(const Vector3 &a, const Vector3 &b, const Vector3 &c, int depth, real_t radius, std::vector<Vector3> &vertices, std::vector<int> &indices);
    Vector3 normalize_to_radius(const Vector3 &v, real_t radius);
};
