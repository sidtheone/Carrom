extends Node

## Game state machine & logic for Carrom Board 3D.
## Autoload singleton — access via GameManager.

# --- Signals ---
signal state_changed(new_state: int)
signal turn_changed(player: int)
signal score_updated(player: int, score: int)
signal piece_pocketed(piece: RigidBody3D, player: int)
signal foul_committed(player: int, reason: String)
signal game_over(winner: int)

# --- Enums ---
enum State { PLACE_STRIKER, AIM, POWER, SIMULATION }
enum PieceColor { BLACK, WHITE, RED }

# --- Constants ---
const SCALE := 0.01
const BOARD_HALF := 370.0 * SCALE  # 3.7 units
const PLACEMENT_Y := -290.0 * SCALE  # -2.9 (striker Z in Godot, board is XZ)
const PLACEMENT_X_MIN := -123.0 * SCALE
const PLACEMENT_X_MAX := 123.0 * SCALE
const MAX_AIM_ANGLE := 75.0  # degrees
const MAX_POWER := 5.0
const POWER_CHARGE_SPEED := 3.0  # units per second
const STOP_THRESHOLD := 0.005  # velocity magnitude to consider stopped
const SCORE_BLACK := 10
const SCORE_WHITE := 20
const SCORE_QUEEN := 50

# --- State ---
var current_state: State = State.PLACE_STRIKER
var current_player: int = 1  # 1 or 2
var scores: Array[int] = [0, 0]  # index 0 = player 1, index 1 = player 2
var aim_angle: float = 0.0
var power: float = 0.0
var is_charging: bool = false

# Player 1 = black pieces, Player 2 = white pieces
var pieces: Array[RigidBody3D] = []
var striker: RigidBody3D = null
var queen: RigidBody3D = null
var queen_pocketed_by: int = -1  # which player pocketed the queen
var queen_covered: bool = false  # queen must be "covered" by pocketing own piece next

var pocketed_this_turn: Array[RigidBody3D] = []
var striker_pocketed: bool = false
var own_piece_pocketed: bool = false

var game_active: bool = true


func _ready() -> void:
	set_process(true)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("reset_board"):
		get_tree().reload_current_scene()


func _process(delta: float) -> void:
	if not game_active:
		return

	match current_state:
		State.POWER:
			if is_charging:
				power = minf(power + POWER_CHARGE_SPEED * delta, MAX_POWER)
		State.SIMULATION:
			_check_simulation_complete()


func start_game() -> void:
	current_player = 1
	scores = [0, 0]
	queen_pocketed_by = -1
	queen_covered = false
	game_active = true
	_set_state(State.PLACE_STRIKER)


func _set_state(new_state: State) -> void:
	current_state = new_state
	state_changed.emit(new_state)


# --- Striker Placement ---

func place_striker_at(x_pos: float) -> void:
	if current_state != State.PLACE_STRIKER or striker == null:
		return
	var clamped_x := clampf(x_pos, PLACEMENT_X_MIN, PLACEMENT_X_MAX)
	# P1 at bottom (+Z), P2 at top (-Z) from camera's perspective
	var placement_z: float = -PLACEMENT_Y if current_player == 1 else PLACEMENT_Y
	striker.global_position = Vector3(clamped_x, 0.02, placement_z)
	striker.linear_velocity = Vector3.ZERO
	striker.angular_velocity = Vector3.ZERO
	striker.freeze = true


func confirm_placement() -> void:
	if current_state != State.PLACE_STRIKER:
		return
	_set_state(State.AIM)


# --- Aiming ---

func set_aim_angle(angle_deg: float) -> void:
	if current_state != State.AIM:
		return
	aim_angle = clampf(angle_deg, -MAX_AIM_ANGLE, MAX_AIM_ANGLE)


func confirm_aim() -> void:
	if current_state != State.AIM:
		return
	power = 0.0
	is_charging = false
	_set_state(State.POWER)


# --- Power ---


func release_power() -> void:
	print("[RELEASE] release_power() called | state=%s power=%.3f" % [State.keys()[current_state], power])
	if current_state != State.POWER:
		print("[RELEASE] REJECTED — state is %s, not POWER" % State.keys()[current_state])
		return
	is_charging = false
	AudioManager.stop_power_bar()
	_shoot_striker()


