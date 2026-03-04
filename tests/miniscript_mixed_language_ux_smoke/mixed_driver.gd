extends Node

func _ready():
	print("Mixed UX driver ready")
	var result = $MiniNode.call("double_value", 21)
	print("Mixed UX gd->ms call ", result)
