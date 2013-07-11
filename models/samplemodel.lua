meshes = {
  HipMesh1 = {
    name = "HipMesh1",
    dimensions = { 0.25, 0.4, 0.25 },
    color = { 0.8, 0.8, 0.2},
    mesh_center = { 0, 0, 0.125 },
    src = "meshes/unit_cube.obj",
  },
  UpperBody = {
    name = "UpperBody",
    dimensions = { 0.3, 0.6, 0.7 },
    color = { 0.8, 0.8, 0.4},
    mesh_center = { 0, 0, 0.35 },
    src = "meshes/unit_cube.obj",
  },
  UpperArm_L = {
    name = "UpperArm_L",
    dimensions = { 0.2, 0.2, 0.45 },
    color = { 0.1, 0.1, 0.8},
    mesh_center = { 0, 0, -0.225 },
    src = "meshes/unit_cube.obj",
  },
  LowerArm_L = {
    name = "LowerArm_L",
    dimensions = { 0.2, 0.2, 0.45 },
    color = { 0.2, 0.2, 0.9},
    mesh_center = { 0, 0, -0.225 },
    src = "meshes/unit_cube.obj",
  },
  UpperArm_R = {
    name = "UpperArm_R",
    dimensions = { 0.2, 0.2, 0.45 },
    color = { 0.8, 0.1, 0.1},
    mesh_center = { 0, 0, -0.225 },
    src = "meshes/unit_cube.obj",
  },
  LowerArm_R = {
    name = "LowerArm_R",
    dimensions = { 0.2, 0.2, 0.45 },
    color = { 0.8, 0.2, 0.2},
    mesh_center = { 0, 0, -0.225 },
    src = "meshes/unit_cube.obj",
  },
  Head = {
    name = "Head",
    dimensions = { 0.8, 1, 0.85},
    color = { 0.1, 0.7, 0.1},
    mesh_center = { 0, 0, 0.225},
    src = "meshes/monkeyhead.obj",
  },
  UpperLeg_L = {
    name = "UpperLeg_L",
    dimensions = { 0.2, 0.2, 0.45 },
    color = { 0.1, 0.1, 0.8},
    mesh_center = { 0, 0, -0.225 },
    src = "meshes/unit_cube.obj",
  },
  LowerLeg_L = {
    name = "LowerLeg_L",
    dimensions = { 0.2, 0.2, 0.45 },
    color = { 0.2, 0.2, 0.9},
    mesh_center = { 0, 0, -0.225 },
    src = "meshes/unit_cube.obj",
  },
  UpperLeg_R = {
    name = "UpperLeg_R",
    dimensions = { 0.2, 0.2, 0.45 },
    color = { 0.8, 0.1, 0.1},
    mesh_center = { 0, 0, -0.225 },
    src = "meshes/unit_cube.obj",
  },
  LowerLeg_R = {
    name = "LowerLeg_R",
    dimensions = { 0.2, 0.2, 0.45 },
    color = { 0.9, 0.2, 0.2},
    mesh_center = { 0, 0, -0.225 },
    src = "meshes/unit_cube.obj",
  },
}

model = {
  configuration = {
    axis_front = { 1, 0, 0 },
    axis_up    = { 0, 0, 1 },
    axis_right = { 0, -1, 0 },
    rotation_order = { 2, 1, 0},
  },

  frames = {
    {
      name = "HIP",
      parent = "ROOT",
      joint_frame = {
        r = { 0, 0 , 0.9 },
      },
      visuals = {
        meshes.HipMesh1,
      },
    },
    {
      name = "UPPERBODY",
      parent = "HIP",
      joint_frame = {
        r = { 0, 0 , 0.25 },
      },
      visuals = {
        meshes.UpperBody,
     },
    },
    {
      name = "UPPERARM_L",
      parent = "UPPERBODY",
      joint_frame = {
        r = { 0, 0.4 , 0.65 },
      },
      visuals = {
        meshes.UpperArm_L,
     },
    },
    {
      name = "LOWERARM_L",
      parent = "UPPERARM_L",
      joint_frame = {
        r = { 0, 0 , -0.45 },
      },
      visuals = {
        meshes.LowerArm_L,
     },
    },
    {
      name = "UPPERARM_R",
      parent = "UPPERBODY",
      joint_frame = {
        r = { 0, -0.4 , 0.65 },
      },
      visuals = {
        meshes.UpperArm_R,
     },
    },
    {
      name = "LOWERARM_R",
      parent = "UPPERARM_R",
      joint_frame = {
        r = { 0, 0 , -0.45 },
      },
      visuals = {
        meshes.LowerArm_R,
     },
    },
    {
      name = "HEAD",
      parent = "UPPERBODY",
      joint_frame = {
        r = { 0, 0 , 0.7 },
      },
      visuals = {
        meshes.Head,
     },
    },
    {
      name = "UPPERLEG_L",
      parent = "HIP",
      joint_frame = {
        r = { 0, 0.15 , 0},
      },
      visuals = {
        meshes.UpperLeg_L,
     },
    },
    {
      name = "LOWERLEG_L",
      parent = "UPPERLEG_L",
      joint_frame = {
        r = { 0, 0 , -0.45 },
      },
      visuals = {
        meshes.LowerLeg_L,
     },
    },
    {
      name = "UPPERLEG_R",
      parent = "HIP",
      joint_frame = {
        r = { 0, -0.15 , 0},
      },
      visuals = {
        meshes.UpperLeg_R,
     },
    },
    {
      name = "LOWERLEG_R",
      parent = "UPPERLEG_R",
      joint_frame = {
        r = { 0, 0 , -0.45 },
      },
      visuals = {
        meshes.LowerLeg_R,
     },
    },
  }
}

return model
