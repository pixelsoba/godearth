#pragma once

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/classes/ref_counted.hpp>

class Ellipsoid; // forward decl (inclu dans le .cpp)

/// Génère un maillage ellipsoïdal (WGS-84) à partir d’une grille de hauteurs.
/// - heights : n×n, rangé en ligne (iy * n + ix)
/// - n       : nombre de points par côté
/// - [lat0,lon0] (UL) → [lat1,lon1] (LR) en degrés
class HeightmapTessellator : public godot::RefCounted{
    GDCLASS(HeightmapTessellator, RefCounted);

public:
    static godot::Ref<godot::ArrayMesh> build_mesh(const godot::PackedFloat32Array& heights,
                                                    int n,
                                                    double lat0, double lon0,
                                                    double lat1, double lon1,
                                                    Ellipsoid* ellipsoid);
                                                   
protected:
    static void _bind_methods();

};