func _shoot_striker() -> void:
	if striker == null:
		print("[SHOOT] ABORTED — striker is null")
		return
	print("[SHOOT] --- _shoot_striker() ---")
	print("[SHOOT] striker.freeze=%s striker.visible=%s" % [striker.freeze, striker.visible])
	print("[SHOOT] striker.position=%s" % striker.global_position)
	print("[SHOOT] striker.mass=%s linear_damp=%s" % [striker.mass, striker.linear_damp])
	print("[SHOOT] aim_angle=%.2f power=%.3f current_player=%d" % [aim_angle, power, current_player])

	striker.freeze = false
	# P1 at Z=+2.9 shoots toward -Z (center), P2 at Z=-2.9 shoots toward +Z
	var angle_rad := deg_to_rad(aim_angle)
	var direction := Vector3(sin(angle_rad), 0.0, -cos(angle_rad))
	if current_player == 2:
		direction.z = -direction.z
	var impulse := direction * power * 2.0  # scale for feel

	print("[SHOOT] direction=%s" % direction)
	print("[SHOOT] impulse=%s (magnitude=%.3f)" % [impulse, impulse.length()])

	striker.apply_central_impulse(impulse)

	# Check velocity on next frame
	print("[SHOOT] velocity BEFORE physics step=%s" % striker.linear_velocity)

	pocketed_this_turn.clear()
	striker_pocketed = false
	own_piece_pocketed = false
	_set_state(State.SIMULATION)

	# Deferred check to see velocity after physics processes the impulse
	call_deferred("_log_post_impulse")


func _log_post_impulse() -> void:
	if striker:
		print("[SHOOT] velocity AFTER deferred=%s (magnitude=%.5f)" % [striker.linear_velocity, striker.linear_velocity.length()])
		print("[SHOOT] freeze=%s mode=%s" % [striker.freeze, striker.freeze_mode])
		print("[SHOOT] collision_layer=%d collision_mask=%d" % [striker.collision_layer, striker.collision_mask])
		print("[SHOOT] axis_lock_linear: x=%s y=%s z=%s" % [striker.axis_lock_linear_x, striker.axis_lock_linear_y, striker.axis_lock_linear_z])


# --- Simulation / Stop Detection ---

func _check_simulation_complete() -> void:
	if striker == null:
		return
	# Check all rigid bodies
	for piece in pieces:
		if piece.visible and piece.linear_velocity.length() > STOP_THRESHOLD:
			return
	if striker.linear_velocity.length() > STOP_THRESHOLD:
		return
	# All stopped
	_resolve_turn()


func _resolve_turn() -> void:
	_return_count = 0
	# Check for fouls
	if striker_pocketed:
		foul_committed.emit(current_player, "Striker pocketed!")
		_handle_foul()
		return

	# Queen covering logic
	if queen_pocketed_by == current_player and not queen_covered:
		if own_piece_pocketed:
			queen_covered = true
		else:
			# Queen not covered — return it to center, lose turn
			foul_committed.emit(current_player, "Queen not covered!")
			_return_queen_to_center()
			queen_pocketed_by = -1
			_switch_turn()
			return

	# Return any opponent pieces pocketed this turn
	_return_opponent_pieces()

	# Check win condition
	if _check_win():
		return

	# If player pocketed own piece, they get another turn
	if own_piece_pocketed:
		_set_state(State.PLACE_STRIKER)
	else:
		_switch_turn()


func _handle_foul() -> void:
	# Reset striker to baseline
	var next_player := 2 if current_player == 1 else 1
	var placement_z: float = -PLACEMENT_Y if next_player == 1 else PLACEMENT_Y
	striker.global_position = Vector3(0, 0.02, placement_z)
	striker.linear_velocity = Vector3.ZERO
	striker.angular_velocity = Vector3.ZERO
	striker.freeze = true
	striker.visible = true
	striker_pocketed = false

	# Penalty: return one pocketed piece of current player to center
	var player_color := PieceColor.BLACK if current_player == 1 else PieceColor.WHITE
	for piece in pieces:
		var pc: int = piece.get_meta("color", PieceColor.BLACK) as int
		if pc == player_color and not piece.visible:
			_return_piece_to_center(piece)
			break

	_switch_turn()


