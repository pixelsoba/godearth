#include "camera_params.hpp"
using namespace godot;

void CameraParams::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_position", "position"), &CameraParams::set_position);
    ClassDB::bind_method(D_METHOD("get_position"), &CameraParams::get_position);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "position"), "set_position", "get_position");

    ClassDB::bind_method(D_METHOD("set_fov_y_deg", "fov_y_deg"), &CameraParams::set_fov_y_deg);
    ClassDB::bind_method(D_METHOD("get_fov_y_deg"), &CameraParams::get_fov_y_deg);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fov_y_deg"), "set_fov_y_deg", "get_fov_y_deg");

    ClassDB::bind_method(D_METHOD("set_viewport_height_px", "viewport_height_px"), &CameraParams::set_viewport_height_px);
    ClassDB::bind_method(D_METHOD("get_viewport_height_px"), &CameraParams::get_viewport_height_px);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "viewport_height_px"), "set_viewport_height_px", "get_viewport_height_px");

    ClassDB::bind_method(D_METHOD("set_forward", "forward"), &CameraParams::set_forward);
    ClassDB::bind_method(D_METHOD("get_forward"), &CameraParams::get_forward);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "forward"), "set_forward", "get_forward");

    ClassDB::bind_method(D_METHOD("set_aspect", "aspect"), &CameraParams::set_aspect); 
    ClassDB::bind_method(D_METHOD("get_aspect"), &CameraParams::get_aspect);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "aspect"), "set_aspect", "get_aspect");

    ClassDB::bind_method(D_METHOD("set_near", "near"), &CameraParams::set_near);
    ClassDB::bind_method(D_METHOD("get_near"), &CameraParams::get_near);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "near"), "set_near", "get_near");

    ClassDB::bind_method(D_METHOD("set_far", "far"), &CameraParams::set_far);
    ClassDB::bind_method(D_METHOD("get_far"), &CameraParams::get_far);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "far"), "set_far", "get_far");
}
