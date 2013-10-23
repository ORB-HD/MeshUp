meshes = {
  HipMesh1 = {
    name = "HipMesh1",
    dimensions = { 0.25, 0.4, 0.25},
    color = { 0.8, 0.8, 0.2},
    mesh_center = { 0, 0, 0.125},
    src = "meshes/unit_cube.obj",
  },
  UpperBody = {
    name = "UpperBody",
    dimensions = { 0.3, 0.6, 0.7},
    color = { 0.8, 0.8, 0.4},
    mesh_center = { 0, 0, 0.35},
    src = "meshes/unit_cube.obj",
  },
}

model = {
  configuration = {
    axis_front = { 1, 0, 0 },
    axis_up    = { 0, 0, 1 },
    axis_right = { 0, -1, 0 },
    rotation_order = { 1, 2, 0},
  },

  frames = {
    {
      name = "HIP",
      parent = "ROOT",
      visuals = {
        meshes.HipMesh1,
      },
    },
    {
      name = "UPPERBODY",
      parent = "HIP",
      joint_frame = {
        r = { 0, 0, 0.25 },
      },
      visuals = {
        meshes.UpperBody,
      },
    },
  }
}

return model
