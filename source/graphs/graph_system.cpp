#include "graph_system.h"
#include "core/log.h"
#include "core/profiler.h"
#include "pin_descriptor.h"
#include "node.h"
#include "graph.h"
#include "graph_context.h"

class ConstantNode : public Graphs::Node
{
public:
	ConstantNode(int value = 0)
		: m_value(value)
	{
		m_name = "Constant<int>";
		m_isConstant = true;
		AddOutputPin(0, "Value", "int");
	}
	ExecResult Execute(Graphs::GraphContext& ctx) override
	{
		SDE_PROF_EVENT();
		ctx.WritePin(0, m_value);
		return ExecResult::Completed;
	}
	int m_value;
};

class AddIntsNode : public Graphs::Node
{
public:
	AddIntsNode()
	{
		m_name = "AddNumbers<int>";
		AddInputPin(0, "Execute", "Execution");
		AddInputPin(1, "Value1", "int");
		AddInputPin(2, "Value2", "int");
		AddOutputPin(3, "RunNext", "Execution");
		AddOutputPin(4, "Result", "int");
	}
	ExecResult Execute(Graphs::GraphContext& ctx) override
	{
		SDE_PROF_EVENT();
		int v1 = ctx.ReadPin(1);
		int v2 = ctx.ReadPin(2);
		ctx.WritePin(4, v1 + v2);
		ctx.ExecuteNext(GetID(), 3);	// run the next execution pin 
		return ExecResult::Completed;
	}
};

class IntGreaterThan : public Graphs::Node
{
public:
	IntGreaterThan()
	{
		m_name = "GreaterThan<int>";
		AddInputPin(0, "Execute", "Execution");
		AddInputPin(1, "Value1", "int");
		AddInputPin(2, "Value2", "int");
		AddOutputPin(3, "RunIfTrue", "Execution");
		AddOutputPin(4, "RunIfFalse", "Execution");
	}
	ExecResult Execute(Graphs::GraphContext& ctx) override
	{
		SDE_PROF_EVENT();
		int v1 = ctx.ReadPin(1);
		int v2 = ctx.ReadPin(2);
		if (v1 > v2)
		{
			ctx.ExecuteNext(GetID(), 3);
		}
		else
		{
			ctx.ExecuteNext(GetID(), 4);
		}
		return ExecResult::Completed;
	}
};

class PrintIntNode : public Graphs::Node
{
public:
	PrintIntNode(std::string_view prefix = "Value = ")
		: m_prefix(prefix)
	{
		m_name = "PrintValue<int>";
		AddInputPin(0, "Execute", "Execution");
		AddInputPin(1, "Value", "int");
		AddOutputPin(2, "RunNext", "Execution");
	}
	ExecResult Execute(Graphs::GraphContext& ctx) override
	{
		SDE_PROF_EVENT();
		int value = ctx.ReadPin(1);
		SDE_LOG("%s%d", m_prefix.c_str(), value);
		ctx.ExecuteNext(GetID(), 2);	// run the next execution pin 
		return ExecResult::Completed;
	}
	std::string m_prefix;
};

Graphs::Graph g_testGraph;

void IncrementOneValueABunch()
{
	SDE_PROF_EVENT();

	Graphs::Graph g;

	{
		SDE_PROF_EVENT("Build Graph");
		g.AddInputPin(0, "Exec", "Execution");
		g.AddInputPin(1, "StartValue", "int");
		g.AddOutputPin(2, "RunNext", "Execution");
		g.AddOutputPin(3, "Result", "int");

		int incrementConstant = g.AddNode(new ConstantNode(1));

		int lastExecNode = -1;
		int lastExecPin = 0;
		int lastResultNode = -1;
		int lastResultPin = 1;
		constexpr bool DoPrint = false;
		for (int i = 0; i < 1000; ++i)
		{
			int thisAddNode = g.AddNode(new AddIntsNode);
			if constexpr (DoPrint)
			{
				int thisPrintNode = g.AddNode(new PrintIntNode);

				// connect last exec to this one + set up next one
				g.AddConnection(lastExecNode, lastExecPin, thisAddNode, 0);
				g.AddConnection(thisAddNode, 3, thisPrintNode, 0);
				lastExecNode = thisPrintNode;
				lastExecPin = 2;	// print node output

				// hook up data connections
				g.AddConnection(lastResultNode, lastResultPin, thisAddNode, 1);		// last result value
				g.AddConnection(incrementConstant, 0, thisAddNode, 2);				// + increment constant
				g.AddConnection(thisAddNode, 4, thisPrintNode, 1);					// add -> print
				lastResultNode = thisAddNode;
				lastResultPin = 4;	// add node result -> next cycle
			}
			else
			{
				// connect last exec to this one + set up next one
				g.AddConnection(lastExecNode, lastExecPin, thisAddNode, 0);
				lastExecNode = thisAddNode;
				lastExecPin = 3;

				// hook up data connections
				g.AddConnection(lastResultNode, lastResultPin, thisAddNode, 1);		// last result value
				g.AddConnection(incrementConstant, 0, thisAddNode, 2);				// + increment constant
				lastResultNode = thisAddNode;
				lastResultPin = 4;	// add node result -> next cycle
			}
		}
		// connect last node to the graph output
		g.AddConnection(lastExecNode, lastExecPin, -1, 2);
		g.AddConnection(lastResultNode, lastResultPin, -1, 3);
	}

	{
		SDE_PROF_EVENT("RunContext");
		Graphs::GraphContext runContext(&g);
		runContext.WriteGraphPin(-1, 1, 1);		// start value
		runContext.ExecuteNext(-1, 0);			// execute graph pin 0
		bool graphRunning = true;
		{
			SDE_PROF_EVENT("RunSteps");
			while (graphRunning)
			{
				auto stepResult = runContext.RunSingleStep();
				if (stepResult == Graphs::GraphContext::Completed)
				{
					SDE_LOG("Graph finished ok");
					int resultValue = runContext.ReadGraphPin(-1, 3);
					SDE_LOG("\tresult = %d", resultValue);
					graphRunning = false;
				}
			}
		}
	}
}

