#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

using namespace godot;

class Ellipsoid : public Node3D {
    GDCLASS(Ellipsoid, Node3D);

protected:
    static void _bind_methods();

public:
    Ellipsoid();
    ~Ellipsoid();

    void _ready() override;

    void initialize(const Vector3 &radii);
    real_t get_minimum_radius() const;
    real_t get_maximum_radius() const;
    Vector3 geodetic_to_3d(real_t latitude_deg, real_t longitude_deg, real_t altitude) const;
    Vector3 scale_to_geodetic_surface(const Vector3 &p) const;
    Array intersections(const Vector3 &origin, const Vector3 &direction) const;
    Vector3 centric_surface_normal(const Vector3 &position) const;

    // Factory static methods to expose common ellipsoid vectors to GDScript
    static Vector3 create_wgs84();
    static Vector3 create_scaled_wgs84();
    static Vector3 create_unit_sphere();

private:
    Vector3 _radii;
    Vector3 _radii_squared;
    Vector3 _radii_to_the_fourth;
    Vector3 _one_over_radii_squared;

    static const Vector3 WGS84;
    static const Vector3 SCALED_WGS84;
    static const Vector3 UNIT_SPHERE;
};
