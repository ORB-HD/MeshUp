/*
 * LuaTables++
 * Copyright (c) 2013-2014 Martin Felis <martin@fyxs.org>.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "luatables.h"

#include <assert.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <cmath>

extern "C"
{
   #include <lua.h>
   #include <lauxlib.h>
   #include <lualib.h>
}

#include <stdio.h>  /* defines FILENAME_MAX */
#ifdef WINDOWS
    #include <direct.h>
    #define get_current_dir _getcwd
#else
    #include <unistd.h>
    #define get_current_dir getcwd
 #endif

#if defined(WIN32) || defined (_WIN32)
#define DIRECTORY_SEPARATOR "\\"
#elif defined(linux) || defined (__linux) || defined(__linux__) || defined(__APPLE__)
#define DIRECTORY_SEPARATOR "/"
#else
#error Platform not supported!
#endif

using namespace std;

std::string get_file_directory (const char* filename) {
	string name (filename);
	string result = name.substr(0, name.find_last_of (DIRECTORY_SEPARATOR) + 1);

	if (result == "")
		result = "./";
#if defined (WIN32) || defined (_WIN32)
#warning get_file_directory() not yet tested under Windows!
	else if (result.substr(1,2) != ":\\")
		result = string(".\\") + result;
#else
	else if (result.substr(0,string(DIRECTORY_SEPARATOR).size()) != DIRECTORY_SEPARATOR  && result[0] != '.')
		result = string("./") + result;
#endif

	return result;
}

const char* serialize_std = "function serialize (o, tabs)\n\
  local result = \"\"\n\
  \n\
  if tabs == nil then\n\
    tabs = \"\"\n\
  end\n\
\n\
  if type(o) == \"number\" then\n\
    result = result .. tostring(o)\n\
  elseif type(o) == \"boolean\" then\n\
    result = result .. tostring(o)\n\
  elseif type(o) == \"string\" then\n\
    result = result .. string.format(\"%q\", o)\n\
  elseif type(o) == \"table\" then\n\
    if o.dont_serialize_me then\n\
      return \"{}\"\n\
    end\n\
    result = result .. \"{\\n\"\n\
    for k,v in pairs(o) do\n\
      if type(v) ~= \"function\" then\n\
        -- make sure that numbered keys are properly are indexified\n\
        if type(k) == \"number\" then\n\
				  if type(v) == \"number\" then\n\
	          result = result .. \" \" .. tostring(v) .. \",\"\n\
					else\n\
	          result = result .. tabs .. serialize(v, tabs .. \"  \") .. \",\\n\"\n\
					end\n\
        else\n\
          result = result .. tabs .. \"  \" .. k .. \" = \" .. serialize(v, tabs .. \"  \") .. \",\\n\"\n\
        end\n\
      end\n\
    end\n\
    result = result .. tabs .. \"}\"\n\
  else\n\
    print (\"ignoring stuff\" .. type(o) )\n\
  end\n\
  return result\n\
end\n\
\n\
return serialize";

//
// Lua Helper Functions
//
void bail(lua_State *L, const char *msg){
	std::cerr << msg << lua_tostring(L, -1) << endl;
	abort();
}

void stack_print (const char *file, int line, lua_State *L) {
	cout << file << ":" << line << ": stack size: " << lua_gettop(L) << endl;;
	for (int i = 1; i < lua_gettop(L) + 1; i++) {
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
		else if (lua_isnil (L, i))
			cout << " nil" << endl;
		else
			cout << " unknown: " << lua_typename (L, lua_type (L, i)) << endl;
	}
}

void l_push_LuaKey (lua_State *L, const LuaKey &key) {
	if (key.type == LuaKey::Integer)
		lua_pushnumber (L, key.int_value);
	else
		lua_pushstring(L, key.string_value.c_str());
}

