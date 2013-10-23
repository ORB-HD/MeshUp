meshes = {
  UPPERMesh1 = {
    name = "UPPERMesh1",
    dimensions = { 0.5, 0.8, 0.4},
    color = { 0.2, 0.2, 0.8},
    src = "meshes/unit_cube.obj",
  },
  LOWERMesh = {
    name = "LOWERMesh",
    dimensions = { 0.5, 0.8, 0.8},
    color = { 0.5, 0.8, 0.5},
    src = "meshes/unit_cube.obj",
  },
}

model = {
  configuration = {
    axis_front = { -1, 0, 0 },
    axis_up    = { 0, 0, 1 },
    axis_right = { 0, 1, 0 },
    rotation_order = { 2, 1, 0},
  },

  frames = {
    {
      name = "BODY",
      parent = "ROOT",
      joint_frame = {
        r = { 0, 0, 0.5 },
      },
      visuals = {
        meshes.LOWERMesh,
      },
    },
    {
      name = "HEAD",
      parent = "ROOT",
      joint_frame = {
        r = { 0, 0, 1.1 },
      },
      visuals = {
        meshes.UPPERMesh1,
      },
    },
  }
}

return model
