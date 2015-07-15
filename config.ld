title = "MeshUp Lua API"
project = "MeshUp"
dir = "doc/scripting"

not_luadoc = true
add_language_extension(".cc", "c")

examples = {
	"doc/scripting/examplescript.lua",
	"doc/scripting/frontal_montage.lua",
	"doc/scripting/lateral_montage.lua",
}

file = { "src", exclude = {
		"src/Animation.cc",
		"src/Animation.h",
		"src/Camera.cc",
		"src/Camera.h",
		"src/Curve.cc",
		"src/Curve.h",
		"src/FrameConfig.h",
		"src/MeshVBO.cc",
		"src/MeshVBO.h",
		"src/MeshupApp.cc",
		"src/MeshupApp.h",
		"src/Model.cc",
		"src/Model.h",
		"src/StateDescriptor.h",
		"src/RenderImageDialog.h",
		"src/RenderImageSeriesDialog.h",
		"src/Scene.cc",
		"src/Scene.h",
		"src/colorscale.h",
		"src/glwidget.cc",
		"src/glwidget.h",
		"src/main.cc",
		"src/string_utils.h",
		"src/timer.h",
		"src/luatables/",
		"src/SimpleMath/",
		"src/ui/",
}}

format = "markdown"