bool query_key_stack (lua_State *L, std::vector<LuaKey> key_stack) {
	for (int i = key_stack.size() - 1; i >= 0; i--) {
		// get the global value when the result of a lua expression was not
		// pushed onto the stack via the return statement.
		if (lua_gettop(L) == 0) {
			lua_getglobal (L, key_stack[key_stack.size() - 1].string_value.c_str());

			if (lua_isnil(L, -1)) {
				return false;
			}

			continue;
		}

		l_push_LuaKey (L, key_stack[i]);

		lua_gettable (L, -2);

		// return if key is not found
		if (lua_isnil(L, -1)) {
			return false;
		}
	}

	return true;
}

void create_key_stack (lua_State *L, std::vector<LuaKey> key_stack) {
	for (int i = key_stack.size() - 1; i > 0; i--) {
		// get the global value when the result of a lua expression was not
		// pushed onto the stack via the return statement.
		if (lua_gettop(L) == 0) {
			lua_getglobal (L, key_stack[key_stack.size() - 1].string_value.c_str());

			if (lua_isnil(L, -1)) {
				lua_pop(L, 1);
				lua_newtable(L);
				lua_pushvalue(L, -1);
				lua_setglobal(L, key_stack[key_stack.size() - 1].string_value.c_str());
			}

			continue;
		}

		l_push_LuaKey (L, key_stack[i]);

		lua_pushvalue (L, -1);
		lua_gettable (L, -3);

		if (lua_isnil(L, -1)) {
			// parent, key, nil
			lua_pop(L, 1);  // parent, key
			lua_newtable(L); // parent, key, table
			lua_insert(L, -2); // parent, table, key
			lua_pushvalue(L, -2); // parent, table, key, table
			lua_settable (L, -4); // parent, table
		}
	}
}

//
// LuaTableNode
//
std::vector<LuaKey> LuaTableNode::getKeyStack() {
	std::vector<LuaKey> result;

	const LuaTableNode *node_ptr = this;

	do {
		result.push_back (node_ptr->key);
		node_ptr = node_ptr->parent;
	} while (node_ptr != NULL);

	return result;	
}

std::string LuaTableNode::keyStackToString() {
	std::vector<LuaKey> key_stack = getKeyStack();

	ostringstream result_stream ("");
	for (int i = key_stack.size() - 1; i >= 0; i--) {
		if (key_stack[i].type == LuaKey::String)
			result_stream << "[\"" << key_stack[i].string_value << "\"]";
		else 
			result_stream << "[" << key_stack[i].int_value << "]";
	}

	return result_stream.str();
}

bool LuaTableNode::stackQueryValue() {
	lua_State *L = luaTable->L;
	stackTop = lua_gettop(L);

	std::vector<LuaKey> key_stack = getKeyStack();

	return query_key_stack (L, key_stack);
}

void LuaTableNode::stackCreateValue() {
	lua_State *L = luaTable->L;
	stackTop = lua_gettop(L);

	std::vector<LuaKey> key_stack = getKeyStack();

	create_key_stack (L, key_stack);
}

LuaTable LuaTableNode::stackQueryTable() {
	lua_State *L = luaTable->L;
	stackTop = lua_gettop(L);

	std::vector<LuaKey> key_stack = getKeyStack();

	if (!query_key_stack (L, key_stack)) {
		std::cerr << "Error: could not query table " << key << "." << std::endl;
		abort();
	}

	return LuaTable::fromLuaState (L);
}

LuaTable LuaTableNode::stackCreateLuaTable() {
	lua_State *L = luaTable->L;
	stackTop = lua_gettop(L);

	std::vector<LuaKey> key_stack = getKeyStack();

	create_key_stack (L, key_stack);

	// create new table for the CustomType
	lua_newtable(luaTable->L);	// parent, CustomTable
	// add table of CustomType to the parent
	stackPushKey(); // parent, CustomTable, key
	lua_pushvalue(luaTable->L, -2); // parent, CustomTable, key, CustomTable
	lua_settable(luaTable->L, -4);

	return LuaTable::fromLuaState (L);
}

