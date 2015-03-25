/// Puppeteer Lua Scripting Module.
// @module meshup

#include "MeshupApp.h"
#include "Scripting.h"

#include "Scene.h"
#include "Animation.h"
#include "Model.h"
#include "Camera.h"

#include <errno.h>

#include <iostream>
#include <cstdio>
#include <fstream>

using namespace std;

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#define check_meshup_model(l, idx)\
	*(MeshupModel**)luaL_checkudata(l, idx, "meshup_model")

#define check_animation(l, idx)\
	*(Animation**)luaL_checkudata(l, idx, "meshup_animation")

#define check_camera(l, idx)\
	*(Camera**)luaL_checkudata(l, idx, "meshup_camera")

MeshupApp *app_ptr = NULL;

void register_functions (lua_State *L);

static void stack_print (const char *file, int line, lua_State *L) {
	int stack_top = lua_gettop(L);
	cout << file << ":" << line << ": stack size: " << lua_gettop(L) << endl;;
	for (unsigned int i = 1; i < lua_gettop(L) + 1; i++) {
		cout << file << ":" << line << ": ";
		cout << i << ": ";
		if (lua_istable (L, i))
			cout << " table" << endl;
		else if (lua_isnumber (L, i))
			cout << " number: " << lua_tonumber (L, i) << endl;
		else if (lua_isuserdata (L, i)) {
			void* userdata = (void*) lua_touserdata (L, i);
			cout << " userdata (" << userdata << ")" << endl;
		} else if (lua_isstring (L, i))
			cout << " string: " << lua_tostring(L, i) << endl;
		else if (lua_isfunction (L, i))
			cout << " function" << endl;
		else
			cout << " unknown: " << lua_typename (L, lua_type (L, i)) << endl;
	}
	assert (lua_gettop(L) == stack_top);
}
#define print_stack_types(L) stack_print (__FILE__, __LINE__, L)

void print_table (lua_State *L) {
	int stack_top = lua_gettop(L);
	
	if (!lua_istable(L, lua_gettop(L))) {
		cerr << "Error: expected table on top of stack but got " << lua_typename(L, lua_type(L, lua_gettop(L))) << endl;
		if (lua_isstring(L, lua_gettop(L))) {
			cout << " string value = " << lua_tostring(L, lua_gettop(L)) << endl;
		}
		print_stack_types (L);
		abort();
	}

	cout << "table = {" << endl;
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		cout << lua_tostring (L, -2) << ": ";
		if (lua_istable (L, -1))
			cout << " table" << endl;
		else if (lua_isnumber (L, -1))
			cout << " number: " << lua_tonumber (L, -1) << endl;
		else if (lua_isuserdata (L, -1)) {
			void* userdata = (void*) lua_touserdata (L, -1);
			cout << " userdata (" << userdata << ")" << endl;
		} else if (lua_isstring (L, -1))
			cout << " string: " << lua_tostring(L, -1) << endl;
		else if (lua_isfunction (L, -1))
			cout << " function" << endl;
		else
			cout << " unknown: " << lua_typename (L, lua_type (L, -1)) << endl;
		lua_pop(L, 1);
	}
	cout << "}" << endl;
	assert (lua_gettop(L) == stack_top);
}

static bool file_exists (const char* filename) {
	bool result = false;
	ifstream test_file (filename);
	if (test_file)
		result = true;
	
	test_file.close();

	return result;
}

template <typename T>
T* create_userdata(lua_State *L, T* ptr, const char* metatableName) {
	*(T**) lua_newuserdata(L, sizeof(void*)) = ptr;
	luaL_getmetatable(L, metatableName);
	lua_setmetatable(L, -2);
	return ptr;
}

template<typename T>
int cleanup_userdata(lua_State* L)
{
	T** ptr = reinterpret_cast<T**>(lua_touserdata(L, 1));
	(*ptr)->~T();

	return 0;
}

template<typename T>
static void push_userdata (lua_State *L, T* ptr, const char* metatableName) {
	// search for an existing userdata for this ptr
	lua_getfield(L, LUA_REGISTRYINDEX, metatableName);
	lua_pushlightuserdata(L, ptr);
	lua_gettable(L, -2);

	if (lua_isnil(L, -1)) {
		// printf("registering new entry");
		lua_pop(L, 1);

		*(T**)lua_newuserdata(L, sizeof(void*)) = ptr;
		luaL_getmetatable(L, metatableName);
		lua_setmetatable(L, -2);

		lua_pushlightuserdata(L, ptr);
		lua_pushvalue(L, -2);

		lua_settable(L, -4);
	}
	lua_remove(L, -2); // remove metatableName
}

