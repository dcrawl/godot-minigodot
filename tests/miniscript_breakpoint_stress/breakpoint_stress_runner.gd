extends Node

const STRESS_CALLS := 50

func _ready() -> void:
	var script_res: Script = load("res://breakpoint_stress.ms")
	if script_res == null:
		print("MiniScript breakpoint stress failed: script missing")
		get_tree().quit(1)
		return

	var probe_node := Node.new()
	probe_node.set_script(script_res)
	add_child(probe_node)

	EngineDebugger.clear_breakpoints()
	EngineDebugger.insert_breakpoint(1, "res://breakpoint_stress.ms")

	var first_result: Variant = probe_node.call("probe")
	if first_result != null:
		print("MiniScript breakpoint stress failed: initial breakpoint did not interrupt call")
		get_tree().quit(2)
		return

	EngineDebugger.remove_breakpoint(1, "res://breakpoint_stress.ms")

	for i in range(STRESS_CALLS):
		var value: Variant = probe_node.call("probe")
		if value != 123:
			print("MiniScript breakpoint stress failed: breakpoint re-hit or wrong return after removal at call ", i + 1, ", got ", value)
			get_tree().quit(3)
			return

	EngineDebugger.clear_breakpoints()
	print("MiniScript breakpoint stress smoke passed (post-remove calls=", STRESS_CALLS, ")")
	get_tree().quit(0)