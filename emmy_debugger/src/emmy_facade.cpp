﻿/*
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

#include "emmy_debugger/emmy_facade.h"
#include <cstdarg>
#include <cstdint>
#include "nlohmann/json.hpp"
#include "emmy_debugger/proto/socket_server_transporter.h"
#include "emmy_debugger/proto/socket_client_transporter.h"
#include "emmy_debugger/proto/pipeline_server_transporter.h"
#include "emmy_debugger/proto/pipeline_client_transporter.h"
#include "emmy_debugger/emmy_debugger.h"
#include "emmy_debugger/transporter.h"
#include "emmy_debugger/emmy_helper.h"
#include "emmy_debugger/lua_version.h"

EmmyFacade& EmmyFacade::Get()
{
	static EmmyFacade instance;
	return instance;
}

void EmmyFacade::HookLua(lua_State* L, lua_Debug* ar)
{
	EmmyFacade::Get().Hook(L, ar);
}

void EmmyFacade::ReadyLuaHook(lua_State* L, lua_Debug* ar)
{
	if (!Get().readyHook)
	{
		return;
	}
	Get().readyHook = false;

	auto states = FindAllCoroutine(L);

	for (auto state : states)
	{
		lua_sethook(state, HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
	}

	lua_sethook(L, HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);

	auto debugger = EmmyFacade::Get().GetDebugger(L);
	if (debugger)
	{
		debugger->Attach();
	}

	Get().Hook(L, ar);
}

EmmyFacade::EmmyFacade()
	: transporter(nullptr),
	  isIDEReady(false),
	  isAPIReady(false),
	  isWaitingForIDE(false),
	  StartHook(nullptr),
	  workMode(WorkMode::EmmyCore),
	  emmyDebuggerManager(std::make_shared<EmmyDebuggerManager>()),
	  readyHook(false)
{
}

EmmyFacade::~EmmyFacade()
{
}

#ifndef EMMY_USE_LUA_SOURCE
extern "C" bool SetupLuaAPI();

bool EmmyFacade::SetupLuaAPI()
{
	isAPIReady = ::SetupLuaAPI();
	return isAPIReady;
}

#endif

int LuaError(lua_State* L)
{
	std::string msg = lua_tostring(L, 1);
	msg = "[Emmy]" + msg;
	lua_getglobal(L, "error");
	lua_pushstring(L, msg.c_str());
	lua_call(L, 1, 0);
	return 0;
}

int LuaPrint(lua_State* L)
{
	std::string msg = lua_tostring(L, 1);
	msg = "[Emmy]" + msg;
	lua_getglobal(L, "print");
	lua_pushstring(L, msg.c_str());
	lua_call(L, 1, 0);
	return 0;
}

bool EmmyFacade::TcpListen(lua_State* L, const std::string& host, int port, std::string& err)
{
	Destroy();

	emmyDebuggerManager->AddDebugger(L);

	SetReadyHook(L);

	const auto s = std::make_shared<SocketServerTransporter>();
	transporter = s;
	// s->SetHandler(shared_from_this());
	const auto suc = s->Listen(host, port, err);
	if (!suc)
	{
		lua_pushcfunction(L, LuaError);
		lua_pushstring(L, err.c_str());
		lua_call(L, 1, 0);
	}
	return suc;
}

bool EmmyFacade::TcpSharedListen(lua_State* L, const std::string& host, int port, std::string& err)
{
	if (transporter == nullptr)
	{
		return TcpListen(L, host, port, err);
	}
	return true;
}

bool EmmyFacade::TcpConnect(lua_State* L, const std::string& host, int port, std::string& err)
{
	Destroy();

	emmyDebuggerManager->AddDebugger(L);

	SetReadyHook(L);

	const auto c = std::make_shared<SocketClientTransporter>();
	transporter = c;
	// c->SetHandler(shared_from_this());
	const auto suc = c->Connect(host, port, err);
	if (suc)
	{
		WaitIDE(true);
	}
	else
	{
		lua_pushcfunction(L, LuaError);
		lua_pushstring(L, err.c_str());
		lua_call(L, 1, 0);
	}
	return suc;
}

bool EmmyFacade::PipeListen(lua_State* L, const std::string& name, std::string& err)
{
	Destroy();

	emmyDebuggerManager->AddDebugger(L);

	SetReadyHook(L);

	const auto p = std::make_shared<PipelineServerTransporter>();
	transporter = p;
	// p->SetHandler(shared_from_this());
	const auto suc = p->pipe(name, err);
	return suc;
}

bool EmmyFacade::PipeConnect(lua_State* L, const std::string& name, std::string& err)
{
	Destroy();

	emmyDebuggerManager->AddDebugger(L);

	SetReadyHook(L);

	const auto p = std::make_shared<PipelineClientTransporter>();
	transporter = p;
	// p->SetHandler(shared_from_this());
	const auto suc = p->Connect(name, err);
	if (suc)
	{
		WaitIDE(true);
	}
	return suc;
}

void EmmyFacade::WaitIDE(bool force, int timeout)
{
	if (transporter != nullptr
		&& (transporter->IsServerMode() || force)
		&& !isWaitingForIDE
		&& !isIDEReady)
	{
		isWaitingForIDE = true;
		std::unique_lock<std::mutex> lock(waitIDEMutex);
		if (timeout > 0)
			waitIDECV.wait_for(lock, std::chrono::milliseconds(timeout));
		else
			waitIDECV.wait(lock);
		isWaitingForIDE = false;
	}
}

int EmmyFacade::BreakHere(lua_State* L)
{
	if (!isIDEReady)
		return 0;

	emmyDebuggerManager->HandleBreak(L);

	return 1;
}

int EmmyFacade::OnConnect(bool suc)
{
	return 0;
}

int EmmyFacade::OnDisconnect()
{
	isIDEReady = false;
	isWaitingForIDE = false;

	emmyDebuggerManager->OnDisconnect();

	if (workMode == WorkMode::Attach)
	{
		emmyDebuggerManager->RemoveAllDebugger();
	}

	return 0;
}

void EmmyFacade::Destroy()
{
	OnDisconnect();

	if (transporter)
	{
		transporter->Stop();
		transporter = nullptr;
	}
}

void EmmyFacade::SetWorkMode(WorkMode mode)
{
	workMode = mode;
}

WorkMode EmmyFacade::GetWorkMode()
{
	return workMode;
}

void EmmyFacade::OnReceiveMessage(const nlohmann::json document)
{
	if (document["cmd"].is_number_integer())
	{
		switch (document["cmd"].get<MessageCMD>())
		{
		case MessageCMD::InitReq:
			OnInitReq(document);
			break;
		case MessageCMD::ReadyReq:
			OnReadyReq(document);
			break;
		case MessageCMD::AddBreakPointReq:
			OnAddBreakPointReq(document);
			break;
		case MessageCMD::RemoveBreakPointReq:
			OnRemoveBreakPointReq(document);
			break;
		case MessageCMD::ActionReq:
			//assert(isIDEReady);
			OnActionReq(document);
			break;
		case MessageCMD::EvalReq:
			//assert(isIDEReady);
			OnEvalReq(document);
			break;
		default:
			break;
		}
	}
}

void EmmyFacade::OnInitReq(const nlohmann::json document)
{
	if (StartHook)
	{
		StartHook();
	}

	if (document["emmyHelper"].is_string())
	{
		emmyDebuggerManager->helperCode = document["emmyHelper"];
	}

	//file extension names: .lua, .txt, .lua.txt ...
	if (document["ext"].is_array())
	{
		emmyDebuggerManager->extNames.clear();
		for (auto& ext : document["ext"])
		{
			emmyDebuggerManager->extNames.emplace_back(ext);
		}
	}

	// 这里有个线程安全问题，消息线程和lua 执行线程不是相同线程，但是没有一个锁能让我做同步
	// 所以我不能在这里访问lua state 指针的内部结构
	// 
	// 方案：提前为主state 设置hook 利用hook 实现同步

	// fix 以上安全问题
	StartDebug();
}

void EmmyFacade::OnReadyReq(const nlohmann::json document)
{
	isIDEReady = true;
	waitIDECV.notify_all();
}

nlohmann::json FillVariables(std::vector<std::shared_ptr<Variable>>& variables);

nlohmann::json FillVariable(const std::shared_ptr<Variable> variable)
{
	auto obj = nlohmann::json::object();
	obj["name"] = variable->name;
	obj["nameType"] = variable->nameType;
	obj["value"] = variable->value;
	obj["valueType"] = variable->valueType;
	obj["valueTypeName"] = variable->valueTypeName;
	obj["cacheId"] = variable->cacheId;

	// children
	if (!variable->children.empty())
	{
		obj["children"] = FillVariables(variable->children);
	}
	return obj;
}

nlohmann::json FillVariables(std::vector<std::shared_ptr<Variable>>& variables)
{
	auto arr = nlohmann::json::array();
	for (auto& child : variables)
	{
		arr.push_back(FillVariable(child));
	}
	return arr;
}

nlohmann::json FillStacks(std::vector<std::shared_ptr<Stack>>& stacks)
{
	auto stacksJson = nlohmann::json::array();
	for (auto stack : stacks)
	{
		auto stackJson = nlohmann::json::object();
		stackJson["file"] = stack->file;
		stackJson["functionName"] = stack->functionName;
		stackJson["line"] = stack->line;
		stackJson["level"] = stack->level;
		stackJson["localVariables"] = FillVariables(stack->localVariables);
		stackJson["upvalueVariables"] = FillVariables(stack->upvalueVariables);

		stacksJson.push_back(stackJson);
	}
	return stacksJson;
}

bool EmmyFacade::OnBreak(std::shared_ptr<Debugger> debugger)
{
	if (!debugger)
	{
		return false;
	}
	std::vector<std::shared_ptr<Stack>> stacks;

	emmyDebuggerManager->SetBreakedDebugger(debugger);

	debugger->GetStacks(stacks, []()
	{
		return std::make_shared<Stack>();
	});

	auto obj = nlohmann::json::object();
	obj["cmd"] = static_cast<int>(MessageCMD::BreakNotify);
	obj["stacks"] = FillStacks(stacks);

	transporter->Send(int(MessageCMD::BreakNotify), obj);

	return true;
}

void ReadBreakPoint(const nlohmann::json value, std::shared_ptr<BreakPoint> bp)
{
	if (value.count("file") != 0)
	{
		bp->file = value["file"];
	}
	if (value.count("line") != 0)
	{
		bp->line = value["line"];
	}
	if (value.count("condition") != 0)
	{
		bp->condition = value["condition"];
	}
	if (value.count("hitCondition") != 0)
	{
		bp->hitCondition = value["hitCondition"];
	}
	if (value.count("logMessage") != 0)
	{
		bp->logMessage = value["logMessage"];
	}
}

void EmmyFacade::OnAddBreakPointReq(const nlohmann::json document)
{
	if (document.count("clear") != 0 && document["clear"].is_boolean())
	{
		bool all = document["clear"];
		if (all)
		{
			emmyDebuggerManager->RemoveAllBreakPoints();
		}
	}
	if (document.count("breakPoints") != 0 && document["breakPoints"].is_array())
	{
		for (auto docBreakPoints : document["breakPoints"])
		{
			auto bp = std::make_shared<BreakPoint>();
			bp->hitCount = 0;
			ReadBreakPoint(docBreakPoints, bp);
			emmyDebuggerManager->AddBreakpoint(bp);
		}
	}
	// todo: response
}

void EmmyFacade::OnRemoveBreakPointReq(const nlohmann::json document)
{
	if (document.count("breakPoints") != 0 && document["breakPoints"].is_array())
	{
		for (auto& breakpoint : document["breakPoints"])
		{
			auto bp = std::make_shared<BreakPoint>();
			ReadBreakPoint(breakpoint, bp);
			emmyDebuggerManager->RemoveBreakpoint(bp->file, bp->line);
		}
	}
	// todo: response
}

void EmmyFacade::OnActionReq(const nlohmann::json document)
{
	if (document.count("action") != 0 && document["action"].is_number_integer())
	{
		const auto action = document["action"].get<DebugAction>();

		emmyDebuggerManager->DoAction(action);
	}
	// todo: response
}

void EmmyFacade::OnEvalReq(const nlohmann::json document)
{
	auto context = std::make_shared<EvalContext>();
	if (document.count("seq") != 0 && document["seq"].is_number_integer())
	{
		context->seq = document["seq"];
	}
	if (document.count("expr") != 0 && document["expr"].is_string())
	{
		context->expr = document["expr"];
	}

	if (document.count("stackLevel") != 0 && document["stackLevel"].is_number_integer())
	{
		context->stackLevel = document["stackLevel"];
	}
	if (document.count("depth") != 0  && document["depth"].is_number_integer())
	{
		context->depth = document["depth"];
	}
	if (document.count("cacheId") != 0 && document["cacheId"].is_number_integer())
	{
		context->cacheId = document["cacheId"];
	}

	context->success = false;

	emmyDebuggerManager->Eval(context);
}

void EmmyFacade::OnEvalResult(std::shared_ptr<EvalContext> context)
{
	auto obj = nlohmann::json::object();
	obj["seq"] = context->seq;
	obj["success"] = context->success;

	if (context->success)
	{
		obj["value"] = FillVariable(context->result);
	}
	else
	{
		obj["error"] = context->error;
	}

	if (transporter)
	{
		transporter->Send(int(MessageCMD::EvalRsp), obj);
	}
}

void EmmyFacade::SendLog(LogType type, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buff[1024] = {0};
	vsnprintf(buff, 1024, fmt, args);
	va_end(args);

	const std::string msg = buff;

	auto obj = nlohmann::json::object();
	obj["type"] = type;
	obj["message"] = msg;

	if (transporter)
	{
		transporter->Send(int(MessageCMD::LogNotify), obj);
	}
}

void EmmyFacade::OnLuaStateGC(lua_State* L)
{
	auto debugger = emmyDebuggerManager->RemoveDebugger(L);

	if (debugger)
	{
		debugger->Detach();
	}

	if (workMode == WorkMode::EmmyCore)
	{
		if (emmyDebuggerManager->IsDebuggerEmpty())
		{
			Destroy();
		}
	}
}

void EmmyFacade::Hook(lua_State* L, lua_Debug* ar)
{
	auto debugger = GetDebugger(L);
	if (debugger)
	{
		if (!debugger->IsRunning())
		{
			if (EmmyFacade::Get().GetWorkMode() == WorkMode::EmmyCore)
			{
				if (luaVersion != LuaVersion::LUA_JIT)
				{
					if (debugger->IsMainCoroutine(L))
					{
						SetReadyHook(L);
					}
				}
				else
				{
					SetReadyHook(L);
				}
			}
			return;
		}

		debugger->Hook(ar, L);
	}
	else
	{
		if (workMode == WorkMode::Attach)
		{
			debugger = emmyDebuggerManager->AddDebugger(L);
			install_emmy_core(L);
			if (emmyDebuggerManager->IsRunning())
			{
				debugger->Start();
				debugger->Attach();
			}
			// send attached notify
			auto obj = nlohmann::json::object();
			obj["state"] = reinterpret_cast<int64_t>(L);

			this->transporter->Send(int(MessageCMD::AttachedNotify), obj);

			debugger->Hook(ar, L);
		}
	}
}

std::shared_ptr<EmmyDebuggerManager> EmmyFacade::GetDebugManager() const
{
	return emmyDebuggerManager;
}


std::shared_ptr<Variable> EmmyFacade::GetVariableRef(Variable* variable)
{
	auto it = luaVariableRef.find(variable);

	if (it != luaVariableRef.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

void EmmyFacade::AddVariableRef(std::shared_ptr<Variable> variable)
{
	luaVariableRef.insert({variable.get(), variable});
}

void EmmyFacade::RemoveVariableRef(Variable* variable)
{
	auto it = luaVariableRef.find(variable);
	if (it != luaVariableRef.end())
	{
		luaVariableRef.erase(it);
	}
}

std::shared_ptr<Debugger> EmmyFacade::GetDebugger(lua_State* L)
{
	return emmyDebuggerManager->GetDebugger(L);
}

void EmmyFacade::SetReadyHook(lua_State* L)
{
	lua_sethook(L, ReadyLuaHook, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
}

void EmmyFacade::StartDebug()
{
	emmyDebuggerManager->SetRunning(true);
	readyHook = true;
}

void EmmyFacade::StartupHookMode(int port)
{
	Destroy();

	// 1024 - 65535
	while (port > 0xffff) port -= 0xffff;
	while (port < 0x400) port += 0x400;

	const auto s = std::make_shared<SocketServerTransporter>();
	std::string err;
	const auto suc = s->Listen("localhost", port, err);
	if (suc)
	{
		transporter = s;
		// transporter->SetHandler(shared_from_this());
	}
}

void EmmyFacade::Attach(lua_State* L)
{
	if (!this->transporter->IsConnected())
		return;

	// 这里存在一个问题就是 hook 的时机太早了，globalstate 都还没初始化完毕

	if (!isAPIReady)
	{
		// 考虑到emmy_hook use lua source
		isAPIReady = install_emmy_core(L);
	}

	lua_sethook(L, EmmyFacade::HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
}
