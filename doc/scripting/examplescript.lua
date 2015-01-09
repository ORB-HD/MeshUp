local dof_count = 0
local animation = {}
local model = {}
local current_time = 0

function meshup.load (args)
	-- arguments that were passed after -s <scriptfile.lua>
	for k,v in ipairs (args) do
		print (k,v)
	end

	print ("Load: There are ", meshup.getModelCount(), " models loaded")
	print ("Load: There are ", meshup.getAnimationCount(), " animations loaded")

	model = meshup.getModel()
	dof_count = model:getDofCount()

	print (string.format ("model has %d dofs", dof_count))

	--[[ 
	-- create a new animation from script and initialize all values with 0.
	-- Note that the first entry of values corresponds to the time of the
	-- keyframe. We therefore need dof_count + 1 values.
	--]]
	animation = meshup.newAnimation ()
	local animation_values = {}
	for i=1,dof_count + 1 do
		animation_values[i] = 0.
	end
	animation:addValues (animation_values)

	r,c = animation:getRawDimensions()
	print (string.format ("animation %s has size %d, %d", animation:getFilename(), r, c))

	print (string.format ("there are %d models and %d animations loaded", meshup.getModelCount(), meshup.getAnimationCount()))
end

function meshup.update (dt)
	current_time = current_time + dt

	--[[
	-- Here we simply overwrite all values at the first entry of the animation
	-- The time value is still 0 and we only overwrite values of index > 4 with
	-- some sine values just for demonstration purposes
	--]]
	local animation_values = {}
	for i=1,dof_count + 1 do
		if i > 4 then
			animation_values[i] = math.sin (current_time * 0.01) * 180. / math.pi
		else
			animation_values[i] = 0.
		end
	end

	animation:setValuesAt (1, animation_values)
end
