#pragma once

#include <stdint.h>

namespace Render
{
	enum class RenderBufferModification : uint32_t
	{
		Static,
		Dynamic
	};

	enum RenderBufferMapHint : uint32_t
	{
		Write = 1,
		Read = 2,
	};

	// This represents a single stream of rendering data
	// It could be indices, vertices, whatever
	class RenderBuffer
	{
	public:
		RenderBuffer();
		~RenderBuffer();

		bool Create(size_t bufferSize, RenderBufferModification modification, bool usePersistentMapping=false, bool isReadable=false);
		bool Create(void* sourceData, size_t bufferSize, RenderBufferModification modification, bool usePersistentMapping = false, bool isReadable = false);
		bool Destroy();
		void SetData(size_t offset, size_t size, void* srcData);
		void* Map(uint32_t hint, size_t offset, size_t size);
		void Unmap();

		inline uint32_t GetHandle() const { return m_handle; }
		inline size_t GetSize() const { return m_bufferSize; }

	private:
		uint32_t TranslateStorageType(RenderBufferModification type) const;
		uint32_t TranslateModificationType(RenderBufferModification type) const;
		size_t m_bufferSize;	// size in bytes
		uint32_t m_handle;
		void* m_persistentMappedBuffer;	// if set, read/write directly
	};
}