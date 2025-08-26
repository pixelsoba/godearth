#include "raster_source.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/core/memory.hpp>

#include <algorithm>

using namespace godot;

RasterSource::RasterSource() {}

RasterSource::~RasterSource() {
    if (dataset) {
        GDALClose(dataset);
        dataset = nullptr;
    }
}

Error RasterSource::open(const String &p_path) {
    path = p_path;
    dataset = reinterpret_cast<GDALDataset *>(GDALOpenEx(p_path.utf8().get_data(), GDAL_OF_RASTER, nullptr, nullptr, nullptr));
    return dataset ? Error::OK : Error::ERR_CANT_OPEN;
}

bool RasterSource::has_band(int idx) const {
    return dataset && idx >= 1 && idx <= dataset->GetRasterCount();
}

static std::string build_tile_key(int band_start, int band_count, int px_w, int px_h, double ulx, double uly, double lrx, double lry) {
    char buf[256];
    // use snprintf to avoid locale issues
    std::snprintf(buf, sizeof(buf), "%d:%d:%d:%d:%.6f:%.6f:%.6f:%.6f", band_start, band_count, px_w, px_h, ulx, uly, lrx, lry);
    return std::string(buf);
}

Variant RasterSource::get_tile(int band_start, int band_count, int px_w, int px_h, double ulx, double uly, double lrx, double lry) {
    std::string key = build_tile_key(band_start, band_count, px_w, px_h, ulx, uly, lrx, lry);
    auto it = tile_cache.find(key);
    if (it != tile_cache.end()) {
        return it->second;
    }

    if (!dataset) {
        return Variant();
    }

    GDALRasterBand* band = dataset->GetRasterBand(band_start);
    if (!band) return Variant();

    double geo_transform[6];
    if (dataset->GetGeoTransform(geo_transform) != CE_None) {
        return Variant();
    }

    auto world_to_pixel = [&](double gx, double gy, int &px, int &py) {
        double inv_det = 1.0 / (geo_transform[1] * geo_transform[5] - geo_transform[2] * geo_transform[4]);
        double dx = gx - geo_transform[0];
        double dy = gy - geo_transform[3];
        px = static_cast<int>((geo_transform[5] * dx - geo_transform[2] * dy) * inv_det);
        py = static_cast<int>((-geo_transform[4] * dx + geo_transform[1] * dy) * inv_det);
    };

    int px_ul, py_ul, px_lr, py_lr;
    world_to_pixel(ulx, uly, px_ul, py_ul);
    world_to_pixel(lrx, lry, px_lr, py_lr);
    int read_w = px_lr - px_ul;
    int read_h = py_lr - py_ul;
    if (read_w <= 0 || read_h <= 0) return Variant();

    // Allocate buffer (interleaved by band index)
    std::vector<float> buffer((size_t)px_w * px_h * (size_t)band_count);

    CPLErr err = dataset->RasterIO(GF_Read,
                                   px_ul, py_ul, read_w, read_h,
                                   buffer.data(), px_w, px_h, GDT_Float32,
                                   band_count, nullptr, 0, 0, 0);
    if (err != CE_None) {
        return Variant();
    }

    if (band_count >= 3) {
        Ref<Image> img;
        img.instantiate();
        // RGBA8 image data
        godot::PackedByteArray img_data;
        img_data.resize(px_w * px_h * 4);
        auto *dst = img_data.ptrw();
        size_t pixel_count = (size_t)px_w * (size_t)px_h;
        for (size_t i = 0; i < pixel_count; ++i) {
            float r = buffer[i];
            float g = buffer[i + pixel_count];
            float b = buffer[i + 2 * pixel_count];
            float a = (band_count >= 4) ? buffer[i + 3 * pixel_count] : 1.0f;
            dst[i * 4 + 0] = static_cast<uint8_t>(std::clamp(r * 255.0f, 0.0f, 255.0f));
            dst[i * 4 + 1] = static_cast<uint8_t>(std::clamp(g * 255.0f, 0.0f, 255.0f));
            dst[i * 4 + 2] = static_cast<uint8_t>(std::clamp(b * 255.0f, 0.0f, 255.0f));
            dst[i * 4 + 3] = static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f));
        }
        img->create_from_data(px_w, px_h, false, Image::FORMAT_RGBA8, img_data);
        Variant var = img;
        tile_cache.emplace(key, var);
        return var;
    } else {
        PackedFloat32Array arr;
        arr.resize(px_w * px_h);
        for (int i = 0; i < px_w * px_h; ++i) {
            arr.set(i, buffer[i]);
        }
        Variant var = arr;
        tile_cache.emplace(key, var);
        return var;
    }
}
