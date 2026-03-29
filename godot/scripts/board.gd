extends Node3D

## Builds the carrom board, pieces, pockets, and walls procedurally.
## Attached to the root Board node in main.tscn.

const S := 0.01  # scale factor (original units → Godot world)
const BOARD_SIZE := 740.0
const BOARD_HEIGHT := 4.0
const BOUNDARY_HEIGHT := 70.0
const BOUNDARY_BREADTH := 80.0
const POCKET_RADIUS := 28.0
const MEN_RADIUS := 18.0
const MEN_MASS := 12.0
const STRIKER_RADIUS := 22.0
const STRIKER_MASS := 18.0
const MEN_HEIGHT := 2.0
const PI_VAL := 3.141
const DEG := 180.0


func _ready() -> void:
	_build_board_surface()
	_build_boundaries()
	_build_pockets()
	_build_center_circle()
	_build_board_markings()
	_spawn_pieces()
	_spawn_striker()
	GameManager.start_game()


# --- Board Surface ---

func _build_board_surface() -> void:
	var body := StaticBody3D.new()
	body.name = "BoardSurface"

	var mesh_inst := MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = Vector3(BOARD_SIZE * S, BOARD_HEIGHT * S, BOARD_SIZE * S)
	mesh_inst.mesh = box

	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.85, 0.75, 0.45)  # yellow/wood
	mat.roughness = 0.8
	mesh_inst.material_override = mat

	var col := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = box.size
	col.shape = shape

	body.add_child(mesh_inst)
	body.add_child(col)
	body.position.y = -(BOARD_HEIGHT * S) / 2.0  # top surface at y=0
	body.collision_layer = 1  # board layer
	body.collision_mask = 0
	add_child(body)


# --- Boundaries (4 walls) ---

func _build_boundaries() -> void:
	var half := (BOARD_SIZE / 2.0) * S
	var bw := BOUNDARY_BREADTH * S
	var bh := BOUNDARY_HEIGHT * S * 0.3  # visual height scaled down
	var wall_thickness := bw * 0.3

	# Walls: [name, position, size]
	var walls := [
		["WallTop",    Vector3(0, bh / 2.0, -half), Vector3(BOARD_SIZE * S + bw * 2, bh, wall_thickness)],
		["WallBottom", Vector3(0, bh / 2.0,  half), Vector3(BOARD_SIZE * S + bw * 2, bh, wall_thickness)],
		["WallLeft",   Vector3(-half, bh / 2.0, 0), Vector3(wall_thickness, bh, BOARD_SIZE * S + bw * 2)],
		["WallRight",  Vector3( half, bh / 2.0, 0), Vector3(wall_thickness, bh, BOARD_SIZE * S + bw * 2)],
	]

	for w: Array in walls:
		var body := StaticBody3D.new()
		body.name = String(w[0])
		body.position = w[1] as Vector3

		var mesh_inst := MeshInstance3D.new()
		var box := BoxMesh.new()
		box.size = w[2] as Vector3
		mesh_inst.mesh = box
		var mat := StandardMaterial3D.new()
		mat.albedo_color = Color(0.35, 0.2, 0.1)  # dark brown
		mat.roughness = 0.9
		mesh_inst.material_override = mat

		var col := CollisionShape3D.new()
		var shape := BoxShape3D.new()
		shape.size = w[2] as Vector3
		col.shape = shape

		body.add_child(mesh_inst)
		body.add_child(col)
		body.collision_layer = 1  # board layer
		body.collision_mask = 0

		# Physics material for bounce
		var phys_mat := PhysicsMaterial.new()
		phys_mat.bounce = 0.6
		phys_mat.friction = 0.3
		body.physics_material_override = phys_mat

		add_child(body)


# --- Pockets (4 corners) ---

func _build_pockets() -> void:
	var half := (BOARD_SIZE / 2.0) * S
	var offset := half * 0.95  # slightly inset from corner
	var corners := [
		Vector3(-offset, 0, -offset),
		Vector3( offset, 0, -offset),
		Vector3(-offset, 0,  offset),
		Vector3( offset, 0,  offset),
	]

	for i in range(4):
		var area := Area3D.new()
		area.name = "Pocket_%d" % i
		area.position = corners[i]

		var col := CollisionShape3D.new()
		var shape := SphereShape3D.new()
		shape.radius = POCKET_RADIUS * S
		col.shape = shape
		area.add_child(col)

		# Visual: dark circle
		var mesh_inst := MeshInstance3D.new()
		var cyl := CylinderMesh.new()
		cyl.top_radius = POCKET_RADIUS * S
		cyl.bottom_radius = POCKET_RADIUS * S
		cyl.height = 0.005
		mesh_inst.mesh = cyl
		var mat := StandardMaterial3D.new()
		mat.albedo_color = Color(0.05, 0.05, 0.05)
		mesh_inst.material_override = mat
		area.add_child(mesh_inst)

		area.collision_layer = 8  # pocket layer (4)
		area.collision_mask = 6   # pieces (2) + striker (3)
		area.body_entered.connect(_on_pocket_body_entered)

		add_child(area)


