extends Control

func _ready():
	print("GDAL:", Gis.get_gdal_version())

	var e := Ellipsoid.create_scaled_wgs84()
	
	
	var vs := VectorSource.new()
	var paths := vs.load_paths("res://data/sample.geojson")
	
	print("Loaded polylines:", paths.size())
