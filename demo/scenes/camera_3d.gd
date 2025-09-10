extends Camera3D

@export var speed: float = 2.0

func _process(delta: float) -> void:
	#var dir = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	
	var dist = speed * delta
	if Input.is_action_pressed("move_up"):
		position += (transform.basis.y) * dist
	if Input.is_action_pressed("move_down"):
		position += -(transform.basis.y) * dist
	if Input.is_action_pressed("move_left"):
		position += (-transform.basis.x) * dist
	if Input.is_action_pressed("move_right"):
		position += (transform.basis.x) * dist
	if Input.is_action_pressed("move_forward"):
		position += (-transform.basis.z) * dist
	if Input.is_action_pressed("move_backward"):
		position += (transform.basis.z) * dist
