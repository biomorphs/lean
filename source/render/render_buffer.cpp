#include "render_buffer.h"
#include "utils.h"
#include "core/profiler.h"
#include <GL/glew.h>
#include <cstring>
#include <cassert>

namespace Render
{
	RenderBuffer::RenderBuffer()
		: m_bufferSize(0)
		, m_handle(0)
		, m_persistentMappedBuffer(nullptr)
	{
	}

	RenderBuffer::~RenderBuffer()
	{
		Destroy();
	}

	uint32_t RenderBuffer::TranslateModificationType(RenderBufferModification type) const
	{
		switch (type)
		{
		case RenderBufferModification::Static:
			return GL_STATIC_DRAW;
		case RenderBufferModification::Dynamic:
			return GL_DYNAMIC_DRAW;
		default:
			return -1;
		}
	}

	uint32_t RenderBuffer::TranslateStorageType(RenderBufferModification type) const
	{
		switch (type)
		{
		case RenderBufferModification::Static:
			return 0;
		case RenderBufferModification::Dynamic:
			return GL_DYNAMIC_STORAGE_BIT;
		default:
			return -1;
		}
	}

	bool RenderBuffer::Create(void* sourceData, size_t bufferSize, RenderBufferModification modification, bool usePersistentMapping, bool isReadable)
	{
		SDE_PROF_EVENT();
		assert(bufferSize > 0);

		if (bufferSize > 0)
		{
			{
				SDE_PROF_EVENT("glCreateBuffers");
				glCreateBuffers(1, &m_handle);
			}

			{
				SDE_PROF_EVENT("glNamedBufferStorage");
				assert(!(modification == RenderBufferModification::Static && sourceData == nullptr));
				auto storageType = TranslateStorageType(modification);
				if (usePersistentMapping)
				{
					storageType |= GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
				}
				if (isReadable)
				{
					storageType |= GL_MAP_READ_BIT;
				}
				glNamedBufferStorage(m_handle, bufferSize, sourceData, storageType);
			}

			if (usePersistentMapping)
			{
				SDE_PROF_EVENT("glMapNamedBufferRange");
				GLuint mappingBits = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
				m_persistentMappedBuffer = glMapNamedBufferRange(m_handle, 0, bufferSize, mappingBits);
			}

			m_bufferSize = bufferSize;
		}

		return true;
	}

	bool RenderBuffer::Create(size_t bufferSize, RenderBufferModification modification, bool usePersistentMapping, bool isReadable)
	{
		return Create(nullptr, bufferSize, modification, usePersistentMapping, isReadable);
	}

	void* RenderBuffer::Map(uint32_t hint, size_t offset, size_t size)
	{
		SDE_PROF_EVENT();
		if (m_persistentMappedBuffer)	// persistent buffers are always mapped
		{
			return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_persistentMappedBuffer) + offset);
		}
		else
		{
			GLuint mappingBits = 0;
			if (hint & RenderBufferMapHint::Read)
			{
				mappingBits |= GL_MAP_READ_BIT;
			}
			if (hint & RenderBufferMapHint::Write)
			{
				mappingBits |= GL_MAP_WRITE_BIT;
			}
			void* buffer = glMapNamedBufferRange(m_handle, offset, size, mappingBits);
			return buffer;
		}
	}

	void RenderBuffer::Unmap()
	{
		assert(m_handle != 0);
		if (!m_persistentMappedBuffer)
		{
			glUnmapNamedBuffer(m_handle);
		}
	}

	void RenderBuffer::SetData(size_t offset, size_t size, void* srcData)
	{
		SDE_PROF_EVENT();
		assert(offset < m_bufferSize);
		assert((size + offset) <= m_bufferSize);
		assert(srcData != nullptr);
		assert(m_handle != 0);

		if (m_persistentMappedBuffer != nullptr)
		{
			SDE_PROF_EVENT("WriteToPersistent");
			void* target = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_persistentMappedBuffer) + offset);
			memcpy(target, srcData, size);
		}
		else
		{
			SDE_PROF_EVENT("glNamedBufferSubData");
			glNamedBufferSubData(m_handle, offset, size, srcData);
		}
	}

	bool RenderBuffer::Destroy()
	{
		SDE_PROF_EVENT();
		if (m_persistentMappedBuffer != nullptr)
		{
			glUnmapNamedBuffer(m_handle);
			m_persistentMappedBuffer = nullptr;
		}

		if (m_handle != 0)
		{
			glDeleteBuffers(1, &m_handle);
			m_handle = 0;
			m_bufferSize = 0;
		}

		return true;
	}
}