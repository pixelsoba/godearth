#include "Ellipsoid.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <cmath>

using namespace godot;

const Vector3 Ellipsoid::WGS84 = Vector3(6378137.0, 6378137.0, 6356752.314245);
const Vector3 Ellipsoid::SCALED_WGS84 = Vector3(1.0, 1.0, 6356752.314245 / 6378137.0);
const Vector3 Ellipsoid::UNIT_SPHERE = Vector3(1.0, 1.0, 1.0);

Ellipsoid::Ellipsoid() {
}

Ellipsoid::~Ellipsoid() {
}

void Ellipsoid::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_axis", "axis"), &Ellipsoid::set_axis);
    ClassDB::bind_method(D_METHOD("get_axis"), &Ellipsoid::get_axis);
    ClassDB::bind_method(D_METHOD("get_minimum_radius"), &Ellipsoid::get_minimum_radius);
    ClassDB::bind_method(D_METHOD("get_maximum_radius"), &Ellipsoid::get_maximum_radius);
    ClassDB::bind_method(D_METHOD("geodetic_to_3d", "latitude_deg", "longitude_deg", "altitude"), &Ellipsoid::geodetic_to_3d);
    ClassDB::bind_method(D_METHOD("scale_to_geodetic_surface", "p"), &Ellipsoid::scale_to_geodetic_surface);
    ClassDB::bind_method(D_METHOD("intersections", "origin", "direction"), &Ellipsoid::intersections);
    ClassDB::bind_method(D_METHOD("centric_surface_normal", "position"), &Ellipsoid::centric_surface_normal);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "axis", PROPERTY_HINT_NONE, "Ellipsoid axes (meters): X=equatorial, Y=equatorial, Z=pole. Set before adding to scene to avoid default overwrite."), "set_axis", "get_axis");

    // Static factories for common constants
    ClassDB::bind_static_method("Ellipsoid", D_METHOD("create_wgs84"), &Ellipsoid::create_wgs84);
    ClassDB::bind_static_method("Ellipsoid", D_METHOD("create_scaled_wgs84"), &Ellipsoid::create_scaled_wgs84);
    ClassDB::bind_static_method("Ellipsoid", D_METHOD("create_unit_sphere"), &Ellipsoid::create_unit_sphere);
}

void Ellipsoid::_ready() {
    // Only initialize default if axis not set (avoid overwriting set_axis calls done before adding to scene)
    if (_axis == Vector3(0,0,0)) {
        set_axis(SCALED_WGS84);
    }
}

void Ellipsoid::set_axis(const Vector3 &axis) {
    if (axis.x <= 0.0 || axis.y <= 0.0 || axis.z <= 0.0) {
        UtilityFunctions::push_error("Invalid axis values");
        return;
    }
    _axis = axis;
    _axis_squared = Vector3(axis.x * axis.x, axis.y * axis.y, axis.z * axis.z);
    _axis_to_the_fourth = Vector3(_axis_squared.x * _axis_squared.x, _axis_squared.y * _axis_squared.y, _axis_squared.z * _axis_squared.z);
    _one_over_axis_squared = Vector3(1.0 / _axis_squared.x, 1.0 / _axis_squared.y, 1.0 / _axis_squared.z);
}

Vector3 Ellipsoid::get_axis() const {
    return _axis;
}

// Update methods to use _axis instead of _radii
real_t Ellipsoid::get_minimum_radius() const {
    return MIN(_axis.x, MIN(_axis.y, _axis.z));
}

real_t Ellipsoid::get_maximum_radius() const {
    return MAX(_axis.x, MAX(_axis.y, _axis.z));
}

Vector3 Ellipsoid::geodetic_to_3d(real_t latitude_deg, real_t longitude_deg, real_t altitude) const {
    real_t rad_lat = Math::deg_to_rad(latitude_deg);
    real_t rad_lon = Math::deg_to_rad(longitude_deg) - Math_PI/2.0;

    real_t a = _axis.x;
    real_t b = _axis.z;
    real_t e2 = 1.0 - (b * b) / (a * a);

    real_t N = a / std::sqrt(1.0 - e2 * std::sin(rad_lat) * std::sin(rad_lat));

    Vector3 cartesian_coord(
        (N + altitude) * std::cos(rad_lat) * std::cos(rad_lon),
        ((N * (1.0 - e2)) + altitude) * std::sin(rad_lat),
        (N + altitude) * std::cos(rad_lat) * std::sin(rad_lon)
    );
    return cartesian_coord;
}

