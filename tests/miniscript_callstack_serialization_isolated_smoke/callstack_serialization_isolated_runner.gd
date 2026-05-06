extends Node

var _captured_file := ""
var _captured_func := ""
var _captured_line := -1

func _ready() -> void:
	var script_res: Script = load("res://callstack_serialization_target.ms")
	if script_res == null:
		print("MiniScript isolated callstack-serialization smoke failed: script missing")
		get_tree().quit(1)
		return

	var probe_node := Node.new()
	probe_node.set_script(script_res)
	add_child(probe_node)

	probe_node.connect("step_signal", _on_step_signal)

	var result: Variant = probe_node.call("probe")

	if _captured_file == "" or _captured_func == "":
		print("MiniScript isolated callstack-serialization smoke failed: no MiniScript backtrace frame captured during emit")
		get_tree().quit(2)
		return

	print("MiniScript isolated callstack-serialization frame file: ", _captured_file)
	print("MiniScript isolated callstack-serialization frame func: ", _captured_func)
	print("MiniScript isolated callstack-serialization frame line: ", _captured_line)

	if result != 99:
		print("MiniScript isolated callstack-serialization smoke failed: expected probe() return 99, got ", result)
		get_tree().quit(3)
		return

	print("MiniScript isolated callstack-serialization smoke passed")
	get_tree().quit(0)

func _on_step_signal() -> void:
	var backtraces: Array = Engine.capture_script_backtraces()
	for bt in backtraces:
		if bt.get_language_name() == "MiniScript" and not bt.is_empty():
			for i in range(bt.get_frame_count()):
				if bt.get_frame_function(i) == "probe":
					_captured_file = bt.get_frame_file(i)
					_captured_func = bt.get_frame_function(i)
					_captured_line = bt.get_frame_line(i)
					return