func _on_pocket_body_entered(body: Node3D) -> void:
	if body is RigidBody3D:
		GameManager.on_piece_pocketed(body)


# --- Center Circle (visual only) ---

func _build_center_circle() -> void:
	var mesh_inst := MeshInstance3D.new()
	mesh_inst.name = "CenterCircle"
	var torus := TorusMesh.new()
	torus.inner_radius = 0.35
	torus.outer_radius = 0.38
	mesh_inst.mesh = torus
	mesh_inst.position.y = 0.005
	mesh_inst.rotation.x = PI / 2  # flat on board
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.4, 0.2, 0.1)
	mesh_inst.material_override = mat
	add_child(mesh_inst)


# --- Board Markings (baselines + diagonals, visual only) ---

func _build_board_markings() -> void:
	var half := (BOARD_SIZE / 2.0) * S
	var baseline_z := 290.0 * S  # matches PLACEMENT_Y
	var line_thick := 0.005
	var line_y := 0.003
	var line_color := Color(0.3, 0.2, 0.05)

	# Two baselines (one per player side)
	for sign_val in [1.0, -1.0]:
		var m := MeshInstance3D.new()
		var box := BoxMesh.new()
		box.size = Vector3(BOARD_SIZE * S * 0.55, 0.002, line_thick)
		m.mesh = box
		m.position = Vector3(0, line_y, sign_val * baseline_z)
		var mat := StandardMaterial3D.new()
		mat.albedo_color = line_color
		m.material_override = mat
		add_child(m)

	# Four diagonal corner lines
	var diag_data := [
		[Vector3(-half * 0.5, line_y, -half * 0.5), 45.0],
		[Vector3( half * 0.5, line_y, -half * 0.5), -45.0],
		[Vector3(-half * 0.5, line_y,  half * 0.5), -45.0],
		[Vector3( half * 0.5, line_y,  half * 0.5), 45.0],
	]
	for dp: Array in diag_data:
		var m := MeshInstance3D.new()
		var box := BoxMesh.new()
		box.size = Vector3(half * 0.7, 0.002, line_thick)
		m.mesh = box
		m.position = dp[0] as Vector3
		m.rotation.y = deg_to_rad(float(dp[1]))
		var mat := StandardMaterial3D.new()
		mat.albedo_color = line_color
		m.material_override = mat
		add_child(m)


# --- Piece Spawning ---

func _spawn_pieces() -> void:
	# Replicate exact positions from original model.cpp
	# Original: X,Y plane. Godot: X,Z plane (Y is up).
	var r := MEN_RADIUS  # 18
	var d := 2.0 * r + 1.0  # 37 — spacing
	var d2 := 2.0 * r - 4.0  # 32 — inner ring offset
	var cos30 := cos(30.0 * PI_VAL / DEG)
	var sin30 := sin(30.0 * PI_VAL / DEG)

	# [position_x, position_y(→z), color]
	# 0=BLACK, 1=WHITE, 2=RED
	var layout: Array = [
		# Queen (center)
		[0, 0, 2],
		# Inner ring
		[0, d, 0],                                    # BLACK top
		[d * cos30, d * sin30, 1],                     # WHITE
		[-d * cos30, d * sin30, 1],                    # WHITE (mirror)
		[0, -d, 1],                                    # WHITE bottom
		[d * cos30, -d * sin30, 0],                    # BLACK
		[-d * cos30, -d * sin30, 0],                   # BLACK (mirror)
		# Outer ring
		[2 * d2 * sin30, 2 * d2 * cos30, 0],          # BLACK
		[2 * d * cos30, 2 * d * sin30, 1],             # WHITE
		[0, 2 * d, 1],                                  # WHITE
		[2 * d2, 0, 0],                                 # BLACK
		[-2 * d2 * sin30, 2 * d2 * cos30, 0],          # BLACK (mirror)
		[-2 * d * cos30, 2 * d * sin30, 1],             # WHITE
		[-2 * d2, 0, 0],                                 # BLACK
		[2 * d2 * sin30, -2 * d2 * cos30, 0],          # BLACK
		[2 * d * cos30, -2 * d * sin30, 1],             # WHITE
		[-2 * d2 * sin30, -2 * d2 * cos30, 0],          # BLACK
		[-2 * d * cos30, -2 * d * sin30, 1],             # WHITE
		[0, -2 * d, 1],                                  # WHITE
	]

	var colors_map := {
		0: [Color(0.1, 0.1, 0.1), GameManager.PieceColor.BLACK],    # black
		1: [Color(0.95, 0.92, 0.85), GameManager.PieceColor.WHITE],  # white
		2: [Color(0.85, 0.1, 0.1), GameManager.PieceColor.RED],      # red queen
	}

	for i in range(layout.size()):
		var data: Array = layout[i]
		var pos_x: float = float(data[0]) * S
		var pos_z: float = float(data[1]) * S
		var color_idx: int = int(data[2])

		var color_data: Array = colors_map[color_idx]
		var piece := _create_piece(
			MEN_RADIUS * S,
			MEN_HEIGHT * S,
			MEN_MASS,
			color_data[0] as Color,
			color_data[1] as GameManager.PieceColor
		)
		piece.name = "Piece_%d" % i
		piece.position = Vector3(pos_x, MEN_HEIGHT * S * 0.5, pos_z)
		add_child(piece)
		GameManager.pieces.append(piece)

		if color_idx == 2:
			GameManager.queen = piece


