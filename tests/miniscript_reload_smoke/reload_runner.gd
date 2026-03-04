extends Node

const INITIAL_SOURCE := "tool\n\nfunction probe()\n    return \"v1\"\nend function\n"
const UPDATED_SOURCE := "tool\n\nfunction probe()\n    return \"v2\"\nend function\n"

func _ready() -> void:
	var script_res: Script = load("res://reload_target.ms")
	if script_res == null:
		print("MiniScript reload smoke failed: script missing")
		get_tree().quit(1)
		return

	if not script_res.is_tool():
		print("MiniScript reload smoke failed: tool mode not enabled")
		get_tree().quit(2)
		return

	var node_a := Node.new()
	node_a.set_script(script_res)
	add_child(node_a)
	var before: Variant = node_a.call("probe")
	if before != "v1":
		print("MiniScript reload smoke failed: expected v1 before reload, got ", before)
		get_tree().quit(3)
		return

	script_res.set_source_code(UPDATED_SOURCE)
	var reload_err: int = script_res.reload(false)
	if reload_err != OK:
		print("MiniScript reload smoke failed: reload returned ", reload_err)
		get_tree().quit(4)
		return

	var after_existing: Variant = node_a.call("probe")
	if after_existing != "v2":
		print("MiniScript reload smoke failed: existing instance not reloaded, got ", after_existing)
		get_tree().quit(5)
		return

	var node_b := Node.new()
	node_b.set_script(script_res)
	add_child(node_b)
	var after_new: Variant = node_b.call("probe")
	if after_new != "v2":
		print("MiniScript reload smoke failed: new instance not updated, got ", after_new)
		get_tree().quit(6)
		return

	script_res.set_source_code(INITIAL_SOURCE)
	script_res.reload(false)
	print("MiniScript reload smoke passed")
	get_tree().quit(0)
