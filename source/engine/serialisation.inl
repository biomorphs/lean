#pragma once
#include "core/glm_headers.h"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <functional>

namespace Engine
{
	namespace Serialisation
	{
		template <typename T>		// SFINAE trick to detect Serialise member fn
		class HasSerialiser
		{
			typedef char one;
			typedef long two;

			template <typename C> static one test(decltype(&C::Serialise));
			template <typename C> static two test(...);

		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template<class T>
		class ObjectFactory
		{
		public:
			using Creator = std::function<T* ()>;
			static T* CreateObject(std::string className)
			{
				auto factory = Factories()[className];
				return factory();
			}
			class Register
			{
			public:
				explicit Register(std::string className, Creator c)
				{
					Factories().insert({ className, c });
				}
			};
		private:
			static std::unordered_map<std::string, Creator>& Factories()
			{
				static std::unordered_map<std::string, Creator> s_factories;
				return s_factories;
			}
		};
	}

	template<class T>
	void ToJson(const std::unique_ptr<T>& v, nlohmann::json& json)
	{
		if constexpr (Serialisation::HasSerialiser<T>::value)
		{
			v->Serialise(json, Engine::SerialiseType::Write);
		}
		else
		{
			json = v;
		}
	}

	template<class T>
	void ToJson(std::unique_ptr<T>& v, nlohmann::json& json)
	{
		if constexpr (Serialisation::HasSerialiser<T>::value)
		{
			v->Serialise(json, Engine::SerialiseType::Write);
		}
		else
		{
			json = v;
		}
	}

	inline void ToJson(const char* name, glm::mat4& v, nlohmann::json& json)
	{
		float values[16] = {	v[0].x, v[0].y, v[0].z, v[0].w,
								v[1].x, v[1].y, v[1].z, v[1].w,
								v[2].x, v[2].y, v[2].z, v[2].w ,
								v[3].x, v[3].y, v[3].z, v[3].w };
		json[name] = values;
	}

	inline void ToJson(const char* name, glm::quat& v, nlohmann::json& json)
	{
		float values[4] = { v.x, v.y, v.z, v.w };
		json[name] = values;
	}

	inline void ToJson(const char* name, glm::vec4& v, nlohmann::json& json)
	{
		float values[4] = { v.x, v.y, v.z, v.w };
		json[name] = values;
	}

	inline void ToJson(const char* name, glm::vec3& v, nlohmann::json& json)
	{
		float values[3] = { v.x, v.y, v.z };
		json[name] = values;
	}

	inline void ToJson(const char* name, glm::vec2& v, nlohmann::json& json)
	{
		float values[2] = { v.x, v.y };
		json[name] = values;
	}

	inline void ToJson(const char* name, glm::ivec2& v, nlohmann::json& json)
	{
		int values[2] = { v.x, v.y };
		json[name] = values;
	}

	template<class T>
	void ToJson(T& v, nlohmann::json& json)
	{
		if constexpr (Serialisation::HasSerialiser<T>::value)	// I'm in love
		{
			v.Serialise(json, Engine::SerialiseType::Write);
		}
		else
		{
			json = v;
		}
	}

	template<class T>
	void ToJson(T* v, nlohmann::json& json)
	{
		if constexpr (Serialisation::HasSerialiser<T>::value)
		{
			v->Serialise(json, Engine::SerialiseType::Write);
		}
		else
		{
			json = v;
		}
	}

	template<class T>
	void ToJson(const char* name, T& v, nlohmann::json& json)
	{
		json[name] = v;
	}

	template<class T>
	void ToJson(const char* name, std::vector<T>& v, nlohmann::json& json)
	{
		std::vector<nlohmann::json> listJson;
		for (auto& it : v)
		{
			nlohmann::json itJson;
			ToJson(it, itJson);
			listJson.push_back(std::move(itJson));
		}
		json[name] = std::move(listJson);
	}