var _return_count: int = 0

func _return_piece_to_center(piece: RigidBody3D) -> void:
	# Spread returned pieces in a ring to avoid overlap and drift
	var angle := _return_count * PI * 2.0 / 3.0 + randf_range(-0.3, 0.3)
	var dist := 0.2 + _return_count * 0.15
	var offset := Vector3(cos(angle) * dist, 0.0, sin(angle) * dist)
	piece.global_position = Vector3(0, 0.02, 0) + offset
	piece.linear_velocity = Vector3.ZERO
	piece.angular_velocity = Vector3.ZERO
	piece.visible = true
	piece.freeze = false
	_return_count += 1


func _return_opponent_pieces() -> void:
	var opponent_color := PieceColor.WHITE if current_player == 1 else PieceColor.BLACK
	for piece in pocketed_this_turn:
		var pc: int = piece.get_meta("color", PieceColor.BLACK) as int
		if pc == opponent_color:
			_return_piece_to_center(piece)
			var points := SCORE_WHITE if pc == PieceColor.WHITE else SCORE_BLACK
			scores[current_player - 1] -= points
			score_updated.emit(current_player, scores[current_player - 1])


func _switch_turn() -> void:
	current_player = 2 if current_player == 1 else 1
	turn_changed.emit(current_player)
	_set_state(State.PLACE_STRIKER)


func _return_queen_to_center() -> void:
	if queen != null and not queen.visible:
		queen.global_position = Vector3(0, 0.02, 0)
		queen.linear_velocity = Vector3.ZERO
		queen.angular_velocity = Vector3.ZERO
		queen.visible = true
		queen.freeze = false


func _check_win() -> int:
	# Player wins when all their pieces are pocketed AND queen is covered
	var black_remaining := 0
	var white_remaining := 0
	for piece in pieces:
		if not piece.visible:
			continue
		if piece == queen:
			continue
		var pc: int = piece.get_meta("color", PieceColor.BLACK) as int
		if pc == PieceColor.BLACK:
			black_remaining += 1
		elif pc == PieceColor.WHITE:
			white_remaining += 1

	# Player 1 = black, Player 2 = white
	if black_remaining == 0 and queen_covered and queen_pocketed_by == 1:
		_end_game(1)
		return 1
	if white_remaining == 0 and queen_covered and queen_pocketed_by == 2:
		_end_game(2)
		return 2
	return 0


func _end_game(winner: int) -> void:
	game_active = false
	game_over.emit(winner)


# --- Pocket Events (called by pocket Area3D) ---

func on_piece_pocketed(body: RigidBody3D) -> void:
	if not game_active:
		return

	AudioManager.play_sound(AudioManager.SFX.POT, 0.7)

	if body == striker:
		striker_pocketed = true
		body.linear_velocity = Vector3.ZERO
		body.angular_velocity = Vector3.ZERO
		body.freeze = true
		body.visible = false
		body.global_position = Vector3(0, -10, 0)
		return

	body.visible = false
	body.freeze = true
	body.global_position = Vector3(0, -10, 0)  # move off-board
	pocketed_this_turn.append(body)

	var piece_color: int = body.get_meta("color", PieceColor.BLACK) as int

	# Scoring
	var points := 0
	match piece_color:
		PieceColor.BLACK:
			points = SCORE_BLACK
		PieceColor.WHITE:
			points = SCORE_WHITE
		PieceColor.RED:
			points = SCORE_QUEEN

	scores[current_player - 1] += points
	score_updated.emit(current_player, scores[current_player - 1])
	piece_pocketed.emit(body, current_player)

	# Track if own piece pocketed
	if (current_player == 1 and piece_color == PieceColor.BLACK) or \
	   (current_player == 2 and piece_color == PieceColor.WHITE):
		own_piece_pocketed = true

	# Queen tracking
	if piece_color == PieceColor.RED:
		queen_pocketed_by = current_player
		queen_covered = false
