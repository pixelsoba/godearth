#pragma once
#include <godot_cpp/classes/control.hpp>

class Map2DControl : public godot::Control {
    GDCLASS(Map2DControl, godot::Control);
  public:
    Map2DControl() = default;
    void _ready() override;
    void _draw() override;
  protected:
    static void _bind_methods();
};