	///////////////////////////////////////////////

	template<class T>
	void FromJson(std::shared_ptr<T>& v, nlohmann::json& json)
	{
		if constexpr (Serialisation::HasSerialiser<T>::value)
		{
			// Get the actual class name from json
			std::string classTypeName = json["ClassName"];
			T* object = Serialisation::ObjectFactory<T>::CreateObject(classTypeName);
			object->Serialise(json, Engine::SerialiseType::Read);
			v.reset(object);
		}
		else
		{
			v = std::make_shared<T>();
			*v = json;
		}
	}

	template<class T>
	void FromJson(std::unique_ptr<T>& v, nlohmann::json& json)
	{
		if constexpr (Serialisation::HasSerialiser<T>::value)
		{
			// Get the actual class name from json
			std::string classTypeName = json["ClassName"];
			T* object = Serialisation::ObjectFactory<T>::CreateObject(classTypeName);
			object->Serialise(json, Engine::SerialiseType::Read);
			v.reset(object);
		}
		else
		{
			v = std::make_unique<T>();
			*v = json;
		}
	}

	template<class T>
	void FromJson(T& v, nlohmann::json& json)
	{
		if constexpr (Serialisation::HasSerialiser<T>::value)
		{
			v.Serialise(json, Engine::SerialiseType::Read);
		}
		else
		{
			v = json;
		}
	}

	template<class T>
	void FromJson(const char* name, std::vector<T>& v, nlohmann::json& json)
	{
		auto& jj = json[name];
		v.reserve(jj.size());
		for (auto it : jj)
		{
			T newVal;
			FromJson(newVal, it);
			v.push_back(std::move(newVal));
		}
	}

	inline void FromJson(const char* name, glm::ivec2& v, nlohmann::json& json)
	{
		auto& vjson = json[name];
		if (json.size() == 2)
		{
			int x = vjson[0];
			int y = vjson[1];
			v = { x, y };
		}
	}

	inline void FromJson(const char* name, glm::vec2& v, nlohmann::json& json)
	{
		auto& vjson = json[name];
		if (json.size() == 2)
		{
			float x = vjson[0];
			float y = vjson[1];
			v = { x, y };
		}
	}

	inline void FromJson(const char* name, glm::vec3& v, nlohmann::json& json)
	{
		auto& vjson = json[name];
		if (json.size() == 3)
		{
			float x = vjson[0];
			float y = vjson[1];
			float z = vjson[2];
			v = { x, y, z };
		}
	}

	inline void FromJson(const char* name, glm::quat& v, nlohmann::json& json)
	{
		auto& vjson = json[name];
		if (json.size() == 4)
		{
			float x = vjson[0];
			float y = vjson[1];
			float z = vjson[2];
			float w = vjson[3];
			v = { x, y, z, w };
		}
	}

	inline void FromJson(const char* name, glm::vec4& v, nlohmann::json& json)
	{
		auto& vjson = json[name];
		if (json.size() == 4)
		{
			float x = vjson[0];
			float y = vjson[1];
			float z = vjson[2];
			float w = vjson[3];
			v = { x, y, z, w };
		}
	}

	inline void FromJson(const char* name, glm::mat4& v, nlohmann::json& json)
	{
		auto& vjson = json[name];
		if (json.size() == 16)
		{
			v[0] = glm::vec4(json[0], json[1], json[2], json[3]);
			v[1] = glm::vec4(json[4], json[5], json[6], json[7]);
			v[2] = glm::vec4(json[8], json[9], json[10], json[11]);
			v[3] = glm::vec4(json[12], json[13], json[14], json[15]);
		}
	}

	template<class T>
	void FromJson(const char* name, T& v, nlohmann::json& json)
	{
		if constexpr (Serialisation::HasSerialiser<T>::value)
		{
			v.Serialise(json[name], Engine::SerialiseType::Read);
		}
		else
		{
			v = json[name];
		}
	}
}