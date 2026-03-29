extends RigidBody3D

## Striker input handling — placement, aiming, and power control.
## Attached to the Striker RigidBody3D at runtime by board.gd.

var _aim_indicator: MeshInstance3D = null


func _ready() -> void:
	_create_aim_indicator()
	GameManager.state_changed.connect(_on_state_changed)


func _create_aim_indicator() -> void:
	_aim_indicator = MeshInstance3D.new()
	var cone := CylinderMesh.new()
	cone.top_radius = 0.0
	cone.bottom_radius = 0.02
	cone.height = 0.6
	_aim_indicator.mesh = cone
	# Rotate so it points along Z
	_aim_indicator.rotation.x = deg_to_rad(90)
	_aim_indicator.position = Vector3(0, 0.03, -0.35)

	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(1, 0.3, 0.3, 0.7)
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_aim_indicator.material_override = mat
	_aim_indicator.visible = false
	add_child(_aim_indicator)


func _on_state_changed(new_state: int) -> void:
	match new_state:
		GameManager.State.AIM:
			_aim_indicator.visible = true
		GameManager.State.POWER:
			_aim_indicator.visible = true
		_:
			_aim_indicator.visible = false


func _unhandled_input(event: InputEvent) -> void:
	if not GameManager.game_active:
		return

	match GameManager.current_state:
		GameManager.State.PLACE_STRIKER:
			_handle_placement(event)
		GameManager.State.AIM:
			_handle_aim(event)
		GameManager.State.POWER:
			_handle_power(event)


func _handle_placement(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		# Convert mouse X to world X
		var viewport := get_viewport()
		var mouse_x: float = event.position.x
		var screen_w: float = viewport.get_visible_rect().size.x
		# Map screen X [0, screen_w] → world X [-3.7, 3.7] then clamp to placement zone
		var world_x: float = (mouse_x - screen_w / 2.0) / screen_w * 7.4
		GameManager.place_striker_at(world_x)

	elif event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
			GameManager.confirm_placement()


func _handle_aim(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		var viewport := get_viewport()
		var mouse_x: float = event.position.x
		var screen_w: float = viewport.get_visible_rect().size.x
		# Map mouse X to angle: center = 0°, edges = ±75°
		var angle: float = -(mouse_x - screen_w / 2.0) / (screen_w / 2.0) * 75.0
		GameManager.set_aim_angle(angle)
		# Rotate aim indicator
		_update_aim_visual()

	elif event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
			GameManager.confirm_aim()


func _handle_power(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		if event.pressed:
			# Hold to charge
			GameManager.is_charging = true
			AudioManager.play_power_bar()
		else:
			# Release to fire (only if charging started)
			if GameManager.is_charging:
				GameManager.release_power()


func _update_aim_visual() -> void:
	if _aim_indicator == null:
		return
	var angle_rad := deg_to_rad(GameManager.aim_angle)
	var flip := 1.0 if GameManager.current_player == 1 else -1.0
	# Offset cone in shoot direction: P1 toward +Z, P2 toward -Z
	_aim_indicator.position = Vector3(
		sin(angle_rad) * flip * 0.35,
		0.03,
		cos(angle_rad) * flip * 0.35
	)
	_aim_indicator.rotation = Vector3(deg_to_rad(90), -angle_rad * flip, 0)
