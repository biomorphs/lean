#pragma once
#include "serialisation.h"
#include "core/string_hashing.h"
#include <string>
#include <functional>

namespace Engine
{
	class Tag
	{
	public:
		Tag(const char* str)
			: m_string(str)
			, m_hash(Core::StringHashing::GetHash(str))
		{
		}
		Tag() = default;
		Tag(Tag&&) = default;
		Tag(const Tag&) = default;
		Tag& operator=(const Tag& t) = default;
		Tag& operator=(Tag&& t) = default;
		inline bool operator==(const Tag& t) const { return m_hash == t.m_hash; }
		inline bool operator!=(const Tag& t) const { return m_hash != t.m_hash; }

		const char* c_str() const { return m_string.c_str(); }
		uint32_t GetHash() const { return m_hash; }

		SERIALISED_CLASS();
	private:
		std::string m_string;
		uint32_t m_hash;
	};
}

namespace std
{
	template<> struct hash<Engine::Tag>
	{
		std::size_t operator()(Engine::Tag const& t) const noexcept
		{
			return std::hash<uint32_t>{}(t.GetHash());
		}
	};
}