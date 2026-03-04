extends Node

func _ready() -> void:
	var script_res: Script = load("res://callstack_target.ms")
	if script_res == null:
		print("MiniScript isolated call-stack smoke failed: script missing")
		get_tree().quit(1)
		return

	var probe_node := Node.new()
	probe_node.set_script(script_res)
	add_child(probe_node)

	var expect_interrupt := OS.get_environment("MINISCRIPT_EXPECT_INTERRUPT") == "1"
	var result: Variant = probe_node.call("outer")

	if expect_interrupt:
		if result != null:
			print("MiniScript isolated call-stack smoke failed: expected interrupt/null, got ", result)
			get_tree().quit(2)
			return
		print("MiniScript isolated call-stack interrupt observed")
		get_tree().quit(0)
		return

	if result != 3:
		print("MiniScript isolated call-stack smoke failed: expected 3, got ", result)
		get_tree().quit(3)
		return

	print("MiniScript isolated call-stack normal completion observed")
	get_tree().quit(0)