extends Node3D

## Builds the carrom board, pieces, pockets, and walls procedurally.
## Attached to the root Board node in main.tscn.
## Scale: 1 unit = 1 cm. Board is 74x74 cm.

# --- Dimensions (cm) ---
const BOARD_SIZE := 74.0
const BOARD_HEIGHT := 0.4
const BOUNDARY_HEIGHT := 2.0
const WALL_THICKNESS := 2.4
const POCKET_RADIUS := 2.8
const MEN_RADIUS := 1.8
const MEN_HEIGHT := 0.2
const STRIKER_RADIUS := 2.2

# --- Physics (grams, real carrom values) ---
const MEN_MASS := 5.0       # ~5g carrom piece
const STRIKER_MASS := 15.0   # ~15g striker (3:1 ratio)
const PIECE_LINEAR_DAMP := 0.5   # gentle surface friction (powdered board)
const PIECE_ANGULAR_DAMP := 2.0  # moderate spin resistance
const PIECE_BOUNCE := 0.8        # hard plastic = elastic collisions
const PIECE_FRICTION := 0.05     # powdered board = near-frictionless
const WALL_BOUNCE := 0.7         # rubber cushion on wood frame
const WALL_FRICTION := 0.1       # smooth lacquered wood


func _ready() -> void:
	# Clear stale refs from previous scene load (autoload persists across reloads)
	GameManager.pieces.clear()
	GameManager.striker = null
	GameManager.queen = null

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
	box.size = Vector3(BOARD_SIZE, BOARD_HEIGHT, BOARD_SIZE)
	mesh_inst.mesh = box

	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.85, 0.75, 0.45)
	mat.roughness = 0.8
	mesh_inst.material_override = mat

	var col := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = box.size
	col.shape = shape

	body.add_child(mesh_inst)
	body.add_child(col)
	body.position.y = -BOARD_HEIGHT / 2.0  # top surface at y=0
	body.collision_layer = 1
	body.collision_mask = 0
	add_child(body)


# --- Boundaries (4 walls) ---

func _build_boundaries() -> void:
	var half := BOARD_SIZE / 2.0
	var bh := BOUNDARY_HEIGHT

	var walls := [
		["WallTop",    Vector3(0, bh / 2.0, -half), Vector3(BOARD_SIZE + WALL_THICKNESS * 2, bh, WALL_THICKNESS)],
		["WallBottom", Vector3(0, bh / 2.0,  half), Vector3(BOARD_SIZE + WALL_THICKNESS * 2, bh, WALL_THICKNESS)],
		["WallLeft",   Vector3(-half, bh / 2.0, 0), Vector3(WALL_THICKNESS, bh, BOARD_SIZE + WALL_THICKNESS * 2)],
		["WallRight",  Vector3( half, bh / 2.0, 0), Vector3(WALL_THICKNESS, bh, BOARD_SIZE + WALL_THICKNESS * 2)],
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
		mat.albedo_color = Color(0.35, 0.2, 0.1)
		mat.roughness = 0.9
		mesh_inst.material_override = mat

		var col := CollisionShape3D.new()
		var shape := BoxShape3D.new()
		shape.size = w[2] as Vector3
		col.shape = shape

		body.add_child(mesh_inst)
		body.add_child(col)
		body.collision_layer = 1
		body.collision_mask = 0

		var phys_mat := PhysicsMaterial.new()
		phys_mat.bounce = WALL_BOUNCE
		phys_mat.friction = WALL_FRICTION
		body.physics_material_override = phys_mat

		add_child(body)


# --- Pockets (4 corners) ---

func _build_pockets() -> void:
	var half := BOARD_SIZE / 2.0
	var offset := half * 0.95
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
		shape.radius = POCKET_RADIUS
		col.shape = shape
		area.add_child(col)

		# Visual: dark circle
		var mesh_inst := MeshInstance3D.new()
		var cyl := CylinderMesh.new()
		cyl.top_radius = POCKET_RADIUS
		cyl.bottom_radius = POCKET_RADIUS
		cyl.height = 0.5
		mesh_inst.mesh = cyl
		var mat := StandardMaterial3D.new()
		mat.albedo_color = Color(0.05, 0.05, 0.05)
		mesh_inst.material_override = mat
		area.add_child(mesh_inst)

		area.collision_layer = 8
		area.collision_mask = 6   # pieces (2) + striker (4)
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
	torus.inner_radius = 3.5
	torus.outer_radius = 3.8
	mesh_inst.mesh = torus
	mesh_inst.position.y = 0.05
	mesh_inst.rotation.x = PI / 2
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.4, 0.2, 0.1)
	mesh_inst.material_override = mat
	add_child(mesh_inst)


