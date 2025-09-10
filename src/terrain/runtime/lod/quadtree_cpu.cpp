#include "quadtree_cpu.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <queue>
#include <cmath>

using namespace godot;

static inline bool approx_visible_planar(
    const Vector3& cam_pos,
    const Vector3& cam_fwd,    // normalisé
    float fov_y_rad,
    float aspect,
    float near_d,
    float far_d,
    const Vector3& center,     // centre monde de la tuile
    float radius               // rayon englobant (≈ size * sqrt(2)/2)
){
    const Vector3 v = center - cam_pos;
    const float dist = v.length();
    if (dist - radius > far_d)  return false;
    if (dist + radius < near_d) return false;

    // cône vertical
    const float cos_vert = Math::cos(fov_y_rad * 0.5f);
    const Vector3 dir = v / (dist + 1e-6f);
    if (dir.dot(cam_fwd) < cos_vert) return false;

    // (facultatif) cône horizontal approx
    // float tan_half_v = Math::tan(fov_y_rad * 0.5f);
    // float tan_half_h = tan_half_v * aspect;
    // proj horizontale -> test similaire

    return true; // conservatif
}


static inline float focal_length_px(float fov_y_deg, float viewport_h_px) {
    // f = H/2 / tan(FOV/2)
    const float fov = Math::deg_to_rad((float)fov_y_deg);
    return (float)viewport_h_px * 0.5f / Math::tan(fov * 0.5f);
}

static inline float tile_projected_size_px(
    const Vector3& cam,
    const Vector2& center,
    float tile_size_world,
    float focal_px
) {
    // ~ f * (size_world / distance)
    const Vector3 tile_pos(center.x, 0.0f, center.y);
    const float dist = (cam - tile_pos).length() + 1e-3f;
    return focal_px * (tile_size_world / dist);
}

void QuadtreeCPU::set_hysteresis_ratio(float r) {
    hysteresis_ratio_ = CLAMP(r, 0.5f, 0.95f);
}

bool QuadtreeCPU::decide_split_with_hysteresis(const TileKey& key, float proj_px) {
    const float split_px = target_error_px_;
    const float merge_px = target_error_px_ * hysteresis_ratio_;

    const bool want_split = proj_px > split_px;
    const bool want_merge = proj_px < merge_px;

    bool* was_hot = hot_split_.getptr(key);

    if (want_split) {
        hot_split_[key] = true;
        return true;   // split
    }
    if (want_merge) {
        hot_split_.erase(key);
        return false;  // merge/keep parent
    }
    // zone neutre : garder l’état précédent pour stabiliser
    if (was_hot && *was_hot) return true;
    return false;
}

void QuadtreeCPU::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_max_lod", "max_lod"), &QuadtreeCPU::set_max_lod);
    ClassDB::bind_method(D_METHOD("set_screen_error_px", "px"), &QuadtreeCPU::set_screen_error_px);
    ClassDB::bind_method(D_METHOD("build_tile_list", "camera_params"), &QuadtreeCPU::build_tile_list);
}

void QuadtreeCPU::set_max_lod(int max_lod) { max_lod_ = MAX(0, max_lod); }
void QuadtreeCPU::set_screen_error_px(float px) { target_error_px_ = MAX(0.5f, px); }

struct QuadTreeNode { int lod; int ix; int iy; float size; Vector2 center; };

static inline float approx_projected_size_px(const Vector3& cam, const Vector2& center, float size) {
    const Vector3 tile_pos(center.x, 0.0f, center.y);
    const float dist = (cam - tile_pos).length() + 1e-3f;
    return size / dist;
}

bool QuadtreeCPU::should_subdivide_px(
    int lod, const Vector3& cam_pos, float focal_px,
    const Vector2& c, float size_world
) const {
    if (lod >= max_lod_) return false;
    const float s_px = tile_projected_size_px(cam_pos, c, size_world, focal_px);
    // split si la tuile projette plus que target_error_px_
    return s_px > target_error_px_;
}

float QuadtreeCPU::compute_morph_factor(
    int lod, const Vector3& cam_pos, float focal_px,
    const Vector2& c, float size_world
) const {
    // Rampe autour du seuil en pixels (±25% par ex.)
    const float s_px = tile_projected_size_px(cam_pos, c, size_world, focal_px);
    const float a = target_error_px_;
    const float w = a * 0.25f;
    const float t = (s_px - (a - w)) / (2.0f * w);
    return CLAMP(t, 0.0f, 1.0f);
}

Array QuadtreeCPU::build_tile_list(const Ref<CameraParams>& cam) {
    Array out;
    if (cam.is_null()) return out;

    const Vector3 cam_pos = cam->get_position();
    const Vector3 cam_fwd = cam->get_forward().normalized();
    const float   fov_y   = Math::deg_to_rad((float)cam->get_fov_y_deg());
    const float   aspect  = (float)cam->get_aspect();
    const float   near_d  = (float)cam->get_near();
    const float   far_d   = (float)cam->get_far();
    const float focal_px  = focal_length_px((float)cam->get_fov_y_deg(),
                                            (float)cam->get_viewport_height_px());

    std::queue<QuadTreeNode> q;
    q.push({0,0,0,1.0f, Vector2(0.5f,0.5f)});

    while (!q.empty()) {
        auto n = q.front(); q.pop();

        // ---- CULLING ICI (prune traversal) ----
        const float radius = n.size * 0.70710678f; // ~ size*sqrt(2)/2 à y≈0
        const Vector3 center3(n.center.x, 0.0f, n.center.y);
        if (!approx_visible_planar(cam_pos, cam_fwd, fov_y, aspect, near_d, far_d, center3, radius)) {
            continue; // hors champ: on ne split pas, on n'émet pas
        }
        // ---------------------------------------

        // Taille projetée en px
        const float proj_px = tile_projected_size_px(cam_pos, n.center, n.size, focal_px);

        // Décision split avec hystérésis
        const TileKey key{n.lod, n.ix, n.iy};
        const bool split = (n.lod < max_lod_) && decide_split_with_hysteresis(key, proj_px);

        if (split) {
            const int child_lod = n.lod + 1;
            const float hs = n.size * 0.5f;
            for (int dy = 0; dy < 2; ++dy) for (int dx = 0; dx < 2; ++dx) {
                q.push({
                    child_lod,
                    (n.ix << 1) | dx,
                    (n.iy << 1) | dy,
                    hs,
                    Vector2(
                        n.center.x + (dx ? +hs*0.5f : -hs*0.5f),
                        n.center.y + (dy ? +hs*0.5f : -hs*0.5f)
                    )
                });
            }
        } else {
            Dictionary d;
            d["lod"]   = n.lod;
            d["ix"]    = n.ix;
            d["iy"]    = n.iy;
            d["morph"] = compute_morph_factor(n.lod, cam_pos, focal_px, n.center, n.size);
            out.push_back(d);
        }
    }

    return out;
}