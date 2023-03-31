#pragma once

#include <memory>
#include "render/render_buffer.h"
#include "render/texture.h"
#include "render/frame_buffer.h"
#include "shader_manager.h"

namespace Render
{
	class FrameBuffer;
	class Device;
	class RenderTargetBlitter;
}

// ref https://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html
namespace Engine
{
	class SSAO
	{
	public:
		bool Init();
		Render::FrameBuffer& GetSSAOFramebuffer() { return *m_ssaoFb[m_currentBuffer]; }
		Render::Texture& GetSSAOTexture() { return m_ssaoFb[m_currentBuffer]->GetColourAttachment(0); }
		bool Update(Render::Device& d, Render::RenderTargetBlitter& blitter, Render::FrameBuffer& gBuffer, Render::RenderBuffer& globals);
	private:
		bool GenerateHemisphereSamples();
		bool GenerateSampleNoise();
		static constexpr int c_totalHemisphereSamples = 32;
		static constexpr int c_sampleNoiseDimensions = 4;
		static constexpr int c_maxSSAOBuffers = 2;
		float m_aoRadius = 16.0f;
		float m_aoBias = 0.4;
		float m_rangeCutoffMulti = 0.25;
		int m_currentBuffer = 0;
		std::unique_ptr<Render::FrameBuffer> m_ssaoFb[c_maxSSAOBuffers];
		Render::Texture m_sampleNoiseTexture;
		Render::RenderBuffer m_hemisphereSamples;
		ShaderHandle m_ssaoShader;
	};
}