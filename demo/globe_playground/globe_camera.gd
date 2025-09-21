# GlobeCamera.gd
@tool
extends Camera3D

@export var globe_radius := 1.0
@export var min_altitude := 0.01
@export var zoom_in_factor := 0.9
@export var zoom_out_factor := 1.1
@export var pitch_limit_deg := 85.0
@export var rot_sens := 0.001      # radians par pixel
@export var start_distance := 3.0

var target := Vector3.ZERO
var distance := 3.0
var yaw := 0.0
var pitch := 0.3
var dragging := false

func _ready():
	distance = max(start_distance, globe_radius + min_altitude)
	_update_camera()

func _unhandled_input(event):
	# Zoom géométrique
	if event.is_action_pressed("zoom_in"):
		distance = max(globe_radius + min_altitude, distance * zoom_in_factor)
		_update_camera()
	elif event.is_action_pressed("zoom_out"):
		distance *= zoom_out_factor
		_update_camera()

	# Drag start/stop partout dans la vue
	if event is InputEventMouseButton:
		if event.pressed and Input.is_action_pressed("mouse_click"):
			dragging = true
			Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		elif not event.pressed:
			dragging = false
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

	# Rotation par delta écran
	if event is InputEventMouseMotion and dragging:
		var s := _rot_sens_eff()
		yaw -= event.relative.x * s
		pitch -= event.relative.y * s
		pitch = clamp(pitch, deg_to_rad(-pitch_limit_deg), deg_to_rad(pitch_limit_deg))
		_update_camera()

func _process(_dt):
	if dragging and not Input.is_action_pressed("mouse_click"):
		dragging = false
		Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

func _rot_sens_eff() -> float:
	var alt := maxf(distance - globe_radius, 0.0)     # altitude au-dessus du sol
	var gain := clampf(alt / globe_radius, 0.15, 6.0) # 0.15 près du sol, ↑ avec l’altitude
	return rot_sens * gain

func _update_camera():
	distance = max(distance, globe_radius + min_altitude)
	var cp: float = -clamp(pitch, deg_to_rad(-pitch_limit_deg), deg_to_rad(pitch_limit_deg))
	var cy := yaw
	
	var dir := Vector3(
		cos(cp) * sin(cy),
		sin(cp),
		cos(cp) * cos(cy)
	)
	global_transform.origin = target + dir * distance
	look_at(target, Vector3.UP)