void scripting_init (MeshupApp *app, const char* init_filename) {
	app_ptr = app;

	app->L = luaL_newstate();
	luaL_openlibs(app->L);

	lua_newtable (app->L);
	lua_setglobal(app->L, "meshup");

	register_functions (app->L);

	if (file_exists (init_filename)) {
		if (!luaL_loadfile(app->L, init_filename)) {
			lua_call (app->L, 0, 0);
		} else {
			cerr << "Error running " << init_filename << ": " << lua_tostring(app->L, lua_gettop(app->L)) << endl;
			// pop the error message and ignore it
			lua_pop (app->L, 1);
			abort();
		}
	}

	assert (lua_gettop(app->L) == 0);
}

void scripting_load (lua_State *L, const int argc, char* argv[]) {
	assert (lua_gettop(L) == 0);
	lua_getglobal (L, "meshup");
	if (lua_istable(L, 1)) {
		lua_getfield (L, 1, "load");
		if (lua_isfunction(L, 2)) {
			lua_createtable (L, argc, 0);
			for (int i = 0; i < argc; i ++) {
				lua_pushnumber (L, i + 1);

				double numvalue = 0.;
				if (sscanf (argv[i], "%lf", &numvalue) == 1) 
					lua_pushnumber (L, numvalue);
				else
					lua_pushstring (L, argv[i]);
				lua_settable (L, 3);
			}
			lua_call (L, 1, 0);
			lua_pop(L,1);
		} else {
			lua_pop(L,2);
		}
	}
	assert (lua_gettop(L) == 0);
}

/***
 * Update function that gets called every frame before it is drawn. 
 * @function meshup.update(dt)
 * @param dt the elapsed time in seconds since the last drawing update
*/
void scripting_update (lua_State *L, float dt) {
	assert (lua_gettop(L) == 0);
	assert (L);
	lua_getglobal (L, "meshup");
	if (lua_istable(L, 1)) {
		lua_getfield (L, 1, "update");
		if (lua_isfunction(L, 2)) {
			lua_pushnumber(L, dt);
			lua_call (L, 1, 0);
		} else {
			lua_pop(L,1);
		}
	}
	lua_pop (L, 1);

	assert (lua_gettop(L) == 0);
}

/***
 * Issues the scene to be drawn 
 * @function meshup.draw()
 */
void scripting_draw (lua_State *L) {
	assert (lua_gettop(L) == 0);
	lua_getglobal (L, "meshup");
	if (lua_istable(L, 1)) {
		lua_getfield (L, 1, "draw");
		if (lua_isfunction(L, 2)) {
			lua_call (L, 0, 0);
		} else {
			lua_pop(L,1);
		}
	}
	lua_pop (L, 1);
	assert (lua_gettop(L) == 0);
}

/***
 * Closes meshup
 * @function meshup.draw()
 */
void scripting_quit (lua_State *L) {
	assert (lua_gettop(L) == 0);
	lua_getglobal (L, "meshup");
	if (lua_istable(L, 1)) {
		lua_getfield (L, 1, "quit");
		if (lua_isfunction(L, 2)) {
			lua_call (L, 0, 0);
		} else {
			lua_pop(L,1);
		}
	}
	lua_pop (L, 1);
	assert (lua_gettop(L) == 0);

	app_ptr = NULL;
}

Vector3f l_checkvector3f (lua_State *L, int index) {
	luaL_checktype (L, lua_gettop(L), LUA_TTABLE);

	Vector3f result;
	for (int i = 0; i < 3; i++) {
		lua_rawgeti (L, index, i+1);
		result[i] = luaL_checknumber (L, lua_gettop(L));
		lua_pop(L, 1);
	}

	return result;
}

void l_pushvector3f (lua_State *L, const Vector3f &vec) {
	lua_createtable(L, 3, 0);

	for (int i = 0; i < 3; i++) {
		lua_pushnumber (L, vec[i]);
		lua_rawseti (L, -2, i+1);
	}
}

VectorNd l_checkvectornd (lua_State *L, int index) {
	luaL_checktype (L, index, LUA_TTABLE);
	int length = lua_objlen (L, index);

	VectorNd result (length);
	for (unsigned int i = 0; i < length; i++) {
		lua_rawgeti (L, index, i + 1);
		result[i] = luaL_checknumber (L, lua_gettop(L));
		lua_pop(L, 1);
	}

	return result;
}

//
// Camera
//

