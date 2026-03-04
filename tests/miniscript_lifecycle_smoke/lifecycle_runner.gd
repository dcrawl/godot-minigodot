extends Node

func _ready() -> void:
	var script_res: Script = load("res://lifecycle.ms")
	if script_res == null:
		print("MiniScript lifecycle smoke failed: script missing")
		get_tree().quit(1)
		return

	var lifecycle_node := Node.new()
	lifecycle_node.set_script(script_res)

	lifecycle_node.notification(Node.NOTIFICATION_ENTER_TREE)
	lifecycle_node.notification(Node.NOTIFICATION_READY)
	lifecycle_node.notification(Node.NOTIFICATION_PROCESS)
	lifecycle_node.notification(Node.NOTIFICATION_PHYSICS_PROCESS)
	lifecycle_node.notification(Node.NOTIFICATION_EXIT_TREE)

	print("MiniScript lifecycle smoke passed")
	get_tree().quit(0)
