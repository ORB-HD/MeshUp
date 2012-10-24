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
				HipMesh = {
					name = "HipMesh",
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
        r = { 0, 0 , 0.25 },
				E = {
					1., 0., 0.,
					0., 1., 0.,
					0., 0., 1.
				}
      },
      visuals = {
				TorsoMesh = {
					name = "TorsoMesh",
					dimensions = { 0.4, 0.6, 0.6 },
					color = { 0.8, 0.8, 0.4},
					mesh_center = { 0, 0, 0.35 },
					src = "meshes/unit_cube.obj",
				},
				HeadMesh = {
					name = "HeadMesh",
					dimensions = { 0.4, 0.4, 0.4 },
					color = { 0.1, 1.0, 0.2},
					mesh_center = { 0, 0, 1.0 },
					src = "meshes/unit_cube.obj",
				},
      },
    },
	}
}

return model
