#pragma once
#include <vector>
#include <string>
#include <functional>

namespace Behaviours
{
	class BehaviourTreeInstance;
	class BehaviourTreeDebugger
	{
	public:
		void DebugTreeInstance(BehaviourTreeInstance* inst);
	};
}