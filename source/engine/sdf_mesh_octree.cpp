#include "sdf_mesh_octree.h"
#include "render/mesh.h"
#include "core/profiler.h"
#include <array>

namespace Engine
{

	SDFMeshOctree::SDFMeshOctree()
	{
	}

	SDFMeshOctree::~SDFMeshOctree()
	{
	}

	std::unique_ptr<SDFMeshOctree::Node> SDFMeshOctree::MakeNode()
	{
		SDE_PROF_EVENT();
		auto node = std::make_unique<Node>();
		node->m_index = ++m_nextNodeIndex;
		m_lookupByIndex[node->m_index] = node.get();
		return node;
	}

	void SDFMeshOctree::GetNodeDimensions(glm::vec3 parentMin, glm::vec3 parentMax, uint32_t childIndex, glm::vec3& bMin, glm::vec3& bMax)
	{
		auto dims = (parentMax - parentMin) * 0.5f;
		float xMul = (float)(childIndex % 2);
		float yMul = (float)(childIndex / 4);
		float zMul = (childIndex == 2 || childIndex == 3 || childIndex == 6 || childIndex == 7) ? 1.0f : 0.0f;
		bMin = parentMin + dims * glm::vec3(xMul, yMul, zMul);
		bMax = bMin + dims;
	}

	bool SDFMeshOctree::NodeIsStale(const Node& n)
	{
		return n.m_nodeGeneration != m_currentGeneration;
	}

	void SDFMeshOctree::Render(Node& n, uint32_t depth, glm::vec3 boundsMin, glm::vec3 boundsMax, ShouldDrawFn shouldDraw, DrawFn draw)
	{
		SDE_PROF_EVENT();
		if (!shouldDraw(boundsMin, boundsMax, depth))
		{
			return;
		}

		// Find how many visible children are fully loaded
		int childCount = 0;
		bool allVisibleReady = true;
		std::array <std::tuple <Node*, glm::vec3, glm::vec3>, 8> childrenToDraw;
		if (depth + 1 < m_maxDepth)
		{
			for (uint32_t i = 0; i < 8; ++i)
			{
				if (n.m_children[i])
				{
					// the child is up to date + ready to draw
					if (n.m_children[i]->m_mesh != nullptr || !NodeIsStale(*n.m_children[i]))
					{
						glm::vec3 nodeMin, nodeMax;
						GetNodeDimensions(boundsMin, boundsMax, i, nodeMin, nodeMax);
						if (shouldDraw(nodeMin, nodeMax, depth + 1))
						{
							childrenToDraw[childCount++] = std::make_tuple(n.m_children[i].get(), nodeMin, nodeMax);
						}
						else
						{
							allVisibleReady = false;
							break;
						}
					}
					else
					{
						allVisibleReady = false;
						break;
					}
				}
			}
		}

		// If there are no children loaded, or they dont all have meshes, draw this node only
 		if((childCount == 0 || !allVisibleReady))
		{
			if (n.m_mesh != nullptr)
			{
				draw(boundsMin, boundsMax, *n.m_mesh);
			}
		}
		else
		{
			for (uint32_t c = 0; c < childCount; ++c)
			{
				auto [theNode, nodeMin, nodeMax] = childrenToDraw[c];
				Render(*theNode, depth + 1, nodeMin, nodeMax, shouldDraw, draw);
			}
		}
	}

	void SDFMeshOctree::Update(Node& n, uint32_t depth, glm::vec3 boundsMin, glm::vec3 boundsMax, ShouldUpdateFn shouldUpdate, UpdateFn update)
	{
		if (!n.m_isBuilding && NodeIsStale(n))
		{
			update(boundsMin, boundsMax, depth, n.m_index);
		}

		// go depth first through all children
		if (depth + 1 < m_maxDepth)
		{
			for (uint32_t i = 0; i < 8; ++i)
			{
				glm::vec3 nodeMin, nodeMax;
				GetNodeDimensions(boundsMin, boundsMax, i, nodeMin, nodeMax);
				if (shouldUpdate(nodeMin, nodeMax, depth + 1))
				{
					if (n.m_children[i] == nullptr)
					{
						n.m_children[i] = MakeNode();
					}
					Update(*n.m_children[i], depth + 1, nodeMin, nodeMax, shouldUpdate, update);
				}
			}
		}
	}

	void SDFMeshOctree::Update(ShouldUpdateFn shouldUpdate, UpdateFn update, ShouldDrawFn shouldDraw, DrawFn draw)
	{
		SDE_PROF_EVENT();
		if (m_root == nullptr)
		{
			m_root = MakeNode();
		}
		Update(*m_root, 0, m_minBounds, m_maxBounds, shouldUpdate, update);
		Render(*m_root, 0, m_minBounds, m_maxBounds, shouldDraw, draw);
	}

	void SDFMeshOctree::SignalNodeUpdating(uint64_t node)
	{
		SDE_PROF_EVENT();
		auto found = m_lookupByIndex.find(node);
		assert(found != m_lookupByIndex.end());
		if (found != m_lookupByIndex.end())
		{
			found->second->m_isBuilding = true;
		}
	}

	void SDFMeshOctree::SetNodeData(NodeIndex node, std::unique_ptr<Render::Mesh>&& m)
	{
		SDE_PROF_EVENT();
		auto found = m_lookupByIndex.find(node);
		assert(found != m_lookupByIndex.end());
		if (found != m_lookupByIndex.end())
		{
			found->second->m_mesh = std::move(m);
			found->second->m_isBuilding = false;
			found->second->m_nodeGeneration = m_currentGeneration;
		}
	}

	void SDFMeshOctree::SetBounds(glm::vec3 minb, glm::vec3 maxb)
	{
		m_minBounds = minb;
		m_maxBounds = maxb;
		Invalidate();
	}
	
	void SDFMeshOctree::SetMaxDepth(uint32_t maxDepth)
	{
		m_maxDepth = maxDepth;
		Invalidate();
	}

	void SDFMeshOctree::Invalidate(bool destroyAll)
	{
		SDE_PROF_EVENT();
		if (destroyAll)
		{
			m_root = nullptr;
			m_lookupByIndex.clear();
		}
		++m_currentGeneration;
	}
}