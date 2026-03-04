extends Node

func _ready() -> void:
	var script_res: Script = load("res://global_test.ms")
	if script_res == null:
		print("MiniScript global class smoke failed: script missing")
		get_tree().quit(1)
		return

	if script_res.get_global_name() != StringName("MiniSmokeClass"):
		print("MiniScript global class smoke failed: global name mismatch: ", script_res.get_global_name())
		get_tree().quit(2)
		return

	if script_res.get_instance_base_type() != StringName("Node"):
		print("MiniScript global class smoke failed: base type mismatch: ", script_res.get_instance_base_type())
		get_tree().quit(3)
		return

	if not script_res.is_tool():
		print("MiniScript global class smoke failed: expected tool script")
		get_tree().quit(4)
		return

	var node := Node.new()
	node.set_script(script_res)
	add_child(node)
	var marker: Variant = node.call("marker")
	if marker != "ok":
		print("MiniScript global class smoke failed: marker mismatch: ", marker)
		get_tree().quit(5)
		return

	print("MiniScript global class smoke passed")
	get_tree().quit(0)