Vector3 Ellipsoid::scale_to_geodetic_surface(const Vector3 &p) const {
    real_t beta = 1.0 / std::sqrt(
        (p.x * p.x) * _one_over_axis_squared.x +
        (p.y * p.y) * _one_over_axis_squared.y +
        (p.z * p.z) * _one_over_axis_squared.z);
    Vector3 n(beta * p.x * _one_over_axis_squared.x,
              beta * p.y * _one_over_axis_squared.y,
              beta * p.z * _one_over_axis_squared.z);
    real_t nlen = n.length();
    real_t alpha = (1.0 - beta) * (p.length() / nlen);
    real_t x2 = p.x * p.x;
    real_t y2 = p.y * p.y;
    real_t z2 = p.z * p.z;

    real_t da = 0.0;
    real_t db = 0.0;
    real_t dc = 0.0;
    real_t s = 0.0;
    real_t dSda = 1.0;
    real_t dSdb = 1.0;
    real_t dSdc = 1.0;

    while (std::abs(s) > 1e-10) {
        alpha -= s / dSda;
        da = 1.0 + (alpha * _one_over_axis_squared.x);
        db = 1.0 + (alpha * _one_over_axis_squared.y);
        dc = 1.0 + (alpha * _one_over_axis_squared.z);

        real_t da2 = da * da;
        real_t db2 = db * db;
        real_t dc2 = dc * dc;
        real_t da3 = da * da2;
        real_t db3 = db * db2;
        real_t dc3 = dc * dc2;

        s = x2 / (_axis_squared.x * da2) +
            y2 / (_axis_squared.y * db2) +
            z2 / (_axis_squared.z * dc2) - 1.0;

        dSda = -2.0 * (x2 / (_axis_to_the_fourth.x * da3) +
                       y2 / (_axis_to_the_fourth.y * db3) +
                       z2 / (_axis_to_the_fourth.z * dc3));
    }
    return Vector3(p.x / da, p.y / db, p.z / dc);
}

Vector3 Ellipsoid::centric_surface_normal(const Vector3 &position) const {
    return position.normalized();
}

Array Ellipsoid::intersections(const Vector3 &origin, const Vector3 &direction) const {
    Vector3 dir = direction.normalized();
    real_t a = dir.x * dir.x * _one_over_axis_squared.x + dir.y * dir.y * _one_over_axis_squared.y + dir.z * dir.z * _one_over_axis_squared.z;
    real_t b = 2.0 * (origin.x * dir.x * _one_over_axis_squared.x + origin.y * dir.y * _one_over_axis_squared.y + origin.z * dir.z * _one_over_axis_squared.z);
    real_t c = origin.x * origin.x * _one_over_axis_squared.x + origin.y * origin.y * _one_over_axis_squared.y + origin.z * origin.z * _one_over_axis_squared.z - 1.0;

    real_t discriminant = b * b - 4 * a * c;
    Array res;
    if (discriminant < 0.0) {
        return res;
    } else if (discriminant == 0.0) {
        res.append(-0.5 * b / a);
        return res;
    }

    real_t t = -0.5 * (b + ((b > 0.0) ? 1.0 : -1.0) * std::sqrt(discriminant));
    real_t root1 = t / a;
    real_t root2 = c / t;

    if (root1 < root2) {
        res.append(root1);
        res.append(root2);
    } else {
        res.append(root2);
        res.append(root1);
    }
    return res;
}

Ellipsoid *Ellipsoid::create_wgs84() {
    Ellipsoid *e = memnew(Ellipsoid);
    e->set_axis(WGS84);
    return e;
}

Ellipsoid *Ellipsoid::create_scaled_wgs84() {
    Ellipsoid *e = memnew(Ellipsoid);
    e->set_axis(SCALED_WGS84);
    return e;
}

Ellipsoid *Ellipsoid::create_unit_sphere() {
    Ellipsoid *e = memnew(Ellipsoid);
    e->set_axis(UNIT_SPHERE);
    return e;
}
