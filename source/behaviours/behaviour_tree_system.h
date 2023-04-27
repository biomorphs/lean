#include "engine/system.h"
#include <vector>
#include <string>
#include <functional>

namespace Behaviours
{
	class Node;
	class BehaviourTree;
	class BehaviourTreeSystem : public Engine::System
	{
	public:
		uint16_t AddNode(BehaviourTree& tree, std::string nodeTypeName);
		virtual ~BehaviourTreeSystem() {}
		bool Initialise();
		bool Tick(float timeDelta);
		void Shutdown();

	private:
		struct NodeFactory
		{
			std::string m_nodeName;
			std::function<std::unique_ptr<Node>()> m_fn;
		};
		std::vector<NodeFactory> m_nodeFactories;
		std::vector<std::string> m_nodeFactoryNames;
	};
}