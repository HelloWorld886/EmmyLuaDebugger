/*
* Copyright (c) 2019. tangzx(love.tangzx@qq.com)
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "emmy_debugger/api/lua_api_loader.h"
#include <cstdio>
#include <cassert>
#include "emmy_debugger/lua_version.h"


#ifdef WIN32
#include <Windows.h>
#include <TlHelp32.h>

HMODULE FindLuaModule()
{
	HMODULE hModule = nullptr;
	MODULEENTRY32 module;
	module.dwSize = sizeof(MODULEENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	BOOL moreModules = Module32First(hSnapshot, &module);

	while (moreModules)
	{
#ifndef LUA_COMPILE_AS_CPP
		if (GetProcAddress(module.hModule, "lua_gettop"))
#else
		if (GetProcAddress(module.hModule, "?lua_gettop@@YAHPEAUlua_State@@@Z"))
#endif
		{
			hModule = module.hModule;
			break;
		}
		moreModules = Module32Next(hSnapshot, &module);
	}
	return hModule;
}

HMODULE hModule = nullptr;

FARPROC LoadAPI(const char* name)
{
	if (!hModule)
		hModule = FindLuaModule();

	return GetProcAddress(hModule, name);
}
#else
#include <dlfcn.h>
void* LoadAPI(const char* name) {
    const auto handler = dlsym(RTLD_DEFAULT, name);
    return handler;
}
#endif
int LUA_REGISTRYINDEX = 0;
int LUA_GLOBALSINDEX = 0;

IMP_LUA_API(lua_gettop);
IMP_LUA_API(lua_settop);
IMP_LUA_API(lua_type);
IMP_LUA_API(lua_typename);
IMP_LUA_API(lua_tolstring);
IMP_LUA_API(lua_toboolean);
IMP_LUA_API(lua_pushnil);
IMP_LUA_API(lua_pushnumber);
IMP_LUA_API(lua_pushlstring);
IMP_LUA_API(lua_pushstring);
IMP_LUA_API(lua_pushcclosure);
IMP_LUA_API(lua_pushboolean);
IMP_LUA_API(lua_pushvalue);
IMP_LUA_API(lua_getfield);
IMP_LUA_API(lua_next);
IMP_LUA_API(lua_createtable);
IMP_LUA_API(lua_setfield);
IMP_LUA_API(lua_setmetatable);
IMP_LUA_API(lua_getstack);
IMP_LUA_API(lua_getinfo);
IMP_LUA_API(lua_getlocal);
IMP_LUA_API(lua_getupvalue);
IMP_LUA_API(lua_setupvalue);
IMP_LUA_API(lua_sethook);
IMP_LUA_API(luaL_loadstring);
IMP_LUA_API(luaL_checklstring);
IMP_LUA_API(luaL_checknumber);
IMP_LUA_API(lua_topointer);
IMP_LUA_API(lua_getmetatable);
IMP_LUA_API(lua_rawget);
IMP_LUA_API(lua_rawset);
IMP_LUA_API(lua_pushlightuserdata);
IMP_LUA_API(lua_touserdata);
IMP_LUA_API(luaL_newstate);
IMP_LUA_API(lua_close);
IMP_LUA_API(lua_pushthread);

IMP_LUA_API(lua_rawseti);
IMP_LUA_API(lua_rawgeti);
//51
IMP_LUA_API_E(lua_setfenv);
IMP_LUA_API_E(lua_tointeger);
IMP_LUA_API_E(lua_tonumber);
IMP_LUA_API_E(lua_call);
IMP_LUA_API_E(lua_pcall);
IMP_LUA_API_E(lua_remove);
IMP_LUA_API_E(lua_insert);
// 52 53 54
IMP_LUA_API_E(lua_rawgetp);
IMP_LUA_API_E(lua_rawsetp);

//53
IMP_LUA_API_E(lua_tointegerx);
IMP_LUA_API_E(lua_tonumberx);
IMP_LUA_API_E(lua_getglobal);
IMP_LUA_API_E(lua_setglobal);
IMP_LUA_API_E(lua_callk);
IMP_LUA_API_E(lua_pcallk);
IMP_LUA_API_E(luaL_setfuncs);
IMP_LUA_API_E(lua_absindex);
IMP_LUA_API_E(lua_rotate);
//51 & 52 & 53
IMP_LUA_API_E(lua_newuserdata);
//54
IMP_LUA_API_E(lua_newuserdatauv);
//jit
IMP_LUA_API_E(luaopen_jit);

int lua_setfenv(lua_State* L, int idx)
{
	if (luaVersion == LuaVersion::LUA_51 || luaVersion == LuaVersion::LUA_JIT)
	{
		return e_lua_setfenv(L, idx);
	}
	return lua_setupvalue(L, idx, 1) != nullptr;
}

int getDebugEvent(lua_Debug* ar)
{
	switch (luaVersion)
	{
	case LuaVersion::LUA_JIT:
	case LuaVersion::LUA_51:
		return ar->u.ar51.event;
	case LuaVersion::LUA_52:
		return ar->u.ar52.event;
	case LuaVersion::LUA_53:
		return ar->u.ar53.event;
	case LuaVersion::LUA_54:
		return ar->u.ar54.event;
	default:
		assert(false);
		return 0;
	}
}

int getDebugCurrentLine(lua_Debug* ar)
{
	switch (luaVersion)
	{
	case LuaVersion::LUA_JIT:
	case LuaVersion::LUA_51:
		return ar->u.ar51.currentline;
	case LuaVersion::LUA_52:
		return ar->u.ar52.currentline;
	case LuaVersion::LUA_53:
		return ar->u.ar53.currentline;
	case LuaVersion::LUA_54:
		return ar->u.ar54.currentline;
	default:
		assert(false);
		return 0;
	}
}

int getDebugLineDefined(lua_Debug* ar)
{
	switch (luaVersion)
	{
	case LuaVersion::LUA_JIT:
	case LuaVersion::LUA_51:
		return ar->u.ar51.linedefined;
	case LuaVersion::LUA_52:
		return ar->u.ar52.linedefined;
	case LuaVersion::LUA_53:
		return ar->u.ar53.linedefined;
	case LuaVersion::LUA_54:
		return ar->u.ar54.linedefined;
	default:
		assert(false);
		return 0;
	}
}

const char* getDebugSource(lua_Debug* ar)
{
	switch (luaVersion)
	{
	case LuaVersion::LUA_JIT:
	case LuaVersion::LUA_51:
		return ar->u.ar51.source;
	case LuaVersion::LUA_52:
		return ar->u.ar52.source;
	case LuaVersion::LUA_53:
		return ar->u.ar53.source;
	case LuaVersion::LUA_54:
		return ar->u.ar54.source;
	default:
		assert(false);
		return nullptr;
	}
}

const char* getDebugName(lua_Debug* ar)
{
	switch (luaVersion)
	{
	case LuaVersion::LUA_JIT:
	case LuaVersion::LUA_51:
		return ar->u.ar51.name;
	case LuaVersion::LUA_52:
		return ar->u.ar52.name;
	case LuaVersion::LUA_53:
		return ar->u.ar53.name;
	case LuaVersion::LUA_54:
		return ar->u.ar54.name;
	default:
		assert(false);
		return nullptr;
	}
}

lua_Integer lua_tointeger(lua_State* L, int idx)
{
	if (luaVersion > LuaVersion::LUA_51)
	{
		return e_lua_tointegerx(L, idx, nullptr);
	}
	return e_lua_tointeger(L, idx);
}

lua_Number lua_tonumber(lua_State* L, int idx)
{
	if (luaVersion > LuaVersion::LUA_51)
	{
		return e_lua_tonumberx(L, idx, nullptr);
	}
	return e_lua_tonumber(L, idx);
}

int lua_getglobal(lua_State* L, const char* name)
{
	if (luaVersion > LuaVersion::LUA_51)
	{
		return e_lua_getglobal(L, name);
	}
	return lua_getfield(L, LUA_GLOBALSINDEX, name);
}

void lua_setglobal(lua_State* L, const char* name)
{
	if (luaVersion > LuaVersion::LUA_51)
	{
		return e_lua_setglobal(L, name);
	}
	return lua_setfield(L, LUA_GLOBALSINDEX, name);
}

void lua_call(lua_State* L, int nargs, int nresults)
{
	if (luaVersion > LuaVersion::LUA_51)
	{
		return e_lua_callk(L, nargs, nresults, 0, nullptr);
	}
	return (void)e_lua_call(L, nargs, nresults);
}

int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc)
{
	if (luaVersion > LuaVersion::LUA_51)
	{
		return e_lua_pcallk(L, nargs, nresults, errfunc, 0, nullptr);
	}
	return e_lua_pcall(L, nargs, nresults, errfunc);
}

void luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup)
{
	if (luaVersion == LuaVersion::LUA_51 || luaVersion == LuaVersion::LUA_JIT)
	{
		for (; l->name != nullptr; l++)
		{
			for (int i = 0; i < nup; i++)
				lua_pushvalue(L, -nup);
			lua_pushcclosure(L, l->func, nup);
			lua_setfield(L, -(nup + 2), l->name);
		}
	}
	else e_luaL_setfuncs(L, l, nup);
}

int lua_upvalueindex(int i)
{
	if (luaVersion == LuaVersion::LUA_54)
		return -1001000 - i;
	if (luaVersion == LuaVersion::LUA_53)
		return -1001000 - i;
	if (luaVersion == LuaVersion::LUA_52)
		return -1001000 - i;
	return -10002 - i;
}

int lua_absindex(lua_State* L, int idx)
{
	if (luaVersion == LuaVersion::LUA_51 || luaVersion == LuaVersion::LUA_JIT)
	{
		if (idx > 0)
		{
			return idx;
		}
		return lua_gettop(L) + idx + 1;
	}
	return e_lua_absindex(L, idx);
}

void lua_remove(lua_State* L, int idx)
{
	if (luaVersion == LuaVersion::LUA_51 || luaVersion == LuaVersion::LUA_52 || luaVersion == LuaVersion::LUA_JIT)
	{
		e_lua_remove(L, idx);
	}
	else
	{
		e_lua_rotate(L, (idx), -1);
		lua_pop(L, 1);
	}
}

void* lua_newuserdata(lua_State* L, int size)
{
	if (luaVersion == LuaVersion::LUA_51 || luaVersion == LuaVersion::LUA_52 || luaVersion == LuaVersion::LUA_53 || luaVersion == LuaVersion::LUA_JIT)
	{
		return e_lua_newuserdata(L, size);
	}
	else
	{
		return e_lua_newuserdatauv(L, size, 1);
	}
}

void lua_pushglobaltable(lua_State* L)
{
	if (luaVersion == LuaVersion::LUA_51 || luaVersion == LuaVersion::LUA_JIT)
	{
		lua_pushvalue(L, LUA_GLOBALSINDEX);
	}
	else
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_GLOBALSINDEX);
	}
}

int lua_rawgetp(lua_State* L, int idx, const void* p)
{
	if (luaVersion == LuaVersion::LUA_51 || luaVersion == LuaVersion::LUA_JIT)
	{
		if (idx < 0)
		{
			idx += lua_gettop(L) + 1;
		}
		lua_pushlightuserdata(L, (void*)p);
		return lua_rawget(L, idx);
	}
	else
	{
		return e_lua_rawgetp(L, idx, p);
	}
}

void lua_rawsetp(lua_State* L, int idx, const void* p)
{
	if (luaVersion == LuaVersion::LUA_51 || luaVersion == LuaVersion::LUA_JIT)
	{
		if (idx < 0)
		{
			idx += lua_gettop(L) + 1;
		}
		lua_pushlightuserdata(L, (void*)p);
		lua_insert(L, -2);
		lua_rawset(L, idx);
	}
	else
	{
		return e_lua_rawsetp(L, idx, p);
	}
}

void lua_insert(lua_State* L, int idx)
{
	if (luaVersion == LuaVersion::LUA_51 || luaVersion == LuaVersion::LUA_52 || luaVersion == LuaVersion::LUA_JIT)
	{
		return e_lua_insert(L, idx);
	}
	else
	{
		return e_lua_rotate(L, idx, 1);
	}
}


extern "C" bool SetupLuaAPI()
{
#ifndef LUA_COMPILE_AS_CPP
	LOAD_LUA_API(lua_gettop);
	LOAD_LUA_API(lua_settop);
	LOAD_LUA_API(lua_type);
	LOAD_LUA_API(lua_typename);
	LOAD_LUA_API(lua_tolstring);
	LOAD_LUA_API(lua_toboolean);
	LOAD_LUA_API(lua_pushnil);
	LOAD_LUA_API(lua_pushnumber);
	LOAD_LUA_API(lua_pushlstring);
	LOAD_LUA_API(lua_pushstring);
	LOAD_LUA_API(lua_pushcclosure);
	LOAD_LUA_API(lua_pushboolean);
	LOAD_LUA_API(lua_pushvalue);
	LOAD_LUA_API(lua_getfield);
	LOAD_LUA_API(lua_next);
	LOAD_LUA_API(lua_createtable);
	LOAD_LUA_API(lua_setfield);
	LOAD_LUA_API(lua_setmetatable);
	LOAD_LUA_API(lua_getstack);
	LOAD_LUA_API(lua_getinfo);
	LOAD_LUA_API(lua_getlocal);
	LOAD_LUA_API(lua_getupvalue);
	LOAD_LUA_API(lua_setupvalue);
	LOAD_LUA_API(lua_sethook);
	LOAD_LUA_API(luaL_loadstring);
	LOAD_LUA_API(luaL_checklstring);
	LOAD_LUA_API(luaL_checknumber);
	LOAD_LUA_API(lua_topointer);
	LOAD_LUA_API(lua_getmetatable);
	LOAD_LUA_API(lua_rawget);
	LOAD_LUA_API(lua_rawset);
	LOAD_LUA_API(lua_pushlightuserdata);
	LOAD_LUA_API(lua_touserdata);
	LOAD_LUA_API(luaL_newstate);
	LOAD_LUA_API(lua_close);
	LOAD_LUA_API(lua_pushthread);

	LOAD_LUA_API(lua_rawseti);
	LOAD_LUA_API(lua_rawgeti);
	//51
	LOAD_LUA_API_E(lua_setfenv);
	LOAD_LUA_API_E(lua_tointeger);
	LOAD_LUA_API_E(lua_tonumber);
	LOAD_LUA_API_E(lua_call);
	LOAD_LUA_API_E(lua_pcall);
	//51 & 52
	LOAD_LUA_API_E(lua_remove);
	//52 & 53 & 54
	LOAD_LUA_API_E(lua_tointegerx);
	LOAD_LUA_API_E(lua_tonumberx);
	LOAD_LUA_API_E(lua_getglobal);
	LOAD_LUA_API_E(lua_setglobal);
	LOAD_LUA_API_E(lua_callk);
	LOAD_LUA_API_E(lua_pcallk);
	LOAD_LUA_API_E(luaL_setfuncs);
	LOAD_LUA_API_E(lua_absindex);
	LOAD_LUA_API_E(lua_rawgetp);
	LOAD_LUA_API_E(lua_rawsetp);


	// 51 & 52 & 53
	LOAD_LUA_API_E(lua_newuserdata);
	//53
	LOAD_LUA_API_E(lua_rotate);
	//54
	LOAD_LUA_API_E(lua_newuserdatauv);

	//jit
	LOAD_LUA_API_E(luaopen_jit);
#else
	LOAD_LUA_API_CPP(lua_gettop, ?lua_gettop@@YAHPEAUlua_State@@@Z);
	LOAD_LUA_API_CPP(lua_settop, ?lua_settop@@YAXPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_type, ?lua_type@@YAHPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_typename, ?lua_typename@@YAPEBDPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_tolstring, ?lua_tolstring@@YAPEBDPEAUlua_State@@HPEA_K@Z);
	LOAD_LUA_API_CPP(lua_toboolean, ?lua_toboolean@@YAHPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_pushnil, ?lua_pushnil@@YAXPEAUlua_State@@@Z);
	LOAD_LUA_API_CPP(lua_pushnumber, ?lua_pushnumber@@YAXPEAUlua_State@@N@Z);
	LOAD_LUA_API_CPP(lua_pushlstring, ?lua_pushlstring@@YAPEBDPEAUlua_State@@PEBD_K@Z);
	LOAD_LUA_API_CPP(lua_pushstring, ?lua_pushstring@@YAPEBDPEAUlua_State@@PEBD@Z);
	LOAD_LUA_API_CPP(lua_pushcclosure, ?lua_pushcclosure@@YAXPEAUlua_State@@P6AH0@ZH@Z);
	LOAD_LUA_API_CPP(lua_pushboolean, ?lua_pushboolean@@YAXPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_pushvalue, ?lua_pushvalue@@YAXPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_getfield, ?lua_getfield@@YAHPEAUlua_State@@HPEBD@Z);
	LOAD_LUA_API_CPP(lua_next, ?lua_next@@YAHPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_createtable, ?lua_createtable@@YAXPEAUlua_State@@HH@Z);
	LOAD_LUA_API_CPP(lua_setfield, ?lua_setfield@@YAXPEAUlua_State@@HPEBD@Z);
	LOAD_LUA_API_CPP(lua_setmetatable, ?lua_setmetatable@@YAHPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_getstack, ?lua_getstack@@YAHPEAUlua_State@@HPEAUlua_Debug@@@Z);
	LOAD_LUA_API_CPP(lua_getinfo, ?lua_getinfo@@YAHPEAUlua_State@@PEBDPEAUlua_Debug@@@Z);
	LOAD_LUA_API_CPP(lua_getlocal, ?lua_getlocal@@YAPEBDPEAUlua_State@@PEBUlua_Debug@@H@Z);
	LOAD_LUA_API_CPP(lua_getupvalue, ?lua_getupvalue@@YAPEBDPEAUlua_State@@HH@Z);
	LOAD_LUA_API_CPP(lua_setupvalue, ?lua_setupvalue@@YAPEBDPEAUlua_State@@HH@Z);
	LOAD_LUA_API_CPP(lua_sethook, ?lua_sethook@@YAXPEAUlua_State@@P6AX0PEAUlua_Debug@@@ZHH@Z);
	LOAD_LUA_API_CPP(luaL_loadstring, ?luaL_loadstring@@YAHPEAUlua_State@@PEBD@Z);
	LOAD_LUA_API_CPP(luaL_checklstring, ?luaL_checklstring@@YAPEBDPEAUlua_State@@HPEA_K@Z);
	LOAD_LUA_API_CPP(luaL_checknumber, ?luaL_checknumber@@YANPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_topointer, ?lua_topointer@@YAPEBXPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_getmetatable, ?lua_getmetatable@@YAHPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_rawget, ?lua_rawget@@YAHPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_rawset, ?lua_rawset@@YAXPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(lua_pushlightuserdata, ?lua_pushlightuserdata@@YAXPEAUlua_State@@PEAX@Z);
	LOAD_LUA_API_CPP(lua_touserdata, ?lua_touserdata@@YAPEAXPEAUlua_State@@H@Z);
	LOAD_LUA_API_CPP(luaL_newstate, ?luaL_newstate@@YAPEAUlua_State@@XZ);
	LOAD_LUA_API_CPP(lua_close, ?lua_close@@YAXPEAUlua_State@@@Z);
	LOAD_LUA_API_CPP(lua_pushthread, ?lua_pushthread@@YAHPEAUlua_State@@@Z);

	LOAD_LUA_API_CPP(lua_rawseti, ?lua_rawseti@@YAXPEAUlua_State@@H_J@Z);
	LOAD_LUA_API_CPP(lua_rawgeti, ?lua_rawgeti@@YAHPEAUlua_State@@H_J@Z);
	//51
	LOAD_LUA_API_E_CPP(lua_setfenv, lua_setfenv);
	LOAD_LUA_API_E_CPP(lua_setfenv, lua_tointeger);
	LOAD_LUA_API_E_CPP(lua_setfenv, lua_tonumber);
	LOAD_LUA_API_E_CPP(lua_setfenv, lua_call);
	LOAD_LUA_API_E_CPP(lua_pcall, lua_pcall);
	//51 & 52
	LOAD_LUA_API_E_CPP(lua_remove, lua_remove);
	//52 & 53 & 54
	LOAD_LUA_API_E_CPP(lua_tointegerx, ?lua_tointegerx@@YA_JPEAUlua_State@@HPEAH@Z);
	LOAD_LUA_API_E_CPP(lua_tonumberx, ?lua_tonumberx@@YANPEAUlua_State@@HPEAH@Z);
	LOAD_LUA_API_E_CPP(lua_getglobal, ?lua_getglobal@@YAHPEAUlua_State@@PEBD@Z);
	LOAD_LUA_API_E_CPP(lua_setglobal, ?lua_setglobal@@YAXPEAUlua_State@@PEBD@Z);
	LOAD_LUA_API_E_CPP(lua_callk, ?lua_callk@@YAXPEAUlua_State@@HH_JP6AH0H1@Z@Z);
	LOAD_LUA_API_E_CPP(lua_pcallk, ?lua_pcallk@@YAHPEAUlua_State@@HHH_JP6AH0H1@Z@Z);
	LOAD_LUA_API_E_CPP(luaL_setfuncs, ?luaL_setfuncs@@YAXPEAUlua_State@@PEBUluaL_Reg@@H@Z);
	LOAD_LUA_API_E_CPP(lua_absindex, ?lua_absindex@@YAHPEAUlua_State@@H@Z);
	LOAD_LUA_API_E_CPP(lua_rawgetp, ?lua_rawgetp@@YAHPEAUlua_State@@HPEBX@Z);
	LOAD_LUA_API_E_CPP(lua_rawsetp, ?lua_rawsetp@@YAXPEAUlua_State@@HPEBX@Z);


	// 51 & 52 & 53
	LOAD_LUA_API_E_CPP(lua_newuserdata, lua_newuserdata);
	//53
	LOAD_LUA_API_E_CPP(lua_rotate, ?lua_rotate@@YAXPEAUlua_State@@HH@Z);
	//54
	LOAD_LUA_API_E_CPP(lua_newuserdatauv, ?lua_newuserdatauv@@YAPEAXPEAUlua_State@@_KH@Z);

	//jit
	LOAD_LUA_API_E_CPP(luaopen_jit, luaopen_jit);
#endif


	if (e_lua_newuserdatauv)
	{
		luaVersion = LuaVersion::LUA_54;
		LUA_REGISTRYINDEX = -1001000;
		LUA_GLOBALSINDEX = 2;
	}
	else if (e_lua_rotate)
	{
		luaVersion = LuaVersion::LUA_53;
		LUA_REGISTRYINDEX = -1001000;
		LUA_GLOBALSINDEX = 2;
	}
	else if (e_lua_callk)
	{
		luaVersion = LuaVersion::LUA_52;
		//todo
		LUA_REGISTRYINDEX = -1001000;
		LUA_GLOBALSINDEX = 2;
	}
	else if (e_luaopen_jit)
	{
		luaVersion = LuaVersion::LUA_JIT;
		LUA_REGISTRYINDEX = -10000;
		LUA_GLOBALSINDEX = -10002;
	}
	else
	{
		luaVersion = LuaVersion::LUA_51;
		LUA_REGISTRYINDEX = -10000;
		LUA_GLOBALSINDEX = -10002;
	}
	if (luaVersion > LuaVersion::LUA_JIT) {
		printf("[EMMY] version: %s, lua version: %d\n", EMMY_CORE_VERSION, luaVersion);
	}
	else
	{
		printf("[EMMY] version: %s, lua version: jit\n", EMMY_CORE_VERSION);
	}
	return true;
}
