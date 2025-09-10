#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {

class CameraParams : public Resource {
    GDCLASS(CameraParams, Resource);

public:
    static void _bind_methods();

    void set_position(const Vector3& p) { position_ = p; }
    Vector3 get_position() const { return position_; }

    void set_fov_y_deg(double f) { fov_y_deg_ = (float)f; }
    double get_fov_y_deg() const { return (double)fov_y_deg_; }

    void set_viewport_height_px(double h) { viewport_height_px_ = (float)h; }
    double get_viewport_height_px() const { return (double)viewport_height_px_; }

    void set_forward(const Vector3& f) { forward_ = f; }
    Vector3 get_forward() const { return forward_; }

    void set_aspect(double a) { aspect_ = (float)a; }
    double get_aspect() const { return (double)aspect_; }

    void set_near(double n) { near_ = (float)n; }
    double get_near() const { return (double)near_; }

    void set_far(double f) { far_ = (float)f; }
    double get_far() const { return (double)far_; }

private:
    Vector3 position_{};
    float   fov_y_deg_ = 60.0f;
    float   viewport_height_px_ = 1080.0f;
    Vector3 forward_{0,0,-1};
    float   aspect_ = 16.0f/9.0f;
    float   near_ = 0.05f, far_ = 1000.0f;
};

} // namespace godot
