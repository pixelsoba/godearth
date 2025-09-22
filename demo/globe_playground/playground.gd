#@tool
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

const TAU_SPLIT_PX := 64.0
const TAU_MERGE_PX := 32.0
const LEAF_BUDGET := 800
const MAX_SPLITS_PER_FRAME := 24
const MAX_MERGES_PER_FRAME := 48

var _split_q: Array = []   # items: {p:Patch, prio:float}
var _merge_q: Array = []   # items: {p:Patch, prio:float}

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
		_frame_select_and_apply(cam)
		#_update_lod(root, cam)

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

static var mode_wireframe: bool = false
func _input(event: InputEvent) -> void:
	if event.is_action_released("mode_wireframe"):
		mode_wireframe = !mode_wireframe
		get_tree().root.get_viewport().debug_draw = Viewport.DEBUG_DRAW_WIREFRAME if mode_wireframe else 0


####################### Culling
# Debug + culling sûrs

@export var DEBUG_CULLING_MODE := 1   # 0=off, 1=minimal, 2=minimal+horizon

var _dbg_visits := 0
var _dbg_visible := 0
var _dbg_cull_front := 0
var _dbg_cull_back := 0
var _dbg_cull_horiz := 0
var _dbg_leaves := 0
var _dbg_sreq := 0
var _dbg_mreq := 0
var _dbg_sapplied := 0
var _dbg_mapplied := 0
var _dbg_last_size_px := 0.0

func _frame_select_and_apply(cam: Camera3D) -> void:
	# reset stats
	_dbg_visits = 0; _dbg_visible = 0; _dbg_cull_front = 0; _dbg_cull_back = 0; _dbg_cull_horiz = 0
	_dbg_leaves = 0; _dbg_sreq = 0; _dbg_mreq = 0; _dbg_sapplied = 0; _dbg_mapplied = 0
	_split_q.clear(); _merge_q.clear()

	for root in faces:
		_dbg_leaves += _visit_for_candidates(root, cam)

	# merges then splits (exemple simple)
	_merge_q.sort_custom(func(a,b): return a.prio < b.prio)
	for i in min(_merge_q.size(), MAX_MERGES_PER_FRAME):
		_merge(_merge_q[i].p, cam); _dbg_mapplied += 1

	_split_q.sort_custom(func(a,b): return a.prio > b.prio)
	for i in min(_split_q.size(), MAX_SPLITS_PER_FRAME):
		_split(_split_q[i].p, cam); _dbg_sapplied += 1

	_update_labels(cam)

func _visit_for_candidates(p:Patch, cam:Camera3D) -> int:
	_dbg_visits += 1
	if not _visible_patch(p, cam):
		if p.mesh_instance: p.mesh_instance.visible = false
		return 0

	_dbg_visible += 1
	var size_px := _patch_size_px(p, cam)
	_dbg_last_size_px = size_px

	if p.children.is_empty():
		if p.mesh_instance: p.mesh_instance.visible = true
		if size_px > TAU_SPLIT_PX and p.level < MAX_LEVEL:
			_split_q.append({ "p": p, "prio": size_px }); _dbg_sreq += 1
		return 1
	else:
		if p.mesh_instance: p.mesh_instance.visible = false
		if size_px < TAU_MERGE_PX:
			_merge_q.append({ "p": p, "prio": size_px }); _dbg_mreq += 1
			return 1
		var leaves := 0
		for c in p.children:
			leaves += _visit_for_candidates(c, cam)
		return leaves

