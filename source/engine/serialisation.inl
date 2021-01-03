#pragma once
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
	void ToJson(T& v, nlohmann::json& json)
	{
		if constexpr (Serialisation::HasSerialiser<T>::value)	// I'm in love
		{
			v.Serialise(json, SDE::Serialiser::Writer);
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
			v.Serialise(json, SDE::Serialiser::Writer);
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
			std::string classTypeName = json["SDE_ClassName"];
			T* object = Serialisation::ObjectFactory<T>::CreateObject(classTypeName);
			object->Serialise(json, SDE::Serialiser::Reader);
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
			std::string classTypeName = json["SDE_ClassName"];
			T* object = Serialisation::ObjectFactory<T>::CreateObject(classTypeName);
			object->Serialise(json, SDE::Serialiser::Reader);
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
			v.Serialise(json, SDE::Serialiser::Reader);
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

	template<class T>
	void FromJson(const char* name, T& v, nlohmann::json& json)
	{
		if constexpr (Serialisation::HasSerialiser<T>::value)
		{
			v.Serialise(json[name], SDE::Serialiser::Reader);
		}
		else
		{
			v = json[name];
		}
	}
}