func _spawn_striker() -> void:
	var striker := _create_piece(
		STRIKER_RADIUS * S,
		MEN_HEIGHT * S,
		STRIKER_MASS,
		Color(0.05, 0.05, 0.25),  # dark blue
		GameManager.PieceColor.BLACK  # doesn't matter for striker
	)
	striker.name = "Striker"
	striker.position = Vector3(0, MEN_HEIGHT * S * 0.5, -290.0 * S)
	striker.freeze = true

	# Attach striker script
	var striker_script: Resource = load("res://scripts/striker.gd")
	if striker_script:
		striker.set_script(striker_script)

	striker.collision_layer = 4  # striker layer (3)
	striker.collision_mask = 3   # board (1) + pieces (2)

	add_child(striker)
	GameManager.striker = striker


func _create_piece(radius: float, height: float, mass_val: float, color: Color, piece_color: GameManager.PieceColor) -> RigidBody3D:
	var body := RigidBody3D.new()
	body.mass = mass_val
	body.gravity_scale = 0.0
	body.linear_damp = 3.0
	body.angular_damp = 8.0
	body.can_sleep = false  # need velocity checks

	# Lock Y position and rotations (keep pieces flat on board)
	body.axis_lock_linear_y = true
	body.axis_lock_angular_x = true
	body.axis_lock_angular_z = true

	# Mesh (cylinder)
	var mesh_inst := MeshInstance3D.new()
	var cyl := CylinderMesh.new()
	cyl.top_radius = radius
	cyl.bottom_radius = radius
	cyl.height = height
	cyl.radial_segments = 32
	mesh_inst.mesh = cyl

	var mat := StandardMaterial3D.new()
	mat.albedo_color = color
	mat.roughness = 0.6
	mat.metallic = 0.1
	mesh_inst.material_override = mat
	body.add_child(mesh_inst)

	# Collision (cylinder approximated as sphere for better physics)
	var col := CollisionShape3D.new()
	var shape := CylinderShape3D.new()
	shape.radius = radius
	shape.height = height
	col.shape = shape
	body.add_child(col)

	# Physics material
	var phys_mat := PhysicsMaterial.new()
	phys_mat.bounce = 0.5
	phys_mat.friction = 0.4
	body.physics_material_override = phys_mat

	body.collision_layer = 2  # pieces layer
	body.collision_mask = 3   # board (1) + pieces (2)

	# Metadata
	body.set_meta("color", piece_color)

	# Collision sound detection
	body.contact_monitor = true
	body.max_contacts_reported = 4
	body.body_entered.connect(_on_piece_collision.bind(body))

	return body


func _on_piece_collision(other: Node, piece: RigidBody3D) -> void:
	var vel := piece.linear_velocity.length()
	if vel < 0.05:
		return  # skip quiet collisions

	if other is StaticBody3D:
		# Wall collision
		if piece == GameManager.striker:
			AudioManager.play_collision_sound(AudioManager.SFX.STRIKER_WALL, vel)
		else:
			AudioManager.play_collision_sound(AudioManager.SFX.PIECE_WALL, vel)
	elif other is RigidBody3D:
		# Piece-piece collision (only trigger from one side)
		if piece.get_instance_id() < other.get_instance_id():
			AudioManager.play_collision_sound(AudioManager.SFX.PIECE_COLLISION, vel)
