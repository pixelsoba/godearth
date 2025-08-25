extends Control

func _ready():
	# Test basic setup
	print("GDAL:", Gis.get_gdal_version())
	
	# Test create ellipsoid
	var e := Ellipsoid.create_scaled_wgs84()
	add_child(e)
	
	# Test create geodetic under ellipsoid
	var geo_obj := Geodetic3D.new()
	geo_obj.altitude = 200
	geo_obj.latitude = 0.125
	geo_obj.longitude = 42.5
	e.add_child(geo_obj)
	
	# Test sphere generation
	var t = TetrahedronTessellator.new()
	var mesh_data = t.create_mesh(2, 6378137.0)
	print("Generating ", mesh_data.vertices.size(), " vertex.")
	
	# Test Gdal vectorial import
	var vs := VectorSource.new()
	var paths := vs.load_paths("res://data/sample.geojson")
	print("Loaded polylines:", paths.size())
