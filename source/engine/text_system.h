#pragma once
#include "system.h"
#include "texture_manager.h"
#include <memory>
#include <string_view>

namespace Engine
{
	class RenderContext2D;
	class TextSystem : public System
	{
	public:
		TextSystem();
		virtual ~TextSystem();
		bool PreInit();
		void PostShutdown();
		struct FontData
		{
			std::string_view m_fontPath;
			int m_heightInPixels = 0;
		};
		struct TextRenderData
		{
			struct Glyph {
				TextureHandle m_texture;
				glm::vec2 m_position;
				glm::vec2 m_size;
				glm::vec2 m_uv0;
				glm::vec2 m_uv1;
			};
			std::vector<Glyph> m_glyphs;
			glm::vec2 m_boundsMin = { -FLT_MAX, -FLT_MAX };
			glm::vec2 m_boundsMax = { FLT_MAX, FLT_MAX };
		};
		TextRenderData GetRenderData(std::string_view text, const FontData& font);
		void DrawText(RenderContext2D& r2d, const TextRenderData& trd, glm::vec2 position, int zIndex, glm::vec2 scale, glm::vec4 colour);
	private:
		struct FreetypeLibData;	// pimpl
		std::unique_ptr<FreetypeLibData> m_freetype;
	};
}