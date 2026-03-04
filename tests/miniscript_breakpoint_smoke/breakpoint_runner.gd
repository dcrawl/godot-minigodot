extends Node

func _ready() -> void:
	var script_res: Script = load("res://breakpoint_test.ms")
	if script_res == null:
		print("MiniScript breakpoint smoke failed: script missing")
		get_tree().quit(1)
		return

	var probe_node := Node.new()
	probe_node.set_script(script_res)
	add_child(probe_node)

	var skip_breakpoints := EngineDebugger.is_skipping_breakpoints()

	EngineDebugger.clear_breakpoints()
	EngineDebugger.insert_breakpoint(1, "res://breakpoint_test.ms")

	var first_result: Variant = probe_node.call("probe")
	if skip_breakpoints:
		if first_result != 123:
			print("MiniScript breakpoint smoke failed: skip mode should bypass breakpoint and return 123, got ", first_result)
			get_tree().quit(2)
			return
	else:
		if first_result != null:
			print("MiniScript breakpoint smoke failed: breakpoint did not interrupt call")
			get_tree().quit(2)
			return

	EngineDebugger.remove_breakpoint(1, "res://breakpoint_test.ms")
	var second_result: Variant = probe_node.call("probe")
	if second_result != 123:
		print("MiniScript breakpoint smoke failed: call did not resume after breakpoint removal, got ", second_result)
		get_tree().quit(3)
		return

	EngineDebugger.clear_breakpoints()
	if skip_breakpoints:
		print("MiniScript breakpoint skip-mode smoke passed")
	else:
		print("MiniScript breakpoint smoke passed")
	get_tree().quit(0)