void LuaTableNode::stackPushKey() {
	l_push_LuaKey (luaTable->L, key);
}

void LuaTableNode::stackRestore() {
	lua_pop (luaTable->L, lua_gettop(luaTable->L) - stackTop);
}

bool LuaTableNode::exists() {
	bool result = true;

	if (!stackQueryValue())
		result = false;

	stackRestore();

	return result;
}

void LuaTableNode::remove() {
	if (stackQueryValue()) {
		lua_pop(luaTable->L, 1);

		if (lua_gettop(luaTable->L) != 0) {
			l_push_LuaKey (luaTable->L, key);
			lua_pushnil (luaTable->L);
			lua_settable (luaTable->L, -3); 
		} else {
			lua_pushnil (luaTable->L);
			lua_setglobal (luaTable->L, key.string_value.c_str());
		}
	}

	stackRestore();
}

size_t LuaTableNode::length() {
	size_t result = 0;

	if (stackQueryValue()) {
		result = lua_objlen(luaTable->L, -1);
	}

	stackRestore();

	return result;
}

std::vector<LuaKey> LuaTableNode::keys() {
	std::vector<LuaKey> result;

	if (stackQueryValue()) {
		// loop over all keys
		lua_pushnil(luaTable->L);
		while (lua_next(luaTable->L, -2) != 0) {
			if (lua_isnumber(luaTable->L, -2)) {
				double number = lua_tonumber (luaTable->L, -2);
				double frac;
				if (modf (number, &frac) == 0) {
					LuaKey key (static_cast<int>(number));
					result.push_back (key);
				}
			} else if (lua_isstring (luaTable->L, -2)) {
				LuaKey key (lua_tostring(luaTable->L, -2));
				result.push_back (key);
			} else {
				cerr << "Warning: invalid LuaKey type for key " << 				lua_typename(luaTable->L, lua_type(luaTable->L, -2)) << "!" << endl;
			}

			lua_pop(luaTable->L, 1);
		}
	}

	stackRestore();

	return result;
}


template<> bool LuaTableNode::getDefault<bool>(const bool &default_value) {
	bool result = default_value;

	if (stackQueryValue()) {
		result = lua_toboolean (luaTable->L, -1);
	}

	stackRestore();

	return result;
}

template<> float LuaTableNode::getDefault<float>(const float &default_value) {
	float result = default_value;

	if (stackQueryValue()) {
		result = static_cast<float>(lua_tonumber (luaTable->L, -1));
	}

	stackRestore();

	return result;
}

template<> double LuaTableNode::getDefault<double>(const double &default_value) {
	double result = default_value;

	if (stackQueryValue()) {
		result = lua_tonumber (luaTable->L, -1);
	}

	stackRestore();

	return result;
}

template<> std::string LuaTableNode::getDefault<std::string>(const std::string &default_value) {
	std::string result = default_value;

	if (stackQueryValue() && lua_isstring(luaTable->L, -1)) {
		result = lua_tostring (luaTable->L, -1);
	}

	stackRestore();

	return result;
}

template<> void LuaTableNode::set<bool>(const bool &value) {
	stackCreateValue();

	l_push_LuaKey (luaTable->L, key);
	lua_pushboolean(luaTable->L, value);
	// stack: parent, key, value
	lua_settable (luaTable->L, -3);

	stackRestore();
}

template<> void LuaTableNode::set<float>(const float &value) {
	stackCreateValue();

	l_push_LuaKey (luaTable->L, key);
	lua_pushnumber(luaTable->L, static_cast<double>(value));
	// stack: parent, key, value
	lua_settable (luaTable->L, -3);

	stackRestore();
}

