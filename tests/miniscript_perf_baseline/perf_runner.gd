extends Node

const CALLS := 500

func _ready() -> void:
    var script_res: Script = load("res://perf_target.ms")
    if script_res == null:
        print("MiniScript perf baseline failed: script missing")
        get_tree().quit(1)
        return

    var node := Node.new()
    node.set_script(script_res)
    add_child(node)

    var t0 := Time.get_ticks_usec()
    for i in range(CALLS):
        node.call("compute", i)
    var elapsed_us := Time.get_ticks_usec() - t0

    var per_call_us := float(elapsed_us) / float(CALLS)
    print("MiniScript perf baseline: calls=%d total_us=%d per_call_us=%.1f" % [CALLS, elapsed_us, per_call_us])
    get_tree().quit(0)
