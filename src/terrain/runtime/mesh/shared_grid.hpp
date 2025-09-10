#pragma once
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

class SharedGrid : public Object {
    GDCLASS(SharedGrid, Object);

public:
    static void _bind_methods();

    Ref<ArrayMesh> get_or_create_grid(int resolution = 65);

private:
    Ref<ArrayMesh> grid_;
};

} // namespace godot