///
// @function camera.getCenter
// @param self the camera
// @return center_x, center_y, center_z
// Returns the point where the camera is looking to
static int meshup_camera_getCenter (lua_State *L) {
	Camera *camera = check_camera (L, 1);

	lua_pushnumber (L, camera->poi[0]);
	lua_pushnumber (L, camera->poi[1]);
	lua_pushnumber (L, camera->poi[2]);

	return 3;
}

///
// @function camera.setCenter
// @param self the camera
// @param center_x
// @param center_y
// @param center_z
// Sets the camera point of interest to the specified coordinates
static int meshup_camera_setCenter (lua_State *L) {
	Camera *camera = check_camera (L, 1);

	Vector3f coords;
	coords[0] = luaL_checknumber (L, 2);
	coords[1] = luaL_checknumber (L, 3);
	coords[2] = luaL_checknumber (L, 4);

	app_ptr->glWidget->setCameraPoi (coords);

	return 0;
}

///
// @function camera.getEye
// @param self the camera
// @return center_x, center_y, center_z
// Returns the coordinates of the camera
static int meshup_camera_getEye (lua_State *L) {
	Camera *camera = check_camera (L, 1);

	lua_pushnumber (L, camera->eye[0]);
	lua_pushnumber (L, camera->eye[1]);
	lua_pushnumber (L, camera->eye[2]);

	return 3;
}

///
// @function camera.setEye
// @param self the camera
// @param eye_x
// @param eye_y
// @param eye_z
// Sets the camera's location to the specified coordinates.
static int meshup_camera_setEye (lua_State *L) {
	Camera *camera = check_camera (L, 1);

	Vector3f coords;
	coords[0] = luaL_checknumber (L, 2);
	coords[1] = luaL_checknumber (L, 3);
	coords[2] = luaL_checknumber (L, 4);

	app_ptr->glWidget->setCameraEye (coords);

	return 0;
}

///
// @function camera.setOrthographic
// @param self the camera
// @param status
// Enables or disables orthographic projection of the camera
static int meshup_camera_setOrthographic (lua_State *L) {
	Camera *camera = check_camera (L, 1);

	bool status = lua_toboolean (L, 2);
	app_ptr->glWidget->toggle_draw_orthographic (status);

	return 0;
}

static const struct luaL_Reg meshup_camera_f[] = {
	{ "getCenter", meshup_camera_getCenter},
	{ "setCenter", meshup_camera_setCenter},
	{ "getEye", meshup_camera_getEye},
	{ "setEye", meshup_camera_setEye},
	{ "setOrthographic", meshup_camera_setOrthographic},
	{ NULL, NULL }
};

//
// Animation
//

///
// @function animation.getFilename 
// @param self the animation
// @return the filename of the model
static int meshup_animation_getFilename (lua_State *L) {
	Animation *animation = check_animation (L, 1);
	lua_pushstring (L, animation->animation_filename.c_str());
	return 1;
}

///
// @function animation.getRawDimensions
// @param self the animation
// @return rows, cols of the raw values
static int meshup_animation_getRawDimensions (lua_State *L) {
	Animation *animation = check_animation (L, 1);
	if (animation->raw_values.size() > 0) {
		lua_pushnumber (L, animation->raw_values.size());
		lua_pushnumber (L, animation->raw_values[0].size());
	} else {
		lua_pushnumber (L, 0.);
		lua_pushnumber (L, 0.);
	}
	return 2;
}

///
// @function animation.addValues
// @param self the animation to which the values should be added
// @param values value of the time at index 1 and state values at the
// remaining entries
// Adds new values to the animation
static int meshup_animation_addValues (lua_State *L) {
	Animation *animation = check_animation (L, 1);
	VectorNd values = l_checkvectornd (L, 2);

	if (animation->raw_values.size() > 0 && animation->raw_values[0].size() != values.size()) {
		luaL_error (L, "Invalid values for animation: expected %d values but got %d", animation->raw_values[0].size(), values.size());
	}

	app_ptr->scene->longest_animation = std::max (app_ptr->scene->longest_animation, animation->duration);

	animation->raw_values.push_back (values);

	if (animation->duration < values[0])
		animation->duration = values[0];

	return 0;
}

///
// @function animation.setValuesAt
// @param self the animation for which the values should be set
// @param row index of the row for which the new values should be set
// @param values value of the time at index 1 and state values at the
// remaining entries
// Sets the raw values at the given row
static int meshup_animation_setValuesAt (lua_State *L) {
	Animation *animation = check_animation (L, 1);
	int row = luaL_checkint (L, 2) - 1;
	VectorNd values = l_checkvectornd (L, 3);

	if (row < 0 || row >= animation->raw_values.size()) {
		luaL_error (L, "Invalid row %d", row);
	}

	if (animation->raw_values.size() > 0 && animation->raw_values[0].size() != values.size()) {
		luaL_error (L, "Invalid values for animation: expected %d values but got %d", animation->raw_values[0].size(), values.size());
	}

	animation->raw_values[row] = values;

	// TODO: properly check whether values are still ordered in time?
	if (animation->duration < values[0])
		animation->duration = values[0];
	
	return 0;
}

