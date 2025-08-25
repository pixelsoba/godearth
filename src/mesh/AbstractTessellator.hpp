#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

class AbstractTessellator : public RefCounted {
    GDCLASS(AbstractTessellator, RefCounted);

protected:
    static void _bind_methods();

public:
    AbstractTessellator();
    ~AbstractTessellator();

    // Create a mesh representation. Returns a Dictionary with keys:
    // "vertices": PackedVector3Array, "indices": PackedInt32Array
    virtual Dictionary create_mesh(int subdivisions, real_t radius);
};
