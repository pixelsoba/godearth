#pragma once
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <vector>

#include "camera_params.hpp"

namespace godot {

struct Tile {
    int lod = 0;    // niveau (0 = plus grossier)
    int ix  = 0;    // index X dans ce LOD
    int iy  = 0;    // index Y dans ce LOD
    float morph = 0.0f; // 0..1
};
struct TileKey { int lod, ix, iy; };
struct KeyHash {
    _FORCE_INLINE_ static uint64_t hash(const TileKey &k) {
        return (uint64_t(k.lod) << 32) ^ (uint64_t(k.ix) << 16) ^ uint64_t(k.iy);
    }
};
struct KeyEq {
    _FORCE_INLINE_ static bool compare(const TileKey &a, const TileKey &b) {
        return a.lod==b.lod && a.ix==b.ix && a.iy==b.iy;
    }
};

class QuadtreeCPU : public Object {
    GDCLASS(QuadtreeCPU, Object);

public:
    static void _bind_methods();

    // Paramètres de contrôle
    void set_hysteresis_ratio(float r);
    void set_max_lod(int max_lod);
    void set_screen_error_px(float px);

    // Construit la liste visible (simple: frustum ignoré dans ce squelette)
    Array build_tile_list(const Ref<CameraParams>& cam);

private:
    float hysteresis_ratio_ = 0.75f;
    float target_error_px_ = 64.0f;
    int max_lod_ = 7;


    godot::HashMap<TileKey, bool, KeyHash, KeyEq> hot_split_;
        bool decide_split_with_hysteresis(
        const TileKey& key,
        float proj_px
    );

    bool  should_subdivide_px(
        int lod,
        const Vector3& cam_pos,
        float focal_px,
        const Vector2& tile_center_world,
        float tile_world_size
    ) const;

    float compute_morph_factor(
        int lod,
        const Vector3& cam_pos,
        float focal_px,
        const Vector2& tile_center_world,
        float tile_world_size
    ) const;
};

} // namespace godot