///
// @function animation.getValuesAt
// @param self the animation of which we want the values
// @param row index of the row for which we want to get the values
// Returns a table with all values (first entry is time)
static int meshup_animation_getValuesAt (lua_State *L) {
	Animation *animation = check_animation (L, 1);
	int row = luaL_checkint (L, 2) - 1;

	if (row < 0 || row >= animation->raw_values.size()) {
		luaL_error (L, "Invalid row %d", row);
	}

	if (animation->raw_values.size() > 0 && animation->raw_values.size() <= row) {
		luaL_error (L, "Invalid row. Requested %d but only have %d rows", row, animation->raw_values.size());
	}

	size_t num_values = animation->raw_values[row].size();
	lua_createtable (L, num_values, 0);

	for (size_t i = 0; i < num_values; i++) {
		lua_pushnumber (L, i + 1);
		lua_pushnumber (L, animation->raw_values[row][i]);
		lua_settable (L, -3);
	}

	return 1;
}

///
// @function animation.getDuration
// @param self the animation of which we want the values
// Returns the duration of the animation in seconds 
static int meshup_animation_getDuration (lua_State *L) {
	Animation *animation = check_animation (L, 1);

	double duration = animation->raw_values[animation->raw_values.size() - 1][0];
	lua_pushnumber (L, duration);
	return 1;
}

static const struct luaL_Reg meshup_animation_f[] = {
	{ "getFilename", meshup_animation_getFilename},
	{ "getRawDimensions", meshup_animation_getRawDimensions},
	{ "addValues", meshup_animation_addValues},
	{ "setValuesAt", meshup_animation_setValuesAt},
	{ "getValuesAt", meshup_animation_getValuesAt},
	{ "getDuration", meshup_animation_getDuration},
	{ NULL, NULL }
};

//
// Model
//

///
// @function model.getFilename 
// @param self the model
// @return the filename of the model
static int meshup_model_getFilename (lua_State *L) {
	MeshupModel *model = check_meshup_model (L, 1);
	lua_pushstring (L, model->model_filename.c_str());
	return 1;
}

///
// @function model.getDofCount
// @param self the model
// @return the number of degrees of freedom
static int meshup_model_getDofCount (lua_State *L) {
	MeshupModel *model = check_meshup_model (L, 1);
	if (model-> state_descriptor.states.size() > 0) 
		lua_pushnumber (L, model->state_descriptor.states.size() - 1);
	else 
		lua_pushnumber (L, 0);
	return 1;
}

static const struct luaL_Reg meshup_model_f[] = {
	{ "getFilename", meshup_model_getFilename},
	{ "getDofCount", meshup_model_getDofCount},
	{ NULL, NULL }
};

///
// @function meshup.loadModel
// @param filename
static int meshup_loadModel (lua_State *L) {
	string filename = luaL_checkstring (L, 1);

	app_ptr->loadModel (filename.c_str());	

	return 0;
}

///
// @function meshup.getCamera
// @param index
// @return model at given index or first model if no index is provided
static int meshup_getCamera (lua_State *L) {
	push_userdata (L, &(app_ptr->glWidget->camera), "meshup_camera");

	return 1;
}

///
// @function meshup.getModel
// @param index
// @return model at given index or first model if no index is provided
static int meshup_getModel (lua_State *L) {
	int index = 0;
	if (lua_gettop (L) == 1) {
		index = luaL_checkint (L, 1) - 1;
	}

	if (index < 0 || index > app_ptr->scene->models.size() - 1) {
		luaL_error (L, "Invalid model index %d. There are %d models loaded", index + 1, app_ptr->scene->models.size());
	}

	push_userdata (L, app_ptr->scene->models[index], "meshup_model");

	return 1;
}

///
// @function meshup.getModelCount
// @return number of loaded models.
static int meshup_getModelCount (lua_State *L) {
	lua_pushinteger (L, app_ptr->scene->models.size());

	return 1;
}

