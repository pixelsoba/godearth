#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "gis_singleton.hpp"
#include "map2d_control.hpp"
#include "globe3d.hpp"
#include "vector_source.hpp"

using namespace godot;

static GisSingleton *g_gis_singleton = nullptr;

static void initialize_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<GisSingleton>();
    ClassDB::register_class<Map2DControl>();
    ClassDB::register_class<Globe3D>();
    ClassDB::register_class<VectorSource>();

    g_gis_singleton = memnew(GisSingleton);
    Engine::get_singleton()->register_singleton("Gis", g_gis_singleton);
}

static void uninitialize_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    if (Engine::get_singleton()->has_singleton("Gis")) {
        Engine::get_singleton()->unregister_singleton("Gis");
    }
    if (g_gis_singleton) {
        memdelete(g_gis_singleton);
        g_gis_singleton = nullptr;
    }
}

extern "C" GDExtensionBool GDE_EXPORT
gdextension_entry(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                  GDExtensionClassLibraryPtr p_library,
                  GDExtensionInitialization *r_initialization) {
    GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_module);
    init_obj.register_terminator(uninitialize_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
