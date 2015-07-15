function animation_set_zero_translation (animation)
	local rows, cols = animation:getRawDimensions()

	for i=1,rows do
		local values = animation:getRawValuesAt (i)
		values[2] = 0.
		animation:setRawValuesAt(i, values)
	end
end

function animation_find_center (animation)
	local rows, cols = animation:getRawDimensions()

	local s_values = animation:getRawValuesAt (1)
	local s_x, s_y, s_z = s_values[2], s_values[3], s_values[4]

	local e_values = animation:getRawValuesAt (rows)
	local e_x, e_y, e_z = e_values[2], e_values[3], e_values[4]

	return 0.5 * (e_x + s_x), 0.5 * (e_z + s_z), 0.5 * s_y + 0.5 * e_y
end

function meshup.load(args)
	local outname = "series"

	if #args == 1 then
		outname = args[1]
	end

	--	meshup.setLightPosition (0., 3., 4.)
	meshup.setModelDisplacement (-1., 0., 0.)
	
	local camera = meshup.getCamera()
	camera:setOrthographic (true)

	local anim_count = meshup.getAnimationCount()
	assert (anim_count == 1)
	local anim = meshup.getAnimation (1)

	cx, cy, cz = animation_find_center (anim)
	cx = cx - 0.9
	camera:setCenter (cx, cy - 0.1, cz)
	camera:setEye (cx + 4, cy + 0.75, cz + 2.)

	local duration = anim:getDuration()
	local num_frames = 8
	local dt = duration / (num_frames - 1)
	local t = 0.

	for f=0, num_frames - 1 do
		t = dt * f
		
		meshup.setCurrentTime (t)
		meshup.saveScreenshot (string.format ("%s-%02d.png", outname, f), 300, 500, true)
	end

	os.execute (string.format ("montage %s-*.png -geometry +0+0 -tile %dx1 -background none montage_%s_perspective.png", outname, num_frames, outname))
	os.execute (string.format ("rm %s-*.png", outname))
	os.exit(0)
end


