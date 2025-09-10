extends Node3D

@onready var grid    = SharedGrid.new()
@onready var quadcpu = QuadtreeCPU.new()

@onready var mm := MultiMesh.new()

@onready var cam: Camera3D = $Camera3D
@onready var mminst: MultiMeshInstance3D = $MultiMeshInstance3D
@onready var mesh_instance: MeshInstance3D = $MeshInstance3D
@onready var label_info_1: Label = %LabelInfo1
@onready var label_info_2: Label = %LabelInfo2

@onready var qt = QuadtreeCPU.new()

var mat: ShaderMaterial

func _ready() -> void:
	var shared_grid = SharedGrid.new().get_or_create_grid(65)
	mesh_instance.mesh = shared_grid

	mm.transform_format = MultiMesh.TRANSFORM_3D   # on peut laisser Identity, le shader place tout
	mm.mesh = shared_grid
	mm.use_custom_data = true
	mminst.multimesh = mm
	
	qt.set_max_lod(7)          # par ex.
	qt.set_screen_error_px(16)
	
	mat = mminst.material_override as ShaderMaterial

func _process(_dt):
	var cp := CameraParams.new()
	cam = get_viewport().get_camera_3d()
	cp.position = cam.global_position
	cp.fov_y_deg = cam.fov
	cp.viewport_height_px = get_viewport().get_visible_rect().size.y
	cp.aspect = float(get_viewport().get_visible_rect().size.x) / float(get_viewport().get_visible_rect().size.y)
	cp.near   = cam.near
	cp.far    = cam.far
	cp.forward = -(cam.global_transform.basis.z) # Godot: -Z est forward

	var tiles : Array = qt.build_tile_list(cp)
	mm.instance_count = tiles.size()
	var max_lod := 0
	for i in tiles.size():
		var t = tiles[i]
		max_lod = max(max_lod, t.lod)
	label_info_1.text = str("Tiles count: ", tiles.size(), " - Max LOD: ",  max_lod)
	label_info_2.text = str("Cam pos: ", cp.position)
	for i in tiles.size():
		var t = tiles[i]
		var tile_size = 1.0 / pow(2.0, t.lod)
		var tx = float(t.ix) * tile_size
		var ty = float(t.iy) * tile_size
		mm.set_instance_transform(i, Transform3D.IDENTITY)
		mm.set_instance_custom_data(i, Color(tx, ty, tile_size, 0.0))