template<> void LuaTableNode::set<double>(const double &value) {
	stackCreateValue();

	l_push_LuaKey (luaTable->L, key);
	lua_pushnumber(luaTable->L, value);
	// stack: parent, key, value
	lua_settable (luaTable->L, -3);

	stackRestore();
}

template<> void LuaTableNode::set<std::string>(const std::string &value) {
	stackCreateValue();

	l_push_LuaKey (luaTable->L, key);
	lua_pushstring(luaTable->L, value.c_str());
	// stack: parent, key, value
	lua_settable (luaTable->L, -3);

	stackRestore();
}

//
// LuaTable
//
LuaTable::~LuaTable() {
	if (deleteLuaState) {
		lua_close(L);
		L = NULL;
	}
}

int LuaTable::length() {
	if ((lua_gettop(L) == 0) || (lua_type (L, -1) != LUA_TTABLE)) {
		cerr << "Error: cannot query table length. No table on stack!" << endl;
		abort();
	}
	size_t result = 0;

	result = lua_objlen(L, -1);

	return result;
}

LuaTable& LuaTable::operator= (const LuaTable &luatable) {
	if (this != &luatable) {
		if (deleteLuaState && L != luatable.L) {
			lua_close (luatable.L);
		}
		filename = luatable.filename;
		L = luatable.L;
		deleteLuaState = luatable.deleteLuaState;
	}

	return *this;
}

LuaTable LuaTable::fromFile (const char* _filename) {
	LuaTable result;
	
	result.filename = _filename;
	result.L = luaL_newstate();
	result.deleteLuaState = true;
	luaL_openlibs(result.L);

	// Add the directory of _filename to package.path
	result.addSearchPath(get_file_directory (_filename).c_str());

	// run the file we 
	if (luaL_dofile (result.L, _filename)) {
		bail (result.L, "Error running file: ");
	}

	return result;
}

LuaTable LuaTable::fromLuaExpression (const char* lua_expr) {
	LuaTable result;
	
	result.L = luaL_newstate();
	result.deleteLuaState = true;
	luaL_openlibs(result.L);

	if (luaL_loadstring (result.L, lua_expr)) {
		bail (result.L, "Error compiling expression!");
	}

	if (lua_pcall (result.L, 0, LUA_MULTRET, 0)) {
		bail (result.L, "Error running expression!");
	}

	return result;
}

LuaTable LuaTable::fromLuaState (lua_State* L) {
	LuaTable result;
	
	result.L = L;
	result.deleteLuaState = false;

	return result;
}

void LuaTable::addSearchPath(const char* path) {
	if (L == NULL) {
		cerr << "Error: Cannot add search path: Lua state is not initialized!" << endl;
		abort();
	}

	lua_getglobal(L, "package");
	lua_getfield (L, -1, "path");
	if (lua_type(L, -1) != LUA_TSTRING) {
		cerr << "Error: could not get package.path!" << endl;
		abort();
	}

	string package_path = lua_tostring (L, -1);
	package_path = package_path + string(path) + "?.lua;";

	lua_pushstring(L, package_path.c_str());
	lua_setfield (L, -3, "path");

	lua_pop(L, 2);
}

std::string LuaTable::serialize() {
	std::string result;

	int current_top = lua_gettop(L);
	if (lua_gettop(L) != 0) {
		if (luaL_loadstring(L, serialize_std)) {
			bail (L, "Error loading serialization function: ");
		}

		if (lua_pcall(L, 0, 0, 0)) {
			bail (L, "Error compiling serialization function: " );
		}

		lua_getglobal (L, "serialize");
		assert (lua_isfunction (L, -1));
		lua_pushvalue (L, -2);
		if (lua_pcall (L, 1, 1, 0)) {
			bail (L, "Error while serializing: ");
		}
		result = string("return ") + lua_tostring (L, -1);
	} else {
		cerr << "Cannot serialize global Lua state!" << endl;
		abort();
	}

	lua_pop (L, lua_gettop(L) - current_top);

	return result;
}