func _visible_patch(p:Patch, cam:Camera3D) -> bool:
	if DEBUG_CULLING_MODE == 0:
		return true

	# centre du patch
	var uvC := p.rect.position + p.rect.size*0.5
	var Cdir := _face_uv_to_dir(p.face, uvC)
	var C    := Cdir * RADIUS

	var cam_pos := cam.global_transform.origin
	var cam_fwd := -cam.global_transform.basis.z

	# 1) devant la caméra (plan de vue)
	var v_cam := C - cam_pos
	if v_cam.dot(cam_fwd) <= 0.0:
		_dbg_cull_front += 1
		return false

	# 2) back-face (normale tournée à l’opposé de la caméra)
	#   si le vecteur du point vers la caméra est "dans le même sens" que la normale,
	#   on regarde la face arrière => cull
	var v_point2cam := (cam_pos - C).normalized()
	if Cdir.dot(v_point2cam) >= 0.0:
		_dbg_cull_back += 1
		return false

	if DEBUG_CULLING_MODE == 1:
		return true

	# 3) horizon culling (approx sûre)
	var d := cam_pos.length()
	if d <= RADIUS:
		return true
	var alpha := acos(clamp(RADIUS / d, -1.0, 1.0))  # demi-angle horizon
	# angle centre->coin ~ "rayon angulaire" du patch
	var uvCorner := uvC + p.rect.size*0.5 * Vector2(1,1)
	var cornerDir := _face_uv_to_dir(p.face, uvCorner)
	var theta := acos(clamp(Cdir.dot(cornerDir), -1.0, 1.0))
	# angle entre la direction centre de patch et direction caméra
	var cam_dir := cam_pos.normalized()
	var gamma := acos(clamp(Cdir.dot(cam_dir * -1.0), -1.0, 1.0))
	if gamma > alpha + theta:
		_dbg_cull_horiz += 1
		return false

	return true

func _patch_size_px(p:Patch, cam:Camera3D) -> float:
	var uv := p.rect
	var uvC := uv.position + uv.size*0.5
	var uvR := uvC + Vector2(uv.size.x*0.5, 0.0)
	var uvU := uvC + Vector2(0.0, uv.size.y*0.5)
	var C := _face_uv_to_dir(p.face, uvC) * RADIUS
	var R := _face_uv_to_dir(p.face, uvR) * RADIUS
	var U := _face_uv_to_dir(p.face, uvU) * RADIUS
	var size_world := maxf((R-C).length(), (U-C).length()) * 2.0

	var cam_pos := cam.global_transform.origin
	var cam_fwd := -cam.global_transform.basis.z
	var depth := (C - cam_pos).dot(cam_fwd)
	if depth <= 1e-3:
		return 0.0

	var fov_v := deg_to_rad(cam.fov)
	var focal_px := 0.5 * float(get_viewport().size.y) / tan(fov_v * 0.5)
	return size_world * focal_px / depth

func _update_labels(cam:Camera3D) -> void:
	if not DEBUG_LABELS: return
	if not is_instance_valid(label_debug_1): return

	var pos := cam.global_transform.origin
	label_debug_1.text = "Cam (%.1f,%.1f,%.1f) | L=%d | size_px=%.1f τS=%.0f τM=%.0f" % [
		pos.x, pos.y, pos.z, MAX_LEVEL, _dbg_last_size_px, TAU_SPLIT_PX, TAU_MERGE_PX
	]
	label_debug_1.text = str("FPS: ", Engine.get_frames_per_second(), " ", label_debug_1.text)

	label_debug_2.text = "visit=%d vis=%d leaves=%d | split rq/ap=%d/%d merge rq/ap=%d/%d" % [
		_dbg_visits, _dbg_visible, _dbg_leaves, _dbg_sreq, _dbg_sapplied, _dbg_mreq, _dbg_mapplied
	]

	var mode_txt := "cull=?"
	match DEBUG_CULLING_MODE:
		0: mode_txt = "cull=OFF"
		1: mode_txt = "cull=MIN"
		2: mode_txt = "cull=MIN+HOR"
		_: mode_txt = "cull=?"
	label_debug_3.text = "%s | front=%d back=%d horiz=%d" % [
		mode_txt, _dbg_cull_front, _dbg_cull_back, _dbg_cull_horiz
	]
