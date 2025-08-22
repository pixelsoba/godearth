#include "gis_singleton.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <gdal.h>
#include <cpl_conv.h>

using namespace godot;

GisSingleton::GisSingleton() {
    GDALAllRegister();
    CPLSetConfigOption("GDAL_CACHEMAX", "512");
    CPLSetConfigOption("CPL_VSIL_CURL_ALLOWED_EXTENSIONS", "tif,tiff,png,jpg,jpeg,webp,dem");
}

GisSingleton::~GisSingleton() {
    GDALDestroyDriverManager();
}

godot::String GisSingleton::get_gdal_version() const {
    return godot::String(GDALVersionInfo("--version"));
}

void GisSingleton::set_cache_mb(int mb) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", mb);
    CPLSetConfigOption("GDAL_CACHEMAX", buf);
}

void GisSingleton::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("get_gdal_version"), &GisSingleton::get_gdal_version);
    godot::ClassDB::bind_method(godot::D_METHOD("set_cache_mb", "mb"), &GisSingleton::set_cache_mb);
}
