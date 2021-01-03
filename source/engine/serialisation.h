#pragma once
#include <nlohmann/json.hpp>

namespace Engine
{
	// Specify read/write from json. I don't like this name
	enum class Serialiser
	{
		Reader,
		Writer
	};
}

// add this to the public interface of your class
#define SERIALISED_CLASS()	\
	void Serialise(nlohmann::json& json, Engine::Serialiser op);

// add this to your cpp, generates serialise function + registers factories
#define SERIALISE_BEGIN(classname)	\
	Engine::Serialisation::ObjectFactory<classname>::Register s_registerFactory_(#classname,[](){ return new classname; });	\
	void classname::Serialise(nlohmann::json& json, Engine::Serialiser op)	\
	{	\
		const char* c_myClassName = #classname;

// add this to your cpp, generates serialise function + registers factories
#define SERIALISE_BEGIN_WITH_PARENT(classname,parent)	\
	Engine::Serialisation::ObjectFactory<parent>::Register s_registerFactory_parent_(#classname,[]() {	\
		return reinterpret_cast<parent*>(new classname);	\
	});	\
	SERIALISE_BEGIN(classname)	\
		parent::Serialise(json,op);	\

// properties (can handle almost anything)
#define SERIALISE_PROPERTY(name,p)	\
		if(op == Engine::Serialiser::Writer) {	\
			Engine::ToJson(name, p, json);	\
		}	\
		else    \
		{	\
			Engine::FromJson(name, p, json);	\
		}

// add this at the end!
// our last act is to record the class name. we do it last to avoid 
// having parents overwrite it

#define SERIALISE_END()	\
		json["ClassName"] = c_myClassName;	\
	}

#include "serialisation.inl"