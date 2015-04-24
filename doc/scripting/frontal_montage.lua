--[[ Render an image series to a grid in frontal view
--
-- Run this using meshup model.lua animation.lua -s [outname="series"]
--
-- Renders the given models and animations to an image which contains a
-- series of snapshots of the animations from a frontal view. The resulting
-- file will be called "montage_[outname]_frontal.png".
--
-- This assembles the images using the montage tool of the package
-- imagemagick, which therefore has to be installed for this script to work
-- properly.
--]]
function animation_set_zero_translation (animation)
	local rows, cols = animation:getRawDimensions()

	for i=1,rows do
		local values = animation:getValuesAt (i)
		values[2] = 0.
		animation:setValuesAt(i, values)
	end
end

function meshup.load(args)
	local outname = "series"

	if #args == 1 then
		outname = args[1]
	end

	--	meshup.setLightPosition (0., 3., 4.)
	meshup.setModelDisplacement (-1., 0., 0.)
	
	local camera = meshup.getCamera()
	camera:setCenter (-0., 0.89, 0.)
	camera:setEye (4.2, 0.89, 0.)
	camera:setOrthographic (true)

	local anim_count = meshup.getAnimationCount()
	local duration = 0.
	for i=1,anim_count do
		local anim = meshup.getAnimation (i)
		local d = anim:getDuration()
		if d > duration then
			duration = d
		end
		animation_set_zero_translation (anim)
	end

	local num_frames = 8
	local dt = duration / (num_frames - 1)
	local t = 0.

	for f=0, num_frames - 1 do
		t = dt * f
		
		meshup.setCurrentTime (t)
		meshup.saveScreenshot (string.format ("%s-%02d.png", outname, f), 300, 500, true)
	end

	os.execute (string.format ("montage %s-*.png -geometry +0+0 -tile %dx1 -background none montage_%s_frontal.png", outname, num_frames, outname))
	os.execute (string.format ("rm %s-*.png", outname))
	os.exit(0)
end


