extends Camera3D

## Camera system with presets for each game state.
## Smooth transitions via Tween.

# Camera presets: [position, rotation_degrees]
# Matches original C++ camera states
const PRESETS := {
	# PLACE_STRIKER: top-down orthographic-like view
	GameManager.State.PLACE_STRIKER: {
		"position": Vector3(0, 8.0, 0.5),
		"rotation": Vector3(-85, 0, 0),
	},
	# AIM: slightly angled behind striker (player 1 side)
	GameManager.State.AIM: {
		"position": Vector3(0, 5.0, 4.5),
		"rotation": Vector3(-50, 0, 0),
	},
	# POWER: same as aim
	GameManager.State.POWER: {
		"position": Vector3(0, 5.0, 4.5),
		"rotation": Vector3(-50, 0, 0),
	},
	# SIMULATION: wide overhead
	GameManager.State.SIMULATION: {
		"position": Vector3(0, 7.0, 2.0),
		"rotation": Vector3(-70, 0, 0),
	},
}

# Flipped versions for player 2
const PRESETS_P2 := {
	GameManager.State.PLACE_STRIKER: {
		"position": Vector3(0, 8.0, -0.5),
		"rotation": Vector3(-85, 180, 0),
	},
	GameManager.State.AIM: {
		"position": Vector3(0, 5.0, -4.5),
		"rotation": Vector3(-50, 180, 0),
	},
	GameManager.State.POWER: {
		"position": Vector3(0, 5.0, -4.5),
		"rotation": Vector3(-50, 180, 0),
	},
	GameManager.State.SIMULATION: {
		"position": Vector3(0, 7.0, -2.0),
		"rotation": Vector3(-70, 180, 0),
	},
}

var _tween: Tween = null
const TRANSITION_DURATION := 0.5


func _ready() -> void:
	GameManager.state_changed.connect(_on_state_changed)
	GameManager.turn_changed.connect(_on_turn_changed)
	# Set initial position
	_apply_preset_instant(GameManager.State.PLACE_STRIKER, 1)


func _on_state_changed(new_state: int) -> void:
	_transition_to(new_state, GameManager.current_player)


func _on_turn_changed(_player: int) -> void:
	# Camera will update on next state_changed
	pass


func _transition_to(state: int, player: int) -> void:
	var presets: Dictionary = PRESETS if player == 1 else PRESETS_P2
	if not presets.has(state):
		return

	var preset: Dictionary = presets[state]
	var target_pos: Vector3 = preset["position"]
	var target_rot: Vector3 = preset["rotation"]

	if _tween:
		_tween.kill()

	_tween = create_tween()
	_tween.set_ease(Tween.EASE_IN_OUT)
	_tween.set_trans(Tween.TRANS_CUBIC)
	_tween.set_parallel(true)
	_tween.tween_property(self, "position", target_pos, TRANSITION_DURATION)
	_tween.tween_property(self, "rotation_degrees", target_rot, TRANSITION_DURATION)


func _apply_preset_instant(state: int, player: int) -> void:
	var presets: Dictionary = PRESETS if player == 1 else PRESETS_P2
	if not presets.has(state):
		return
	var preset: Dictionary = presets[state]
	position = preset["position"] as Vector3
	rotation_degrees = preset["rotation"] as Vector3
