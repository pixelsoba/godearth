#pragma once
#include <godot_cpp/classes/node3d.hpp>

class Globe3D : public godot::Node3D {
    GDCLASS(Globe3D, godot::Node3D);
  public:
    void _ready() override;
  protected:
    static void _bind_methods();
};
