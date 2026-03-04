extends Node

func _ready() -> void:
	var script_res: Script = load("res://step_over_target.ms")
	if script_res == null:
		print("MiniScript isolated step-over smoke failed: script missing")
		get_tree().quit(1)
		return

	var probe_node := Node.new()
	probe_node.set_script(script_res)
	add_child(probe_node)

	var result: Variant = probe_node.call("probe")
	if result == null:
		print("MiniScript isolated step-over interrupt observed")
		get_tree().quit(0)
		return

	if result != 123:
		print("MiniScript isolated step-over smoke failed: expected 123, got ", result)
		get_tree().quit(2)
		return

	print("MiniScript isolated step-over normal completion observed")
	get_tree().quit(0)