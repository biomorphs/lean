#include "engine/system.h"
#include "behaviour_tree_instance.h"
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

namespace Behaviours
{
	class Node;
	class BehaviourTree;
	class BehaviourTreeSystem : public Engine::System
	{
	public:
		uint16_t AddNode(BehaviourTree& tree, std::string nodeTypeName);
		virtual ~BehaviourTreeSystem();
		bool Initialise();
		bool Tick(float timeDelta);
		void Shutdown();
		const std::vector<std::string>& GetNodeTypeNames() const { return m_nodeFactoryNames; }
		BehaviourTreeInstance* CreateTreeInstance(std::string_view path);
		void DestroyInstance(BehaviourTreeInstance* instance);
		void OnTreeModified(std::string_view path);	// called from editor if a tree is saved
	private:
		BehaviourTree* LoadTree(std::string_view path);
		struct NodeFactory
		{
			std::string m_nodeName;
			std::function<std::unique_ptr<Node>()> m_fn;
		};
		bool m_debuggerEnabled = false;
		BehaviourTreeInstance* m_debuggingInstance = nullptr;
		std::vector<std::unique_ptr<BehaviourTreeInstance>> m_activeInstances;
		std::vector<NodeFactory> m_nodeFactories;
		std::vector<std::string> m_nodeFactoryNames;
		std::unordered_map<std::string, std::unique_ptr<BehaviourTree>> m_loadedTrees;
	};
}