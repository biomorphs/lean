#pragma once
#include "core/glm_headers.h"
#include <memory>
#include <functional>

// Octree of SDF meshes with each level representing LODs
// Root node = lowest LOD, leaf nodes = highest

namespace Render
{
	class Mesh;
}

namespace Engine
{
	class SDFMeshOctree
	{
	public:
		SDFMeshOctree();
		~SDFMeshOctree();
		using NodeIndex = uint64_t;													// identify a node in the tree

		using ShouldDrawFn = std::function<bool(glm::vec3, glm::vec3, uint32_t)>;	// bounds min/max, depth, return true if this node should be drawn
		using DrawFn = std::function<void(glm::vec3, glm::vec3, Render::Mesh&)>;	// bounds min/max, mesh, draw whatever is passed to you (or not)
		using ShouldUpdateFn = std::function<bool(glm::vec3, glm::vec3, uint32_t)>;	// bounds min/max, depth, return true if this node should be updated
		// bounds, depth, node id-used to request updates
		// (updater calls SignalNodeUpdating and then SetNodeData when finished)
		using UpdateFn = std::function<void(glm::vec3, glm::vec3, uint32_t, uint64_t)>;	

		void Update(ShouldUpdateFn shouldUpdate, UpdateFn update, ShouldDrawFn shouldDraw, DrawFn draw);
		void SignalNodeUpdating(uint64_t node);										// user should call this once when building new data
		void SetNodeData(uint64_t node, std::unique_ptr<Render::Mesh>&& m);			// pass null if no mesh was generated to stop the updates for this node
		void SetBounds(glm::vec3 min, glm::vec3 max);
		void SetMaxDepth(uint32_t maxDepth);
		void Invalidate(bool destroyAll=false);

	private:
		class Node
		{
		public:
			bool m_isBuilding = false;
			uint32_t m_nodeGeneration = 0;	// used to track staleness of data
			NodeIndex m_index = -1;
			std::unique_ptr<Render::Mesh> m_mesh;
			std::unique_ptr<Node> m_children[8];
		};
		std::unique_ptr<Node> MakeNode();
		void GetNodeDimensions(glm::vec3 parentMin, glm::vec3 parentMax, uint32_t childIndex, glm::vec3& bMin, glm::vec3& bMax);
		bool NodeIsStale(const Node& n);
		void Update(Node& n, uint32_t depth, glm::vec3 boundsMin, glm::vec3 boundsMax, ShouldUpdateFn shouldUpdate, UpdateFn update);
		void Render(Node& n, uint32_t depth, glm::vec3 boundsMin, glm::vec3 boundsMax, ShouldDrawFn shouldDraw, DrawFn draw);
		glm::vec3 m_minBounds;
		glm::vec3 m_maxBounds;
		uint32_t m_maxDepth = 5;		// depth 0 = root
		uint32_t m_currentGeneration = 0;
		NodeIndex m_nextNodeIndex = 0;
		std::unique_ptr<Node> m_root;
		std::unordered_map<NodeIndex, Node*> m_lookupByIndex;
	};
}