# --- Board Markings (baselines + diagonals, visual only) ---

func _build_board_markings() -> void:
	var half := BOARD_SIZE / 2.0
	var baseline_z := 29.0  # matches PLACEMENT_Y
	var line_thick := 0.5
	var line_y := 0.03
	var line_color := Color(0.3, 0.2, 0.05)

	for sign_val in [1.0, -1.0]:
		var m := MeshInstance3D.new()
		var box := BoxMesh.new()
		box.size = Vector3(BOARD_SIZE * 0.55, 0.02, line_thick)
		m.mesh = box
		m.position = Vector3(0, line_y, sign_val * baseline_z)
		var mat := StandardMaterial3D.new()
		mat.albedo_color = line_color
		m.material_override = mat
		add_child(m)

	var diag_data := [
		[Vector3(-half * 0.5, line_y, -half * 0.5), 45.0],
		[Vector3( half * 0.5, line_y, -half * 0.5), -45.0],
		[Vector3(-half * 0.5, line_y,  half * 0.5), -45.0],
		[Vector3( half * 0.5, line_y,  half * 0.5), 45.0],
	]
	for dp: Array in diag_data:
		var m := MeshInstance3D.new()
		var box := BoxMesh.new()
		box.size = Vector3(half * 0.7, 0.02, line_thick)
		m.mesh = box
		m.position = dp[0] as Vector3
		m.rotation.y = deg_to_rad(float(dp[1]))
		var mat := StandardMaterial3D.new()
		mat.albedo_color = line_color
		m.material_override = mat
		add_child(m)


# --- Piece Spawning ---

func _spawn_pieces() -> void:
	var r := MEN_RADIUS  # 1.8 cm
	var d := 2.0 * r + 0.1  # spacing with small gap
	var d2 := 2.0 * r - 0.4
	var cos30 := cos(deg_to_rad(30.0))
	var sin30 := sin(deg_to_rad(30.0))

	# [position_x, position_z, color] — 0=BLACK, 1=WHITE, 2=RED
	var layout: Array = [
		[0, 0, 2],                                          # Queen (center)
		[0, d, 0],                                          # BLACK
		[d * cos30, d * sin30, 1],                          # WHITE
		[-d * cos30, d * sin30, 1],                         # WHITE
		[0, -d, 1],                                         # WHITE
		[d * cos30, -d * sin30, 0],                         # BLACK
		[-d * cos30, -d * sin30, 0],                        # BLACK
		[2 * d2 * sin30, 2 * d2 * cos30, 0],               # BLACK
		[2 * d * cos30, 2 * d * sin30, 1],                  # WHITE
		[0, 2 * d, 1],                                      # WHITE
		[2 * d2, 0, 0],                                     # BLACK
		[-2 * d2 * sin30, 2 * d2 * cos30, 0],              # BLACK
		[-2 * d * cos30, 2 * d * sin30, 1],                 # WHITE
		[-2 * d2, 0, 0],                                    # BLACK
		[2 * d2 * sin30, -2 * d2 * cos30, 0],              # BLACK
		[2 * d * cos30, -2 * d * sin30, 1],                 # WHITE
		[-2 * d2 * sin30, -2 * d2 * cos30, 0],             # BLACK
		[-2 * d * cos30, -2 * d * sin30, 1],                # WHITE
		[0, -2 * d, 1],                                     # WHITE
	]

	var colors_map := {
		0: [Color(0.1, 0.1, 0.1), GameManager.PieceColor.BLACK],
		1: [Color(0.95, 0.92, 0.85), GameManager.PieceColor.WHITE],
		2: [Color(0.85, 0.1, 0.1), GameManager.PieceColor.RED],
	}

	for i in range(layout.size()):
		var data: Array = layout[i]
		var pos_x: float = float(data[0])
		var pos_z: float = float(data[1])
		var color_idx: int = int(data[2])

		var color_data: Array = colors_map[color_idx]
		var piece := _create_piece(
			MEN_RADIUS,
			MEN_HEIGHT,
			MEN_MASS,
			color_data[0] as Color,
			color_data[1] as GameManager.PieceColor
		)
		piece.name = "Piece_%d" % i
		piece.position = Vector3(pos_x, MEN_HEIGHT * 0.5, pos_z)
		add_child(piece)
		GameManager.pieces.append(piece)

		if color_idx == 2:
			GameManager.queen = piece

	print("[BOARD] Spawned %d pieces" % GameManager.pieces.size())
	for p: RigidBody3D in GameManager.pieces:
		print("[BOARD]   %s: pos=%s freeze=%s layer=%d mask=%d mass=%.1f damp=%.1f" % [
			p.name, p.position, p.freeze, p.collision_layer, p.collision_mask,
			p.mass, p.linear_damp])


