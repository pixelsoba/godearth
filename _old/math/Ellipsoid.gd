@tool
extends Node3D
class_name Ellipsoid

var WGS84 = Vector3(6378137.0, 6378137.0, 6356752.314245)
var SCALED_WGS84 = Vector3(1.0, 1.0, 6356752.314245 / 6378137.0)
var UNIT_SPHERE = Vector3(1.0, 1.0, 1.0)

func _ready() -> void:
	initialize(SCALED_WGS84)

func initialize(radii: Vector3):
	if radii.x <= 0.0 or radii.y <= 0.0 or radii.z <= 0.0:
		push_error("Invalid radii values")
		return
	
	_radii = radii
	_radii_squared = Vector3(radii.x * radii.x, radii.y * radii.y, radii.z * radii.z)
	_radii_to_the_fourth = Vector3(_radii_squared.x * _radii_squared.x, _radii_squared.y * _radii_squared.y, _radii_squared.z * _radii_squared.z)
	_one_over_radii_squared = Vector3(1.0 / _radii_squared.x, 1.0 / _radii_squared.y, 1.0 / _radii_squared.z)
	print(_one_over_radii_squared)
	RenderingServer.global_shader_parameter_add("u_one_over_radii_squared", RenderingServer.GLOBAL_VAR_TYPE_VEC3, _one_over_radii_squared)

func get_minimum_radius() -> float:
	return min(_radii.x, min(_radii.y, _radii.z))

func get_maximum_radius() -> float:
	return max(_radii.x, max(_radii.y, _radii.z))

func geodetic_to_3d(geodetic: Geodetic3D) -> Vector3:
	var lat = geodetic.latitude
	var lon = geodetic.longitude
	var alt = geodetic.altitude
	
	# Convertir la latitude et la longitude en radians
	var rad_lat = deg_to_rad(lat)
	var rad_lon = deg_to_rad(lon) - PI/2
	
	# Rayon de l'ellipsoïde (les rayons x, y et z)
	var a = _radii.x  # rayon équatorial
	var b = _radii.z  # rayon polaire
	var e2 = 1.0 - (b * b) / (a * a)  # Excentricité au carré
	
	# Calcul du rayon de courbure en fonction de la latitude
	var N = a / sqrt(1.0 - e2 * sin(rad_lat) * sin(rad_lat))
	
	# Calcul des coordonnées 3D dans le système cartésien
	var cartesian_coord = Vector3(
		# X : rayon * cos(latitude) * cos(longitude)
		(N + alt) * cos(rad_lat) * cos(rad_lon),
		# Z : (N * (1 - e2)) + altitude * sin(latitude)
		((N * (1 - e2)) + alt) * sin(rad_lat),
		# Y : rayon * cos(latitude) * sin(longitude)
		(N + alt) * cos(rad_lat) * sin(rad_lon),
	)
	return cartesian_coord




func create_curve(start: Vector3, stop: Vector3, granularity: float) -> Array[Vector3]:
	assert(granularity > 0.0, "Granularity must be greater than zero.")
	
	var normal: Vector3 = start.cross(stop).normalized()
	var theta: float = start.angle_to(stop)
	var n: int = max( int(theta / granularity) - 1, 0)
	
	var positions: Array[Vector3] = [start]
	
	for i in range(n):
		var phi = (i * granularity)
		positions.push_back(scale_to_geodetic_surface(start.rotated(normal, phi)))
	positions.push_back(stop)
	
	return positions

func scale_to_geodetic_surface(p: Vector3) -> Vector3:
	var beta = 1.0 / sqrt(
		(p.x * p.x) * _one_over_radii_squared.x +
		(p.y * p.y) * _one_over_radii_squared.y +
		(p.z * p.z) * _one_over_radii_squared.z)
	var n = Vector3(
		beta * p.x * _one_over_radii_squared.x,
		beta * p.y * _one_over_radii_squared.y,
		beta * p.z * _one_over_radii_squared.z).length()
	var alpha = (1.0 - beta) * (p.length() / n)
	var x2 = p.x * p.x
	var y2 = p.y * p.y
	var z2 = p.z * p.z
	
	var da = 0.0
	var db = 0.0
	var dc = 0.0
	var s = 0.0
	var dSda = 1.0
	var dSdb = 1.0
	var dSdc = 1.0
	
	while abs(s) > 1e-10:
		alpha -= s / dSda
		da = 1.0 + (alpha * _one_over_radii_squared.x)
		db = 1.0 + (alpha * _one_over_radii_squared.y)
		dc = 1.0 + (alpha * _one_over_radii_squared.z)
		
		var da2 = da * da
		var db2 = db * db
		var dc2 = dc * dc
		var da3 = da * da2
		var db3 = db * db2
		var dc3 = dc * dc2
		
		s = x2 / (_radii_squared.x * da2) +\
			y2 / (_radii_squared.y * db2) +\
			z2 / (_radii_squared.z * dc2) - 1.0
		
		dSda = -2.0 * (x2 / (_radii_to_the_fourth.x * da3) +
					   y2 / (_radii_to_the_fourth.y * db3) +
					   z2 / (_radii_to_the_fourth.z * dc3))
	return Vector3(p.x / da, p.y / db, p.z / dc)

# Normale de surface géocentrique
static func centric_surface_normal(position: Vector3) -> Vector3:
	return position.normalized()

# Normale de surface géodésique
func geodetic_surface_normal(position: Vector3) -> Vector3:
	return (position * _one_over_radii_squared).normalized()

func geodetic_surface_normal_geodetic(latitude: float, longitude: float) -> Vector3:
	var cos_latitude = cos(latitude)
	return Vector3(
		cos_latitude * cos(longitude),
		cos_latitude * sin(longitude),
		sin(latitude)
	)

# Calcul des intersections
func intersections(origin: Vector3, direction: Vector3) -> Array[float]:
	direction = direction.normalized()
	
	var a = direction.x * direction.x * _one_over_radii_squared.x + direction.y * direction.y * _one_over_radii_squared.y + direction.z * direction.z * _one_over_radii_squared.z
	var b = 2.0 * (origin.x * direction.x * _one_over_radii_squared.x + origin.y * direction.y * _one_over_radii_squared.y + origin.z * direction.z * _one_over_radii_squared.z)
	var c = origin.x * origin.x * _one_over_radii_squared.x + origin.y * origin.y * _one_over_radii_squared.y + origin.z * origin.z * _one_over_radii_squared.z - 1.0

	var discriminant = b * b - 4 * a * c
	if discriminant < 0.0:
		return []  # Pas d'intersections
	elif discriminant == 0.0:
		return [-0.5 * b / a]  # Une intersection tangentielle
	
	var t = -0.5 * (b + (1.0 if b > 0.0 else -1.0) * sqrt(discriminant))
	var root1 = t / a
	var root2 = c / t

	if root1 < root2:
		return [root1, root2]
	else:
		return [root2, root1]

# Variables privées pour stocker les rayons
var _radii: Vector3
var _radii_squared: Vector3
var _radii_to_the_fourth: Vector3
var _one_over_radii_squared: Vector3
