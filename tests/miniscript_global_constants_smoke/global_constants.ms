// Verify Godot global constants are accessible from MiniScript.

function _ready()
	// OK == 0 in Godot error enum
	if OK == 0 then
		print "global_constant_OK_visible"
	end if
	// FAILED == 1
	if FAILED == 1 then
		print "global_constant_FAILED_visible"
	end if
	// PI is approximately 3.14
	if PI > 3.14 and PI < 3.15 then
		print "global_constant_PI_visible"
	end if
	print "global_constants_check_done"
end function
