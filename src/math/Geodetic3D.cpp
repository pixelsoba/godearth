#include "Geodetic3D.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/node.hpp>

using namespace godot;

Geodetic3D::Geodetic3D() {}
Geodetic3D::~Geodetic3D() {}

void Geodetic3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_latitude", "lat"), &Geodetic3D::set_latitude);
    ClassDB::bind_method(D_METHOD("get_latitude"), &Geodetic3D::get_latitude);
    ClassDB::bind_method(D_METHOD("set_longitude", "lon"), &Geodetic3D::set_longitude);
    ClassDB::bind_method(D_METHOD("get_longitude"), &Geodetic3D::get_longitude);
    ClassDB::bind_method(D_METHOD("set_altitude", "alt"), &Geodetic3D::set_altitude);
    ClassDB::bind_method(D_METHOD("get_altitude"), &Geodetic3D::get_altitude);

    ClassDB::bind_method(D_METHOD("update_3d_position"), &Geodetic3D::update_3d_position);

    // Property bindings
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "latitude", PROPERTY_HINT_RANGE, "-89.99,89.99,0.01"), "set_latitude", "get_latitude");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "longitude", PROPERTY_HINT_RANGE, "-180,180,0.01"), "set_longitude", "get_longitude");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "altitude"), "set_altitude", "get_altitude");
}

void Geodetic3D::_ready() {
    ellipsoid = find_ellipsoid_parent(this);
    if (!ellipsoid) {
        UtilityFunctions::push_error("No Ellipsoid found in parent hierarchy");
        return;
    }
    update_3d_position();
}

void Geodetic3D::set_latitude(real_t p_lat) {
    latitude = p_lat;
    update_3d_position();
}
real_t Geodetic3D::get_latitude() const { return latitude; }

void Geodetic3D::set_longitude(real_t p_lon) {
    longitude = p_lon;
    update_3d_position();
}
real_t Geodetic3D::get_longitude() const { return longitude; }

void Geodetic3D::set_altitude(real_t p_alt) {
    altitude = p_alt;
    update_3d_position();
}
real_t Geodetic3D::get_altitude() const { return altitude; }

void Geodetic3D::update_3d_position() {
    if (!ellipsoid) return;
    Vector3 pos = ellipsoid->geodetic_to_3d(latitude, longitude, altitude);
    set_position(pos);
}

Ellipsoid *Geodetic3D::find_ellipsoid_parent(Node *node) const {
    if (!node) return nullptr;
    if (node->is_class(Ellipsoid::get_class_static())) {
        return Object::cast_to<Ellipsoid>(node);
    }
    if (node->get_parent()) return find_ellipsoid_parent(node->get_parent());
    return nullptr;
}
