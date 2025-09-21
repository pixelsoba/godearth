@tool
extends Node3D

# --- DEBUG FLAGS ---
@export var DEBUG_VERBOSE := false      # prints détaillés dans l’Output
@export var DEBUG_LABELS := true       # mises à jour des 3 Labels

# --- LABELS FOURNIS PAR TOI ---
@onready var label_debug_1: Label = %LabelDebug1
@onready var label_debug_2: Label = %LabelDebug2
@onready var label_debug_3: Label = %LabelDebug3

# --- PARAMS GLOBE (garde tes valeurs) ---
const MAX_LEVEL := 10
const PATCH_SIZE := 10
const RADIUS := 1.0
const SPLIT_PIX_ERR := 0.1

var max_level: int =  -1

class Patch extends RefCounted:
	var face: int
	var level: int
	var rect: Rect2
	var mesh_instance: MeshInstance3D
	var children: Array[Patch] = []
	
	func delete() -> void:
		mesh_instance.queue_free()
		for c: Patch in children: c.delete()
		children.clear()

var faces: Array = []

# --- METRICS RUNTIME ---
var _dbg_splits := 0
var _dbg_merges := 0
var _dbg_last_reason := ""   # dernière décision de LOD
var _dbg_patch_count := 0
var _dbg_frame := 0
var _dbg_time_acc := 0.0

func _ready():
	faces.clear()
	for f in 6:
		var p = _make_patch_root(f)
		faces.append(p)
	if DEBUG_VERBOSE:
		print("[Planet] Ready. 6 faces, MAX_LEVEL=%d, PATCH_SIZE=%d" % [MAX_LEVEL, PATCH_SIZE])

func _process(dt:float):
	_dbg_frame += 1
	_dbg_time_acc += dt
	var cam := get_viewport().get_camera_3d()
	if cam == null:
		if DEBUG_LABELS:
			label_debug_1.text = "Cam: null"
			label_debug_2.text = ""
			label_debug_3.text = ""
		return

	_dbg_splits = 0
	_dbg_merges = 0
	_dbg_patch_count = 0
	for root in faces:
		_update_lod(root, cam)

	# Affichage labels toutes les ~0.1 s
	if _dbg_time_acc > 0.1:
		_update_labels(cam)
		_dbg_time_acc = 0.0

# ---------- LOD ----------
func _update_lod(p:Patch, cam:Camera3D):
	_dbg_patch_count += 1

	var want_split := _needs_split(p, cam)
	if want_split and p.level < MAX_LEVEL and p.children.is_empty():
		_split(p, cam)
	elif not want_split and not p.children.is_empty():
		_merge(p, cam)

	for c in p.children:
		_update_lod(c, cam)

func _needs_split(p: Patch, cam: Camera3D) -> bool:
	const SPLIT_PX := 512  # cible ~48–96 px

	# 1) centre + demi-axes du patch en UV (PAS de / 2^level supplémentaire)
	var uv  := p.rect
	var uvC := uv.position + uv.size * 0.5
	var uvR := uvC + Vector2( uv.size.x * 0.5, 0.0)
	var uvU := uvC + Vector2( 0.0,            uv.size.y * 0.5)

	# 2) points monde sur la sphère
	var C := _face_uv_to_dir(p.face, uvC) * RADIUS
	var R := _face_uv_to_dir(p.face, uvR) * RADIUS
	var U := _face_uv_to_dir(p.face, uvU) * RADIUS

	# 3) “taille monde” du patch (diamètre local, chordes)
	var w_world := (R - C).length() * 2.0
	var h_world := (U - C).length() * 2.0
	var size_world := maxf(w_world, h_world)

	# 4) profondeur vue (distance le long de l’axe -Z caméra)
	var cam_pos := cam.global_transform.origin
	var cam_fwd := -cam.global_transform.basis.z
	var depth := (C - cam_pos).dot(cam_fwd)
	if depth <= 1e-3:
		return false  # derrière ou trop proche du near

	# 5) focal en pixels (FOV vertical)
	var fov_v := deg_to_rad(cam.fov)
	var focal_px := 0.5 * float(get_viewport().size.y) / tan(fov_v * 0.5)

	# 6) taille écran estimée
	var size_px = size_world * focal_px / depth
	var split = size_px > SPLIT_PX

	# debug optionnel
	if DEBUG_VERBOSE and _dbg_frame % 15 == 0:
		_dbg_last_reason = "L=%d size_px=%.1f depth=%.2f -> %s" % [
			p.level, size_px, depth, "SPLIT" if split else "KEEP"
		]
	return split

