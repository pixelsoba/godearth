// RasterSource.h
#pragma once

#include <gdal_priv.h>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <unordered_map>

using namespace godot;

class RasterSource : public RefCounted {
    GDCLASS(RasterSource, RefCounted);

private:
    GDALDataset *dataset {nullptr};
    String path;

    std::unordered_map<std::string, Variant> tile_cache;

public:
    RasterSource();
    ~RasterSource();

    Error open(const String& p_path);
    bool has_band(int idx) const;

    // Read a tile. If band_count >= 3 returns Ref<Image>, otherwise returns PackedFloat32Array
    Variant get_tile(int band_start, int band_count,
                     int px_w, int px_h,
                     double ulx, double uly,
                     double lrx, double lry);
protected:
    static void _bind_methods();
};