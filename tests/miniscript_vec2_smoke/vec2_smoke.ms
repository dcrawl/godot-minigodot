// Verify Vector2, Vector3, Color bridge between Godot and MiniScript.

function _ready()
	// vec2() constructor
	v = vec2(3, 4)
	if v.x == 3 and v.y == 4 then
		print "vec2_constructor_ok"
	end if

	// vec3() constructor
	u = vec3(1, 2, 3)
	if u.x == 1 and u.y == 2 and u.z == 3 then
		print "vec3_constructor_ok"
	end if

	// color() constructor
	c = color(1, 0, 0, 1)
	if c.r == 1 and c.g == 0 and c.b == 0 and c.a == 1 then
		print "color_constructor_ok"
	end if

	// Default args
	v0 = vec2
	if v0.x == 0 and v0.y == 0 then
		print "vec2_defaults_ok"
	end if

	print "vec2_check_done"
end function
