#pragma once
#include <godot_cpp/classes/object.hpp>

class GisSingleton : public godot::Object {
    GDCLASS(GisSingleton, godot::Object);

  public:
    GisSingleton();
    ~GisSingleton();

    godot::String get_gdal_version() const;
    void set_cache_mb(int mb);

  protected:
    static void _bind_methods();
};
