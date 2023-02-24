#include "text_system.h"
#include "core/log.h"
#include "system_manager.h"
#include "2d_render_context.h"
#include <ft2build.h>
#include <freetype/freetype.h>
#include <robin_hood.h>

namespace Engine
{
	struct TextSystem::FreetypeLibData
	{
		FT_Library m_library;
		struct Glyph
		{
			TextureHandle m_texture;
			glm::vec2 m_uv0;	// uvs for sampling
			glm::vec2 m_uv1;

			glm::ivec2 m_dimensions;	// in pixels
			glm::ivec2 m_bearing;		// offset relative to baseline
			float m_advance;			// distance to next character in pixels
		};
		struct Font
		{
			FT_Face m_face;
			robin_hood::unordered_map<uint32_t, Glyph> m_glyphs;
		};

		Font* GetFont(std::string_view fontFullName, const FontData& f)
		{
			std::string fontHashName = std::string(f.m_fontPath) + "_" + std::to_string(f.m_heightInPixels);
			auto foundFont = m_loadedFonts.find(fontHashName);
			if (foundFont == m_loadedFonts.end())
			{
				FreetypeLibData::Font newFontData;
				if (FT_New_Face(m_library, f.m_fontPath.data(), 0, &newFontData.m_face) != 0)
				{
					SDE_LOG("Failed to load font '%s'", f.m_fontPath.data());
					return nullptr;
				}
				FT_Set_Pixel_Sizes(newFontData.m_face, 0, f.m_heightInPixels);
				m_loadedFonts[fontHashName] = newFontData;
				foundFont = m_loadedFonts.find(fontHashName);
			}
			return &foundFont->second;
		}

		Glyph* GetGlyph(std::string_view fontFullName, Font* f, uint32_t c)
		{
			// find or create the glyph for each character
			auto& glyphs = f->m_glyphs;
			auto& face = f->m_face;
			auto foundGlyph = glyphs.find(c);
			if (foundGlyph == glyphs.end())
			{
				auto textures = Engine::GetSystem<TextureManager>("Textures");
				if (FT_Load_Char(face, c, FT_LOAD_RENDER) == 0)	// FT_LOAD_RENDER populates face->glyph->bitmap
				{
					const uint32_t glyphWidth = face->glyph->bitmap.width;
					const uint32_t glyphHeight = face->glyph->bitmap.rows;
					std::vector<Render::TextureSource::MipDesc> mips;
					mips.push_back({ glyphWidth, glyphHeight, 0, glyphWidth * glyphHeight });
					Render::TextureSource glyphTexData(glyphWidth, glyphHeight, Render::TextureSource::Format::R8, mips, face->glyph->bitmap.buffer, glyphWidth * glyphHeight);
					glyphTexData.SetGenerateMips(false);
					glyphTexData.SetWrapMode(Render::TextureSource::WrapMode::ClampToEdge, Render::TextureSource::WrapMode::ClampToEdge);
					glyphTexData.SetDataRowAlignment(1);
					Render::Texture newTexture;
					if (!newTexture.Create(glyphTexData))
					{
						SDE_LOG("Failed to create glyph '%c' texture", c);
						return nullptr;
					}
					std::string glyphTexName = std::string(fontFullName) + "_" + std::to_string(c);
					FreetypeLibData::Glyph newGlyph;
					newGlyph.m_texture = textures->AddTexture(glyphTexName, std::move(newTexture));
					newGlyph.m_uv0 = { 0,1 };
					newGlyph.m_uv1 = { 1,0 };
					newGlyph.m_dimensions = { glyphWidth, glyphHeight };
					newGlyph.m_bearing = { face->glyph->bitmap_left, face->glyph->bitmap_top - glyphHeight};
					newGlyph.m_advance = face->glyph->advance.x / 64.0f;		// freetype advance is in pixel/64
					glyphs[c] = newGlyph;
					foundGlyph = glyphs.find(c);
				}
			}
			return &foundGlyph->second;
		}

		robin_hood::unordered_map<std::string, Font> m_loadedFonts;
	};

	TextSystem::TextSystem()
	{

	}

	TextSystem::~TextSystem()
	{

	}

	bool TextSystem::PreInit()
	{
		SDE_PROF_EVENT(); 

		m_freetype = std::make_unique<FreetypeLibData>();
		if (FT_Init_FreeType(&m_freetype->m_library))
		{
			SDE_LOG("Failed to initialise Freetype");
			return false;
		}

		return true;
	}

	void TextSystem::PostShutdown()
	{
		SDE_PROF_EVENT();

		FT_Done_FreeType(m_freetype->m_library);
		m_freetype = nullptr;
	}

	void TextSystem::DrawText(RenderContext2D& r2d, const TextRenderData& trd, glm::vec2 position, int zIndex, glm::vec2 scale, glm::vec4 colour)
	{
		for (const auto& g : trd.m_glyphs)
		{
			r2d.DrawQuad(g.m_position + position, zIndex, g.m_size * scale, g.m_uv0, g.m_uv1, colour, g.m_texture);
		}
	}

	TextSystem::TextRenderData TextSystem::GetRenderData(std::string_view text, const FontData& font)
	{
		SDE_PROF_EVENT();
		TextRenderData trd;

		std::string fontFullName = std::string(font.m_fontPath) + "_" + std::to_string(font.m_heightInPixels);

		// find or create the font (face)
		FreetypeLibData::Font* foundFont = m_freetype->GetFont(fontFullName, font);
		if (foundFont == nullptr)
		{
			return trd;
		}
		
		float xAdvance = 0.0f;	// baseline x position
		const float yBaseline = 0.0f;
		for (const auto& c : text)
		{
			FreetypeLibData::Glyph* glyphData = m_freetype->GetGlyph(fontFullName, foundFont, c);
			if (glyphData)
			{
				TextRenderData::Glyph newGlyph;
				newGlyph.m_texture = glyphData->m_texture;
				newGlyph.m_position = glm::vec2(xAdvance, yBaseline) + glm::vec2(glyphData->m_bearing);
				newGlyph.m_size = glyphData->m_dimensions;
				newGlyph.m_uv0 = glyphData->m_uv0;
				newGlyph.m_uv1 = glyphData->m_uv1;
				trd.m_boundsMin = glm::min(trd.m_boundsMin, newGlyph.m_position);
				trd.m_boundsMax = glm::max(trd.m_boundsMax, newGlyph.m_position + newGlyph.m_size);
				trd.m_glyphs.push_back(newGlyph);
				xAdvance += glyphData->m_advance;
			}
		}

		return trd;
	}
}