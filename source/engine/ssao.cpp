#include "ssao.h"
#include "core/glm_headers.h"
#include "core/random.h"
#include "core/log.h"
#include "render/texture_source.h"
#include "render/window.h"
#include "render/device.h"
#include "render/render_target_blitter.h"
#include "system_manager.h"
#include "render_system.h"
#include "entity/component_inspector.h"
#include <vector>

COMPONENT_SCRIPTS(SSAOSettings,
	"SetRadius", &SSAOSettings::SetRadius,
	"SetDepthBias", &SSAOSettings::SetDepthBias,
	"SetRangeCutoffMulti", &SSAOSettings::SetRangeCutoffMulti,
	"SetPower", &SSAOSettings::SetPower
)

SERIALISE_BEGIN(SSAOSettings)
SERIALISE_PROPERTY("Radius", m_aoRadius)
SERIALISE_PROPERTY("Bias", m_aoBias)
SERIALISE_PROPERTY("CutoffMulti", m_rangeCutoffMulti)
SERIALISE_PROPERTY("Power", m_aoPower)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(SSAOSettings)
{
	auto fn = [](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto c = static_cast<StorageType&>(cs).Find(e);
		i.Inspect("Radius", c->GetRadius(), InspectFn(e, &SSAOSettings::SetRadius));
		i.Inspect("Depth Bias", c->GetDepthBias(), InspectFn(e, &SSAOSettings::SetDepthBias));
		i.Inspect("Range Cutoff Multi", c->GetRangeCutoffMulti(), InspectFn(e, &SSAOSettings::SetRangeCutoffMulti));
		i.Inspect("Power", c->GetPower(), InspectFn(e, &SSAOSettings::SetPower));
	};
	return fn;
}

namespace Engine
{
	void SSAO::ApplySettings(const SSAOSettings& s)
	{
		m_aoRadius = s.GetRadius();
		m_aoBias = s.GetDepthBias();
		m_rangeCutoffMulti = s.GetRangeCutoffMulti();
		m_aoPower = s.GetPower();
	}

	bool SSAO::Update(Render::Device& d, Render::RenderTargetBlitter& blitter, Render::FrameBuffer& gBuffer, Render::RenderBuffer& globals)
	{
		SDE_PROF_EVENT();
		m_currentBuffer = m_currentBuffer + 1 >= c_maxSSAOBuffers ? 0 : m_currentBuffer + 1;

		auto& ssaoFB = m_ssaoFb[m_currentBuffer];
		d.SetWireframeDrawing(false);
		d.DrawToFramebuffer(*ssaoFB);
		d.SetViewport(glm::ivec2(0, 0), ssaoFB->Dimensions());
		d.SetBackfaceCulling(false, true);	// backface culling, ccw order
		d.SetBlending(false);				// no blending for opaques
		d.SetScissorEnabled(false);			// (don't) scissor me timbers
		d.SetDepthState(false, false);		// no depth test/write

		auto shaders = Engine::GetSystem<ShaderManager>("Shaders");
		auto ssaoShader = shaders->GetShader(m_ssaoShader);
		if (ssaoShader)
		{
			d.BindShaderProgram(*ssaoShader);		// must be set before uniforms
			d.BindUniformBufferIndex(*ssaoShader, "Globals", 0);
			d.SetUniforms(*ssaoShader, globals, 0);
			d.BindUniformBufferIndex(*ssaoShader, "SSAO_HemisphereSamples", 1);
			d.SetUniforms(*ssaoShader, m_hemisphereSamples, 1);
			uint32_t noiseHandle = ssaoShader->GetUniformHandle("SSAO_Noise");
			uint32_t noiseScaleHandle = ssaoShader->GetUniformHandle("SSAO_NoiseScale");
			uint32_t gbufferPosHandle = ssaoShader->GetUniformHandle("GBuffer_Pos");
			uint32_t gbufferNormHandle = ssaoShader->GetUniformHandle("GBuffer_NormalShininess");
			uint32_t radiusHandle = ssaoShader->GetUniformHandle("SSAO_Radius");
			uint32_t biasHandle = ssaoShader->GetUniformHandle("SSAO_Bias");
			uint32_t rangeMultiHandle = ssaoShader->GetUniformHandle("SSAO_RangeMulti");
			uint32_t powerMultiHandle = ssaoShader->GetUniformHandle("SSAO_PowerMulti");
			if (gbufferPosHandle != -1)
				d.SetSampler(gbufferPosHandle, gBuffer.GetColourAttachment(0).GetResidentHandle());
			if (gbufferNormHandle != -1)
				d.SetSampler(gbufferNormHandle, gBuffer.GetColourAttachment(1).GetResidentHandle());
			if (noiseHandle != -1)
				d.SetSampler(noiseHandle, m_sampleNoiseTexture.GetResidentHandle());
			if (radiusHandle != -1)
				d.SetUniformValue(radiusHandle, m_aoRadius);
			if (biasHandle != -1)
				d.SetUniformValue(biasHandle, m_aoBias);
			if (rangeMultiHandle != -1)
				d.SetUniformValue(rangeMultiHandle, m_rangeCutoffMulti);
			if (powerMultiHandle != -1)
				d.SetUniformValue(powerMultiHandle, m_aoPower);
			const auto windowSize = Engine::GetSystem<Engine::RenderSystem>("Render")->GetWindow()->GetSize();
			glm::vec2 noiseScale = glm::vec2(windowSize) / float(c_sampleNoiseDimensions);
			if (noiseScaleHandle != -1)
				d.SetUniformValue(noiseScaleHandle, noiseScale);

			blitter.RunOnTarget(d, *ssaoFB, *ssaoShader);
		}

		return true;
	}