bool GraphSystem::Initialise()
{
	SDE_PROF_EVENT();

	g_testGraph.AddInputPin(0, "Execute", "Execution");
	g_testGraph.AddInputPin(1, "TestValue", "int");
	g_testGraph.AddOutputPin(2, "RunNext", "Execution");
	g_testGraph.AddOutputPin(3, "Result", "int");

	uint16_t c0id = g_testGraph.AddNode(new ConstantNode(3));
	uint16_t printC0Id = g_testGraph.AddNode(new PrintIntNode(""));
	uint16_t c1id = g_testGraph.AddNode(new ConstantNode(5));
	uint16_t printC1Id = g_testGraph.AddNode(new PrintIntNode(" + "));
	uint16_t addIntId = g_testGraph.AddNode(new AddIntsNode);
	uint16_t printResultId = g_testGraph.AddNode(new PrintIntNode(" = "));
	uint16_t igtId = g_testGraph.AddNode(new IntGreaterThan());
	uint16_t printGtId = g_testGraph.AddNode(new PrintIntNode("which is greater than "));
	uint16_t printNotGtId = g_testGraph.AddNode(new PrintIntNode("which is not greater than "));

	// exec flow
	g_testGraph.AddConnection(-1, 0, printC0Id, 0);				// graph execute -> c0 print
	g_testGraph.AddConnection(printC0Id, 2, printC1Id, 0);		// c0 print -> c1 print
	g_testGraph.AddConnection(printC1Id, 2, addIntId, 0);		// c1 print -> addInt execute
	g_testGraph.AddConnection(addIntId, 3, printResultId, 0);	// addInt exec -> print reult
	g_testGraph.AddConnection(printResultId, 2, igtId, 0);		// print result -> isIntGreater
	g_testGraph.AddConnection(igtId, 3, printGtId, 0);			// int greater true -> print greater
	g_testGraph.AddConnection(igtId, 4, printNotGtId, 0);		// int greater false -> print not greater
	g_testGraph.AddConnection(printGtId, 2, -1, 2);				// print greater -> graph out
	g_testGraph.AddConnection(printNotGtId, 2, -1, 2);			// print not greater -> graph out

	// data flow
	g_testGraph.AddConnection(c0id, 0, addIntId, 1);			// c0 value -> addInt v1
	g_testGraph.AddConnection(c1id, 0, addIntId, 2);			// c1 value -> addInt v2
	g_testGraph.AddConnection(c0id, 0, printC0Id, 1);			// c0 value -> printC0
	g_testGraph.AddConnection(c1id, 0, printC1Id, 1);			// c1 value -> printC1
	g_testGraph.AddConnection(addIntId, 4, printResultId, 1);	// addInt result -> print value
	g_testGraph.AddConnection(addIntId, 4, -1, 3);				// addInt result -> graph result
	g_testGraph.AddConnection(addIntId, 4, igtId, 1);			// addInt result -> igt v1
	g_testGraph.AddConnection(-1, 1, igtId, 2);					// graph input test value -> igt v2
	g_testGraph.AddConnection(-1, 1, printGtId, 1);				// graph input test value -> print greater
	g_testGraph.AddConnection(-1, 1, printNotGtId, 1);			// graph input test value -> print not greater
	
	return true;
}

bool GraphSystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();
	IncrementOneValueABunch();
	{
		Graphs::GraphContext runContext(&g_testGraph);
		runContext.WriteGraphPin(Graphs::Node::InvalidID, 1, 4);		// test value
		runContext.ExecuteNext(Graphs::Node::InvalidID, 0);				// execute graph pin 0
		bool graphRunning = true;
		while (graphRunning)
		{
			auto stepResult = runContext.RunSingleStep();
			if (stepResult == Graphs::GraphContext::Completed)
			{
				SDE_LOG("Graph finished ok");
				int resultValue = runContext.ReadGraphPin(Graphs::Node::InvalidID, 3);
				SDE_LOG("\tresult = %d", resultValue);
				graphRunning = false;
			}
		}
	}

	return true;
}

void GraphSystem::Shutdown()
{
	SDE_PROF_EVENT();
}