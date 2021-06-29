#pragma once
#include <nlohmann/json.hpp>

namespace Engine
{
	// Specify read/write from json. I don't like this name
	enum class SerialiseType
	{
		Read,
		Write
	};
}

// add this to the public interface of your class
#define SERIALISED_CLASS()	\
	void Serialise(nlohmann::json& json, Engine::SerialiseType op)

// add this to your cpp, generates serialise function + registers factories
#define SERIALISE_BEGIN(classname)	\
	Engine::Serialisation::ObjectFactory<classname>::Register s_registerFactory_##classname(#classname,[](){ return new classname; });	\
	void classname::Serialise(nlohmann::json& json, Engine::SerialiseType op)	\
	{	\
		const char* c_myClassName = #classname;

// add this to your cpp, generates serialise function + registers factories for templated class
#define SERIALISE_BEGIN_TEMPL(classname, templ)	\
	Engine::Serialisation::ObjectFactory<classname<templ>>::Register s_registerFactory_##classname_##templ(#classname "<" #templ ">", [](){ return new classname<templ>; });	\
	inline void classname<templ>::Serialise(nlohmann::json& json, Engine::SerialiseType op)	\
	{	\
		const char* c_myClassName = #classname "<" #templ ">";

// add this to your cpp, generates serialise function + registers factories for objects with parent classes
#define SERIALISE_BEGIN_WITH_PARENT(classname,parent)	\
	Engine::Serialisation::ObjectFactory<parent>::Register s_registerFactory_##parent_##classname(#classname,[]() {	\
		return reinterpret_cast<parent*>(new classname);	\
	});	\
	void classname::Serialise(nlohmann::json& json, Engine::SerialiseType op)	\
	{	\
		const char* c_myClassName = #classname;	\
		parent::Serialise(json,op);	\

// add this to your cpp, generates serialise function + registers factories for templated objects with parent classes
#define SERIALISE_BEGIN_WITH_PARENT_TEMPL(classname,parent,templ)	\
	Engine::Serialisation::ObjectFactory<parent>::Register s_registerFactory_##parent_##classname_##templ(#classname "<" #templ ">",[]() {	\
		return reinterpret_cast<parent*>(new classname<templ>);	\
	});	\
	inline void classname<templ>::Serialise(nlohmann::json& json, Engine::SerialiseType op)	\
	{	\
		const char* c_myClassName = #classname "<" #templ ">";	\
		parent::Serialise(json,op);	\

// properties (can handle almost anything)
#define SERIALISE_PROPERTY(name,p)	\
		if(op == Engine::SerialiseType::Write) {	\
			Engine::ToJson(name, p, json);	\
		}	\
		else    \
		{	\
			Engine::FromJson(name, p, json);	\
		}

// robin hood containers need their own macros (avoids template madness)
// i'm sure there is a better way to handle this
#define SERIALISE_PROPERTY_ROBINHOOD(name,p)	\
		if(op==Engine::SerialiseType::Write) {				\
			std::unordered_map<decltype(p)::key_type,nlohmann::json> m;	\
			for(const auto& it : p)	{						\
				nlohmann::json storageJson;					\
				Engine::ToJson(it.second, storageJson);		\
				m[it.first] = storageJson;					\
			}												\
			json[name] = m;									\
		}													\
		else if(op==Engine::SerialiseType::Read) {			\
			std::unordered_map<decltype(p)::key_type, nlohmann::json> allStorage = json[name];	\
			for (auto& it : allStorage)						\
			{												\
				decltype(p)::mapped_type storage;			\
				Engine::FromJson(storage, it.second);		\
				p[it.first] = std::move(storage);			\
			}												\
		}

// add this at the end!
// our last act is to record the class name. we do it last to avoid 
// having parents overwrite it

#define SERIALISE_END()	\
		json["ClassName"] = c_myClassName;	\
	}

#include "serialisation.inl"