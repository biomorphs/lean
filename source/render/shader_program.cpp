#include "shader_program.h"
#include "shader_binary.h"
#include "utils.h"
#include "core/string_hashing.h"
#include "core/profiler.h"
#include <memory>

namespace Render
{
	ShaderProgram::ShaderProgram()
		: m_handle(0)
	{
	}

	ShaderProgram::~ShaderProgram()
	{
		Destroy();
	}

	bool ShaderProgram::Create(const ShaderBinary& computeShader, std::string& result)
	{
		SDE_PROF_EVENT();

		SDE_RENDER_ASSERT(computeShader.GetType() == ShaderType::ComputeShader);
		SDE_RENDER_ASSERT(computeShader.GetHandle() != 0);

		m_handle = glCreateProgram();
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCreateProgram");

		glAttachShader(m_handle, computeShader.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glAttachShader");

		glLinkProgram(m_handle);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glLinkProgram");

		// check the results
		int32_t linkResult = 0, logLength = 0;
		glGetProgramiv(m_handle, GL_LINK_STATUS, &linkResult);
		glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0)
		{
			std::unique_ptr<char[]> logResult(new char[logLength]);
			glGetProgramInfoLog(m_handle, logLength, NULL, logResult.get());
			result = logResult.get();
		}

		return linkResult == GL_TRUE;
	}

	bool ShaderProgram::Create(const ShaderBinary& vertexShader, const ShaderBinary& fragmentShader, std::string& result)
	{
		SDE_PROF_EVENT();

		SDE_RENDER_ASSERT(vertexShader.GetType() == ShaderType::VertexShader);
		SDE_RENDER_ASSERT(fragmentShader.GetType() == ShaderType::FragmentShader);
		SDE_RENDER_ASSERT(vertexShader.GetHandle() != 0);
		SDE_RENDER_ASSERT(fragmentShader.GetHandle() != 0);

		m_handle = glCreateProgram();
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCreateProgram");

		glAttachShader(m_handle, vertexShader.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glAttachShader");

		glAttachShader(m_handle, fragmentShader.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glAttachShader");

		glLinkProgram(m_handle);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glLinkProgram");

		// check the results
		int32_t linkResult = 0, logLength = 0;
		glGetProgramiv(m_handle, GL_LINK_STATUS, &linkResult);
		glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0)
		{
			std::unique_ptr<char[]> logResult(new char[logLength]);
			glGetProgramInfoLog(m_handle, logLength, NULL, logResult.get());
			result = logResult.get();
		}
		
		return linkResult == GL_TRUE;
	}

	uint32_t ShaderProgram::GetUniformHandle(const char* uniformName)
	{
		return GetUniformHandle(uniformName, Core::StringHashing::GetHash(uniformName));
	}

	uint32_t ShaderProgram::GetUniformBufferBlockIndex(const char* bufferName) const
	{
		uint32_t index = glGetUniformBlockIndex(m_handle, bufferName);
		SDE_RENDER_PROCESS_GL_ERRORS("glGetUniformBlockIndex");
		return index;
	}

	uint32_t ShaderProgram::GetUniformHandle(const char* uniformName, uint32_t nameHash)
	{
		uint32_t foundHandle = -1;
		auto it = m_uniformHandles.find(nameHash);
		if (it == m_uniformHandles.end())
		{
			foundHandle = glGetUniformLocation(m_handle, uniformName);
			m_uniformHandles[nameHash] = foundHandle;	// we even cache missing uniforms as its faster to hit the map
		}
		else
		{
			foundHandle = it->second;
		}
		return foundHandle;
	}

	void ShaderProgram::Destroy()
	{
		SDE_PROF_EVENT();
		if (m_handle != 0)
		{
			glDeleteProgram(m_handle);
			SDE_RENDER_PROCESS_GL_ERRORS("glDeleteProgram");
		}
		
		m_handle = 0;
	}
}
