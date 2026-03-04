extends Node

const DEFAULT_CYCLES := 8

func _ready() -> void:
	var script_res: Script = load("res://pause_continue_stability_target.ms")
	if script_res == null:
		print("MiniScript isolated pause-continue smoke failed: script missing")
		get_tree().quit(1)
		return

	var probe_node := Node.new()
	probe_node.set_script(script_res)
	add_child(probe_node)

	var cycles := int(OS.get_environment("MINISCRIPT_STABILITY_CYCLES"))
	if cycles <= 0:
		cycles = DEFAULT_CYCLES

	var expect_interrupt := OS.get_environment("MINISCRIPT_EXPECT_INTERRUPT") == "1"

	if expect_interrupt:
		var interrupt_count := 0
		for idx in range(cycles):
			var result: Variant = probe_node.call("step_point")
			if result != null:
				print("MiniScript isolated pause-continue smoke failed: expected interrupt/null at cycle ", idx + 1, ", got ", result)
				get_tree().quit(2)
				return
			interrupt_count += 1

		print("MiniScript isolated pause-continue interrupt stability observed: ", interrupt_count)
		get_tree().quit(0)
		return

	for idx in range(cycles):
		var result: Variant = probe_node.call("step_point")
		if result != 1:
			print("MiniScript isolated pause-continue smoke failed: expected 1 at cycle ", idx + 1, ", got ", result)
			get_tree().quit(3)
			return

	print("MiniScript isolated pause-continue normal completion observed: ", cycles)
	get_tree().quit(0)
