model = {
  configuration = {
    axis_front = { 1, 0, 0 },
    axis_up    = { 0, 0, 1 },
    axis_right = { 0, -1, 0 },
    rotation_order = { 2, 1, 0},
  },

  frames = {
    {
      name = "FrameA",
      parent = "ROOT",
      joint_frame = {
        r = { 0, 0 , 1. },
      },
      visuals = {
				MeshA = {
					name = "MeshA",
					dimensions = { 0.25, 0.4, 0.25 },
					color = { 0.8, 0.8, 0.2},
					mesh_center = { 0, 0, 0.125 },
					src = "meshes/unit_cube.obj",
				},
      },
    },
    {
      name = "FrameB",
      parent = "FrameA",
      joint_frame = {
        r = { 0, 0 , 1. },
				E = { 
					{1., 0., 0.},
					{0., 0., -1.},
					{0., 1., 0.}
				}
      },
      visuals = {
				MeshA = {
					name = "MeshA",
					dimensions = { 0.25, 0.4, 0.25 },
					color = { 0.8, 0.8, 0.2},
					mesh_center = { 0, 0, 0.125 },
					src = "meshes/unit_cube.obj",
				},
				MeshB = {
					name = "MeshB",
					dimensions = { 0.3, 0.6, 0.7 },
					color = { 0.8, 0.8, 0.4},
					mesh_center = { 0, 0, 0.35 },
					src = "meshes/unit_cube.obj",
				},
      },
    },
	}
}

return model
