#pragma once

namespace Behaviours
{
	class BehaviourTree;
	class BehaviourTreeInstance;
	class BehaviourTreeRenderer
	{
	public:
		void DrawTree(const BehaviourTree& t, const BehaviourTreeInstance* instance);
	};
}