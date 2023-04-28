#include "behaviour_tree_renderer.h"
#include "behaviour_tree.h"
#include "behaviour_tree_instance.h"
#include "node.h"
#include "core/profiler.h"
#include "engine/graphics_system.h"
#include "engine/texture_manager.h"
#include "engine/2d_render_context.h"
#include "engine/system_manager.h"
#include "engine/text_system.h"

namespace Behaviours
{
	void BehaviourTreeRenderer::DrawTree(const BehaviourTree& t, const BehaviourTreeInstance* instance)
	{
		SDE_PROF_EVENT();
		auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
		auto textures = Engine::GetSystem<Engine::TextureManager>("Textures");
		auto whiteTex = textures->LoadTexture("white.bmp");
		auto circleTex = textures->LoadTexture("circle_64x64.png");
		auto& r2d = graphics->GetRender2D();
		auto textRender = Engine::GetSystem<Engine::TextSystem>("Text");
		const std::string c_statusText[] = {
			"Not Ran",
			"Success",
			"Failed",
			"Running"
		};
		const glm::vec4 c_statusBorderColour[] = {
			{0.2,0.2,0.2,1},
			{0,1,0,1},
			{1,0,0,1},
			{1,1,0,1}
		};
		const glm::vec2 border(2, 2);
		const Engine::TextSystem::FontData nodeHeaderFont{ "arial.ttf", 18 };
		const Engine::TextSystem::FontData nodeInstanceFont{ "arial.ttf", 14 };
		for (int n = 0; n < t.m_allNodes.size(); ++n)
		{
			const auto& node = *t.m_allNodes[n];
			glm::vec2 nodeDrawDimensions = node.m_editorDimensions;
			glm::vec4 borderColour(1, 1, 1, 1);
			if (instance != nullptr)
			{
				nodeDrawDimensions.y += 16.0f;
				nodeDrawDimensions.x = glm::max(node.m_editorDimensions.x, 150.0f);
				borderColour = c_statusBorderColour[(int)instance->m_nodeContexts[n].m_runningState];
			}
			const glm::vec2 nameTextOffset(8, nodeDrawDimensions.y - 20);
			const glm::vec2 instanceTextOffset(8, nodeDrawDimensions.y - 40);

			r2d.DrawQuad(node.m_editorPosition - border, -1, nodeDrawDimensions + (border * 2.0f), { 0,0 }, { 1,1 }, borderColour, whiteTex);
			r2d.DrawQuad(node.m_editorPosition, 0, nodeDrawDimensions, { 0,0 }, { 1,1 }, glm::vec4(node.m_bgColour,1.0f), whiteTex);

			const auto trd = textRender->GetRenderData(std::string(node.GetTypeName()) + ":" + node.m_name, nodeHeaderFont);
			textRender->DrawText(r2d, trd, node.m_editorPosition + nameTextOffset, 1, { 1,1 }, glm::vec4(node.m_textColour,1.0f));
			
			if (instance != nullptr)
			{
				auto& nodeContext = instance->m_nodeContexts[n];
				std::string instanceText = c_statusText[(int)nodeContext.m_runningState] + "  ";
				instanceText += "Init:" + std::to_string(nodeContext.m_initCount) + " ";
				instanceText += "Tick:" + std::to_string(nodeContext.m_tickCount);
				const auto trd = textRender->GetRenderData(instanceText, nodeInstanceFont);
				textRender->DrawText(r2d, trd, node.m_editorPosition + instanceTextOffset, 1, { 1,1 }, glm::vec4(node.m_textColour, 1.0f));
			}

			const glm::vec2 outConnectionPos = { node.m_editorPosition.x + node.m_editorDimensions.x / 2.0f, node.m_editorPosition.y };
			for (int c = 0; c < node.m_outputPins.size(); ++c)
			{
				Node* connectedNode = t.FindNode(node.m_outputPins[c].m_connectedNodeID);
				if (connectedNode != nullptr)
				{
					const glm::vec2 inConnectionPos(connectedNode->m_editorPosition.x + connectedNode->m_editorDimensions.x / 2.0f,
						connectedNode->m_editorPosition.y + connectedNode->m_editorDimensions.y);
					const glm::vec4 connectionColour(node.m_outputPins[c].m_colour,1.0f);
					r2d.DrawLine(outConnectionPos, inConnectionPos, 2.0f, 2, connectionColour, connectionColour);
				}
			}
		}
	}
}