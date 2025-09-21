extends CharacterBody3D


const SPEED = 100.0

func _physics_process(delta: float) -> void:
	var dst = SPEED * delta
	
	var input_dir := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	var direction := (transform.basis * Vector3(input_dir.x, 0, input_dir.y)).normalized()
	if direction:
		velocity.x = direction.x * dst
		velocity.z = direction.z * dst
	else:
		velocity.x = move_toward(velocity.x, 0, dst)
		velocity.z = move_toward(velocity.z, 0, dst)

	move_and_slide()