///
// @function meshup.getAnimation
// @param index
// @return animation at given index or first animation if no index is provided
static int meshup_getAnimation (lua_State *L) {
	int index = 0;
	if (lua_gettop (L) == 1) {
		index = luaL_checkint (L, 1) - 1;
	}

	if (index < 0 || index > app_ptr->scene->animations.size() - 1) {
		luaL_error (L, "Invalid animation index %d. There are %d animations loaded", index + 1, app_ptr->scene->animations.size());
	}

	push_userdata (L, app_ptr->scene->animations[index], "meshup_animation");

	return 1;
}

///
// @function meshup.getAnimationCount
// @return number of loaded animations
static int meshup_getAnimationCount (lua_State *L) {
	lua_pushinteger (L, app_ptr->scene->animations.size());

	return 1;
}

static int meshup_newAnimation (lua_State *L) {
	Animation *animation = new Animation();
	animation->animation_filename = "<generated by script>";
	app_ptr->scene->animations.push_back (animation);
	
	push_userdata (L, animation, "meshup_animation");
	return 1;
}

///
// @function meshup.setCurrentTime
// @param time the current time
// Sets the current time of all animations to the specified value
static int meshup_setCurrentTime (lua_State *L) {
	double time  = luaL_checknumber (L, 1);

	app_ptr->glWidget->scene->setCurrentTime (time);

	return 0;
}

///
// @function meshup.setLightPosition
// @param light_x
// @param light_y
// @param light_y
// Sets the light to the given position
static int meshup_setLightPosition (lua_State *L) {
	Vector4f coords;
	coords[0] = luaL_checknumber (L, 1);
	coords[1] = luaL_checknumber (L, 2);
	coords[2] = luaL_checknumber (L, 3);
	coords[4] = 1.;

	app_ptr->glWidget->light_position = coords;

	return 0;
}

///
// @function meshup.saveScreenshot
// @param filename
// @param width (optional)
// @param height (optional)
// @param transparency (optional) whether black should be transparent
// Makes a screenshot and stores as PNG file.
static int meshup_saveScreenshot (lua_State *L) {
	string filename = luaL_checkstring (L, 1);

	int width = app_ptr->glWidget->camera.width;
	int height = app_ptr->glWidget->camera.height;
	bool transparency = false;

	if (lua_gettop(L) > 1 && !lua_isnil(L, 2))
		width = luaL_checknumber (L, 2);

	if (lua_gettop(L) > 2 && !lua_isnil(L, 3))
		height = luaL_checknumber (L, 3);

	if (lua_gettop(L) > 3) {
		transparency = lua_toboolean (L, 4);
	}

	app_ptr->glWidget->saveScreenshot (filename.c_str(), width, height, transparency);

	return 0;
}

///
// @function meshup.setModelDisplacement
// @param x
// @param y
// @param z
// Specifies how multiple models should be displaced.
static int meshup_setModelDisplacement (lua_State *L) {
	Vector3f coords;

	coords[0] = luaL_checknumber (L, 1);
	coords[1] = luaL_checknumber (L, 2);
	coords[2] = luaL_checknumber (L, 3);

	app_ptr->scene->model_displacement = coords;

	return 0;
}

static const struct luaL_Reg meshup_f[] = {
	{ "getCamera", meshup_getCamera},
	{ "getModel", meshup_getModel},
	{ "getModelCount", meshup_getModelCount},
	{ "getAnimation", meshup_getAnimation},
	{ "getAnimationCount", meshup_getAnimationCount},
	{ "newAnimation", meshup_newAnimation},
	{ "setCurrentTime", meshup_setCurrentTime},
	{ "setLightPosition", meshup_setLightPosition},
	{ "saveScreenshot", meshup_saveScreenshot},
	{ "setModelDisplacement", meshup_setModelDisplacement},
	{ NULL, NULL}
};

void register_functions (lua_State *L){
	// create metatables for all types of objects we want to access from Lua
	luaL_newmetatable(L, "meshup_camera");
	lua_newtable(L); // index
	luaL_register(L, NULL, meshup_camera_f);
	lua_setfield(L, -2, "__index");
	lua_pop (L, 1);

	luaL_newmetatable(L, "meshup_model");
	lua_newtable(L); // index
	luaL_register(L, NULL, meshup_model_f);
	lua_setfield(L, -2, "__index");
	lua_pop (L, 1);

	luaL_newmetatable(L, "meshup_animation");
	lua_newtable(L); // index
	luaL_register(L, NULL, meshup_animation_f);
	lua_setfield(L, -2, "__index");
	lua_pop (L, 1);

	lua_getglobal(L, "meshup");
	int meshup_table = lua_gettop(L);
	luaL_register(L, NULL, meshup_f);
	lua_pop (L, 1); // meshup

	assert (lua_gettop(L) == 0);
}