func _split(p: Patch, _cam: Camera3D):
	_dbg_splits += 1
	if DEBUG_VERBOSE:
		print("[LOD] split face=%d L=%d rect=%s" % [p.face, p.level, str(p.rect)])
	var uv = p.rect
	var hs = uv.size * 0.5
	var offs = [Vector2(0,0), Vector2(hs.x,0), Vector2(0,hs.y), Vector2(hs.x,hs.y)]
	for o in offs:
		var c = Patch.new()
		c.face = p.face
		c.level = p.level + 1
		if max_level < c.level:
			max_level = c.level
			print(max_level)
		
		c.rect = Rect2(uv.position + o, hs)
		c.mesh_instance = _build_patch_mesh(c)
		add_child(c.mesh_instance)
		p.children.append(c)
	p.mesh_instance.visible = false

func _merge(p: Patch, _cam: Camera3D):
	_dbg_merges += 1
	if DEBUG_VERBOSE:
		print("[LOD] merge face=%d L=%d rect=%s" % [p.face, p.level, str(p.rect)])
	for c: Patch in p.children:
		c.delete()
	p.children.clear()
	p.mesh_instance.visible = true

# ---------- MESH ----------
func _make_patch_root(face:int) -> Patch:
	var p = Patch.new()
	p.face = face
	p.level = 0
	p.rect = Rect2(Vector2(-1,-1), Vector2(2,2))
	p.mesh_instance = _build_patch_mesh(p)
	add_child(p.mesh_instance)
	return p

const DUMMY_MAT = preload("res://globe_playground/data/dummy_mat.tres")

func _build_patch_mesh(p:Patch) -> MeshInstance3D:
	var st = SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	# Génération minimale. A remplacer par une grille indexée propre au besoin.
	for y in PATCH_SIZE - 1:
		for x in PATCH_SIZE - 1:
			var v00 = _vert_at(p, x,   y)
			var v10 = _vert_at(p, x+1, y)
			var v01 = _vert_at(p, x,   y+1)
			var v11 = _vert_at(p, x+1, y+1)
			st.add_vertex(v00); st.add_vertex(v01); st.add_vertex(v10)
			st.add_vertex(v10); st.add_vertex(v01); st.add_vertex(v11)
	var mesh = st.commit()
	var mi = MeshInstance3D.new()
	var material := DUMMY_MAT.duplicate()
	mesh.surface_set_material(0, material)
	material.albedo_color = Color(1.0, 0.0, 0.0, 1.0)
	material.albedo_color.h = randf()
	
	mi.mesh = mesh
	return mi

func _vert_at(p:Patch, x:int, y:int) -> Vector3:
	var u = float(x)/(PATCH_SIZE-1)
	var v = float(y)/(PATCH_SIZE-1)
	var uv = p.rect.position + Vector2(u, v) * p.rect.size
	var dir = _face_uv_to_dir(p.face, uv)
	return dir * (RADIUS + _height(dir))

# ---------- HEIGHT ----------
func _height(_dir:Vector3) -> float:
	return 0.0

# ---------- MAPPINGS ----------
func _face_uv_to_dir(face:int, uv:Vector2) -> Vector3:
	var x = uv.x
	var y = uv.y
	var v:Vector3
	match face:
		0: v = Vector3( 1,  y, -x) # +X
		1: v = Vector3(-1,  y,  x) # -X
		2: v = Vector3( x,  1, -y) # +Y
		3: v = Vector3( x, -1,  y) # -Y
		4: v = Vector3( x,  y,  1) # +Z
		5: v = Vector3(-x,  y, -1) # -Z
	return _cube_to_sphere(v.normalized())

func _cube_to_sphere(v:Vector3) -> Vector3:
	var x = v.x; var y = v.y; var z = v.z
	var x2 = x * x; var y2 = y * y; var z2 = z * z
	return Vector3(
		x * sqrt(1.0 - (y2 + z2)/2.0 + (y2 * z2)/3.0),
		y * sqrt(1.0 - (z2 + x2)/2.0 + (z2 * x2)/3.0),
		z * sqrt(1.0 - (x2 + y2)/2.0 + (x2 * y2)/3.0)
	).normalized()

# ---------- DEBUG UI ----------
func _update_labels(cam:Camera3D):
	if not DEBUG_LABELS:
		return

	# Label 1: Cam info
	var pos := cam.global_transform.origin
	var dir := cam.global_transform.basis.z * -1.0
	label_debug_1.text = "Cam pos=%.1f, %.1f, %.1f | dir=%.2f, %.2f, %.2f" % [
		pos.x, pos.y, pos.z, dir.x, dir.y, dir.z
	]

	# Label 2: Patches + ops
	label_debug_2.text = "Patches=%d | splits=%d merges=%d | maxL=%d" % [
		_dbg_patch_count, _dbg_splits, _dbg_merges, MAX_LEVEL
	]

	# Label 3: Dernière décision LOD
	label_debug_3.text = _dbg_last_reason

static var mode_wireframe: bool = false
func _input(event: InputEvent) -> void:
	if event.is_action_released("mode_wireframe"):
		mode_wireframe = !mode_wireframe
		get_tree().root.get_viewport().debug_draw = Viewport.DEBUG_DRAW_WIREFRAME if mode_wireframe else 0
