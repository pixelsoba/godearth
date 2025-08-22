extends Control

func _ready():
	print("GDAL:", Gis.get_gdal_version())

	var vs := VectorSource.new()
	# Mets un vrai chemin (fixe ou via res:// si tu as importé un GeoJSON en res)
	var paths := vs.load_paths("C:/temp/sample.geojson")
	
	print("Loaded polylines:", paths.size())
