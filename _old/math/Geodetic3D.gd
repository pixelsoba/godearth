@tool
extends Node3D
class_name Geodetic3D

# Propriétés géodésiques (latitude, longitude, altitude)
@export_range(-89.99, 89.99) var latitude: float = 0.0: set = _lat_setter  # Plage correcte pour latitude
@export_range(-180.0, 180.0) var longitude: float = 0.0: set = _lon_setter  # Plage correcte pour longitude
@export var altitude: float = 0.0: set = _alt_setter  # Pas de plage spécifique pour l'altitude, mais tu peux la limiter si nécessaire


# Référence à l'ellipsoïde parent (il sera recherché dans la scène)
var ellipsoid: Ellipsoid = null

# Mettre à jour les coordonnées 3D en fonction des coordonnées géodésiques
func _ready():
	# Recherche de l'ellipsoïde dans les parents
	ellipsoid = find_ellipsoid_parent(self)
	if ellipsoid == null:
		push_error("No Ellipsoid found in the parent hierarchy")
		return
	
	# Mettre à jour la position en 3D dès qu'on a l'ellipsoïde
	update_3d_position()

func find_ellipsoid_parent(node: Node) -> Ellipsoid:
	if node is Ellipsoid:
		return node
	if node.get_parent() != null:
		return find_ellipsoid_parent(node.get_parent())
	return null

	

func update_3d_position():
	if ellipsoid:
		position = ellipsoid.geodetic_to_3d(self)

func set_geodetic_coordinates(new_lat: float, new_lon: float, new_alt: float):
	latitude = new_lat
	longitude = new_lon
	altitude = new_alt
	update_3d_position()

func _lat_setter(new: float) -> void:
	latitude = new
	update_3d_position()

func _lon_setter(new: float) -> void:
	longitude = new
	update_3d_position()

func _alt_setter(new: float) -> void:
	altitude = new
	update_3d_position()
