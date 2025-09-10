extends Control

func _ready():
	# Test basic setup
	print("GDAL:", Gis.get_gdal_version())

func test():
	
	
	
	
	
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
	
	# GDScript
	var raster = RasterSource.new()
	#raster.open("res://data/eu_pasture.tiff")
	raster.open("res://data/NE2_LR_LC/NE2_LR_LC.tif")
	var heights = raster.get_tile(2, 2, 256, 256, -5.0, 51.5, 10.0, 41.0)
	print("Heights count: ", heights.size())
	
	var mesh = HeightmapTessellator.build_mesh(heights, 256, -5.0, 51.5, 10.0, 41.0, Ellipsoid.new())
	print("AABB for Heightmap raster: ", mesh.get_aabb())
	
	var mi = MeshInstance3D.new()
	mi.mesh = mesh
	add_child(mi)

	# Test Gdal vectorial import
	var vs := VectorSource.new()
	var paths := vs.load_paths("res://data/sample.geojson")
	print("Loaded polylines:", paths.size())

func _input(event: InputEvent) -> void:
	if event.is_action_released("ui_accept"):
		test()