	bool SSAO::Init()
	{
		SDE_PROF_EVENT();
		if (!GenerateHemisphereSamples())
		{
			SDE_LOG("Failed to create hemisphere samples");
			return false;
		}

		if (!GenerateSampleNoise())
		{
			SDE_LOG("Failed to create sample noise");
			return false;
		}

		auto shaders = Engine::GetSystem<ShaderManager>("Shaders");
		m_ssaoShader = shaders->LoadShader("SSAO", "basic_blit.vs", "ssao.fs");
		if (m_ssaoShader.m_index == -1)
		{
			SDE_LOG("Failed to load ssao shader");
			return false;
		}

		const auto windowSize = Engine::GetSystem<Engine::RenderSystem>("Render")->GetWindow()->GetSize();
		for (int i = 0; i < c_maxSSAOBuffers; ++i)
		{
			m_ssaoFb[i] = std::make_unique<Render::FrameBuffer>(windowSize);
			m_ssaoFb[i]->AddColourAttachment(Render::FrameBuffer::ColourAttachmentFormat::RGBA_U8);
			m_ssaoFb[i]->Create();
		}
		
		return true;
	}

	float lerpVal(float v0, float v1, float t)
	{
		return v0 + (v1 - v0) * t;
	}

	// generate a small square texture containing random values 
	// this will be tiled over the screen and used to bias sample rotation
	bool SSAO::GenerateSampleNoise()
	{
		SDE_PROF_EVENT();
		constexpr int totalValues = c_sampleNoiseDimensions * c_sampleNoiseDimensions;
		std::vector<glm::vec4> noiseValues(totalValues);
		for (int i = 0; i < totalValues; ++i)
		{
			glm::vec4 v = {
				Core::Random::GetFloat(-1.0f, 1.0f),
				Core::Random::GetFloat(-1.0f, 1.0f),
				0.0f, 0.0f
			};
			v = glm::normalize(v);
			noiseValues[i] = v;
		}
		std::vector<Render::TextureSource::MipDesc> mips;
		mips.push_back({
			c_sampleNoiseDimensions, c_sampleNoiseDimensions,
			0, sizeof(glm::vec4) * totalValues
		});
		Render::TextureSource ts(c_sampleNoiseDimensions, c_sampleNoiseDimensions, 
			Render::TextureSource::Format::RGBAF32, 
			mips, (const uint8_t*)noiseValues.data(), totalValues * sizeof(glm::vec4));
		return m_sampleNoiseTexture.Create(ts);
	}

	// hemisphere oriented around +z
	bool SSAO::GenerateHemisphereSamples()
	{
		SDE_PROF_EVENT();
		std::vector<glm::vec4> samples(c_totalHemisphereSamples);
		for (int i = 0; i < c_totalHemisphereSamples; ++i)
		{
			// generate point on surface of hemisphere
			glm::vec3 sample = { Core::Random::GetFloat(-1.0f, 1.0f),
				Core::Random::GetFloat(-1.0f, 1.0f),
				Core::Random::GetFloat(0.0f, 1.0f) };
			sample = glm::normalize(sample);

			// scale to generate evenly(ish) scattered points
			sample *= Core::Random::GetFloat(0.0f, 1.0f);

			// add falloff to bias points towards origin
			float biasScale = float(i) / float(c_totalHemisphereSamples);
			biasScale = lerpVal(0.2f, 1.0f, biasScale * biasScale);
			sample = sample * biasScale;

			samples[i] = glm::vec4(sample,0.0f);
		}
		return m_hemisphereSamples.Create(samples.data(), sizeof(glm::vec4) * c_totalHemisphereSamples, Render::RenderBufferModification::Static);
	}
}
