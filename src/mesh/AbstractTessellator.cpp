#include "AbstractTessellator.hpp"

using namespace godot;

AbstractTessellator::AbstractTessellator() {}
AbstractTessellator::~AbstractTessellator() {}

void AbstractTessellator::_bind_methods() {
    ClassDB::bind_method(D_METHOD("create_mesh", "subdivisions", "radius"), &AbstractTessellator::create_mesh);
}

Dictionary AbstractTessellator::create_mesh(int subdivisions, real_t radius) {
    // Default implementation returns an empty dictionary
    return Dictionary();
}
