#pragma once

#include <memory>
#include "render/render_buffer.h"
#include "render/texture.h"
#include "render/frame_buffer.h"
#include "shader_manager.h"
#include "entity/component.h"

namespace Render
{
	class FrameBuffer;
	class Device;
	class RenderTargetBlitter;
}

class SSAOSettings
{
public:
	COMPONENT(SSAOSettings);
	COMPONENT_INSPECTOR();

	void SetRadius(float r) { m_aoRadius = r; }
	float GetRadius() const { return m_aoRadius; }
	void SetDepthBias(float r) { m_aoBias = r; }
	float GetDepthBias() const { return m_aoBias; }
	void SetRangeCutoffMulti(float r) { m_rangeCutoffMulti = r; }
	float GetRangeCutoffMulti() const { return m_rangeCutoffMulti; }
	void SetPower(float r) { m_aoPower = r; }
	float GetPower() const { return m_aoPower; }
private:
	float m_aoRadius = 8.0f;
	float m_aoBias = 0.2f;
	float m_rangeCutoffMulti = 0.5;
	float m_aoPower = 1.0f;
};

// ref https://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html
namespace Engine
{
	class SSAO
	{
	public:
		bool Init();
		void ApplySettings(const SSAOSettings& s);
		Render::FrameBuffer& GetSSAOFramebuffer() { return *m_blurFb[m_currentBuffer]; }
		Render::Texture& GetSSAOTexture() { return m_blurFb[m_currentBuffer]->GetColourAttachment(0); }
		bool Update(Render::Device& d, Render::RenderTargetBlitter& blitter, Render::FrameBuffer& gBuffer, Render::RenderBuffer& globals);
		void SetRadius(float r) { m_aoRadius = r; }
		float GetRadius() const { return m_aoRadius; }
		void SetDepthBias(float r) { m_aoBias = r; }
		float GetDepthBias() const { return m_aoBias; }
		void SetRangeCutoffMulti(float r) { m_rangeCutoffMulti = r; }
		float GetRangeCutoffMulti() const { return m_rangeCutoffMulti; }
	private:
		bool GenerateHemisphereSamples();
		bool GenerateSampleNoise();
		static constexpr int c_totalHemisphereSamples = 16;
		static constexpr int c_sampleNoiseDimensions = 4;
		static constexpr int c_maxSSAOBuffers = 2;
		float m_aoRadius = 8.0f;
		float m_aoBias = 0.2f;
		float m_rangeCutoffMulti = 0.5;
		float m_aoPower = 1.0f;
		int m_currentBuffer = 0;
		std::unique_ptr<Render::FrameBuffer> m_ssaoFb[c_maxSSAOBuffers];
		std::unique_ptr<Render::FrameBuffer> m_blurFb[c_maxSSAOBuffers];
		Render::Texture m_sampleNoiseTexture;
		Render::RenderBuffer m_hemisphereSamples;
		ShaderHandle m_ssaoShader;
		ShaderHandle m_blurShader;
	};
}