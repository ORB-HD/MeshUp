#ifndef MESHUP_SCRIPTING_H
#define MESHUP_SCRIPTING_H

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

class MeshupApp;

void scripting_init (MeshupApp *app, const char* init_filename);

void scripting_load (lua_State *L, const int argc, char* argv[]);

void scripting_update (lua_State *L, float dt);

void scripting_draw (lua_State *L);

void scripting_quit (lua_State *L);

/* _SCRIPTING_H */
#endif