func _spawn_striker() -> void:
	var striker := _create_piece(
		STRIKER_RADIUS,
		MEN_HEIGHT,
		STRIKER_MASS,
		Color(0.05, 0.05, 0.25),
		GameManager.PieceColor.BLACK
	)
	striker.name = "Striker"
	striker.position = Vector3(0, MEN_HEIGHT * 0.5, 29.0)
	striker.freeze = true

	var striker_script: Resource = load("res://scenes/game/striker.gd")
	if striker_script:
		striker.set_script(striker_script)

	striker.collision_layer = 4  # striker layer
	striker.collision_mask = 3   # board (1) + pieces (2)
	striker.continuous_cd = true

	add_child(striker)
	GameManager.striker = striker
	print("[BOARD] Striker: pos=%s mass=%.1f damp=%.1f bounce=%.2f friction=%.2f" % [
		striker.position, striker.mass, striker.linear_damp,
		PIECE_BOUNCE, PIECE_FRICTION])


func _create_piece(radius: float, height: float, mass_val: float, color: Color, piece_color: GameManager.PieceColor) -> RigidBody3D:
	var body := RigidBody3D.new()
	body.mass = mass_val
	body.gravity_scale = 0.0
	body.linear_damp = PIECE_LINEAR_DAMP
	body.angular_damp = PIECE_ANGULAR_DAMP
	body.can_sleep = false
	body.continuous_cd = true

	body.axis_lock_linear_y = true
	body.axis_lock_angular_x = true
	body.axis_lock_angular_z = true

	# Mesh
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

	# Collision shape — sphere instead of cylinder for reliable collision detection
	# (Godot's built-in physics has poor cylinder-vs-box for thin discs)
	var col := CollisionShape3D.new()
	var shape := SphereShape3D.new()
	shape.radius = radius
	col.shape = shape
	body.add_child(col)

	# Physics material — real carrom values
	var phys_mat := PhysicsMaterial.new()
	phys_mat.bounce = PIECE_BOUNCE
	phys_mat.friction = PIECE_FRICTION
	body.physics_material_override = phys_mat

	body.collision_layer = 2
	body.collision_mask = 7   # board (1) + pieces (2) + striker (4)

	body.set_meta("color", piece_color)

	body.contact_monitor = true
	body.max_contacts_reported = 4
	body.body_entered.connect(_on_piece_collision.bind(body))

	return body


func _on_piece_collision(other: Node, piece: RigidBody3D) -> void:
	var vel := piece.linear_velocity.length()
	if vel < 5.0:  # skip inaudible collisions (cm/s)
		return

	if other is StaticBody3D:
		if piece == GameManager.striker:
			AudioManager.play_collision_sound(AudioManager.SFX.STRIKER_WALL, vel)
		else:
			AudioManager.play_collision_sound(AudioManager.SFX.PIECE_WALL, vel)
	elif other is RigidBody3D:
		if piece.get_instance_id() < other.get_instance_id():
			AudioManager.play_collision_sound(AudioManager.SFX.PIECE_COLLISION, vel)
