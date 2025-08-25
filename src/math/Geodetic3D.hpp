#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>

#include "math/Ellipsoid.hpp"

using namespace godot;

class Geodetic3D : public Node3D {
    GDCLASS(Geodetic3D, Node3D);

protected:
    static void _bind_methods();

public:
    Geodetic3D();
    ~Geodetic3D();

    void _ready() override;

    void set_latitude(real_t p_lat);
    real_t get_latitude() const;
    void set_longitude(real_t p_lon);
    real_t get_longitude() const;
    void set_altitude(real_t p_alt);
    real_t get_altitude() const;

    void update_3d_position();

private:
    real_t latitude = 0.0;
    real_t longitude = 0.0;
    real_t altitude = 0.0;

    Ellipsoid *ellipsoid = nullptr;

    Ellipsoid *find_ellipsoid_parent(Node *node) const;
};
