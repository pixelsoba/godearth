#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

class VectorSource : public godot::RefCounted {
    GDCLASS(VectorSource, godot::RefCounted);
  public:
    godot::Array load_paths(const godot::String &path);
  protected:
    static void _bind_methods();
};
