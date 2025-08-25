#include "vector_source.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/class_db.hpp> 
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
using namespace godot;

/*
Note: If your resource is packed inside the project PCK (not an actual file on disk),
`ProjectSettings::globalize_path` will not produce a readable filesystem path. In that
case GDAL cannot open the resource directly. We can implement a fallback that opens the
resource with FileAccess, loads bytes into memory and registers a /vsimem/ file for GDAL
so it can read the dataset from memory.
*/
Array VectorSource::load_paths(const String &path) {
    Array out;

    // Convert Godot resource path (res://, user://) to an absolute filesystem path
    String real_path = ProjectSettings::get_singleton()->globalize_path(path);

    GDALDataset *ds = (GDALDataset*) GDALOpenEx(real_path.utf8().get_data(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (!ds) {
        UtilityFunctions::printerr("GDALOpenEx failed: ", real_path);
        return out;
    }
    for (int i = 0; i < ds->GetLayerCount(); ++i) {
        OGRLayer *layer = ds->GetLayer(i);
        if (!layer) continue;
        layer->ResetReading();
        OGRFeature *feat = nullptr;
        while ((feat = layer->GetNextFeature()) != nullptr) {
            OGRGeometry *geom = feat->GetGeometryRef();
            if (geom && wkbFlatten(geom->getGeometryType()) == wkbLineString) {
                auto *ls = geom->toLineString();
                PackedVector2Array arr;
                arr.resize(ls->getNumPoints());
                for (int p = 0; p < ls->getNumPoints(); ++p) {
                    arr.set(p, Vector2(ls->getX(p), ls->getY(p)));
                }
                out.push_back(arr);
            }
            OGRFeature::DestroyFeature(feat);
        }
    }
    GDALClose(ds);
    return out;
}

void VectorSource::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_paths", "path"), &VectorSource::load_paths);
}

