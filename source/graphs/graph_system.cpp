#include "graph_system.h"
#include "core/log.h"
#include "core/profiler.h"
#include "pin_descriptor.h"
#include "node.h"
#include "graph.h"
#include "graph_context.h"
#include "engine/graphics_system.h"
#include "engine/texture_manager.h"
#include "engine/2d_render_context.h"
#include "engine/system_manager.h"

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

	auto AddEditorData = [](uint16_t nodeId, glm::vec4 bgColour, glm::vec2 pos, glm::vec2 dims)
	{
		Graphs::NodeEditorData ned;
		ned.m_backgroundColour = bgColour;
		ned.m_borderColour = { 1,1,1,1 };
		ned.m_position = pos;
		ned.m_dimensions = dims;
		g_testGraph.SetNodeEditorData(nodeId, ned);
	};

	uint16_t c0id = g_testGraph.AddNode(new ConstantNode(3));
	uint16_t printC0Id = g_testGraph.AddNode(new PrintIntNode(""));
	uint16_t c1id = g_testGraph.AddNode(new ConstantNode(5));
	uint16_t printC1Id = g_testGraph.AddNode(new PrintIntNode(" + "));
	uint16_t addIntId = g_testGraph.AddNode(new AddIntsNode);
	uint16_t printResultId = g_testGraph.AddNode(new PrintIntNode(" = "));
	uint16_t igtId = g_testGraph.AddNode(new IntGreaterThan());
	uint16_t printGtId = g_testGraph.AddNode(new PrintIntNode("which is greater than "));
	uint16_t printNotGtId = g_testGraph.AddNode(new PrintIntNode("which is not greater than "));

	AddEditorData(c0id, { 0.2,0.2,1.0,1.0 }, { 150, 300 }, { 64, 70 });
	AddEditorData(c1id, { 0.2,0.2,1.0,1.0 }, { 256, 160 }, { 64, 70 });

	AddEditorData(printC0Id, { 0.5,0.5,0.5,1.0 }, { 260, 400 }, { 96, 84 });
	AddEditorData(printC1Id, { 0.5,0.5,0.5,1.0 }, { 420, 400 }, { 96, 84 });

	AddEditorData(addIntId, { 0.8, 0.5, 0.5, 1 }, { 550, 300 }, { 80, 116 });
	AddEditorData(printResultId, { 0.5,0.5,0.5,1.0 }, { 680, 400 }, { 96, 84 });

	AddEditorData(igtId, { 0.8,0.8,1.0,1.0 }, { 850, 350 }, { 96, 100 });

	AddEditorData(printGtId, { 0.5,0.5,0.5,1.0 }, { 1000, 370 }, { 96, 84 });
	AddEditorData(printNotGtId, { 0.5,0.5,0.5,1.0 }, { 1000, 240 }, { 96, 84 });

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
		SDE_PROF_EVENT("Run test graph");
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

	auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
	auto textures = Engine::GetSystem<Engine::TextureManager>("Textures");
	auto whiteTex = textures->LoadTexture("white.bmp");
	auto circleTex = textures->LoadTexture("circle_64x64.png");
	auto& r2d = graphics->GetRender2D();
	constexpr float c_nodeHeaderHeight = 24;
	constexpr float c_pinSize = 12;
	constexpr float c_pinXOffset = -6;
	constexpr float c_pinYShift = 20;

	auto GetInputPinPos = [&](const Graphs::Node* n, const Graphs::NodeEditorData* ned, uint8_t pinID) {
		float inputPinsY = ned->m_position.y + ned->m_dimensions.y - c_nodeHeaderHeight - c_pinYShift;
		for (const Graphs::PinDescriptor& pin : n->GetPins())
		{
			if (!pin.m_isOutput)
			{
				if (pin.m_id == pinID)
				{
					return glm::vec2( ned->m_position.x + c_pinXOffset + (c_pinSize / 2), inputPinsY );
				}
				inputPinsY -= c_pinYShift;
			}
		}
		return glm::vec2(-1, -1);
	};

	auto GetOutputPinPos = [&](const Graphs::Node* n, const Graphs::NodeEditorData* ned, uint8_t pinID) {
		float outputPinsY = ned->m_position.y + ned->m_dimensions.y - c_nodeHeaderHeight - c_pinYShift;
		for (const Graphs::PinDescriptor& pin : n->GetPins())
		{
			if (pin.m_isOutput)
			{
				if (pin.m_id == pinID)
				{
					return glm::vec2(ned->m_position.x + ned->m_dimensions.x - c_pinXOffset - (c_pinSize / 2), outputPinsY);
				}
				outputPinsY -= c_pinYShift;
			}
		}
		return glm::vec2(-1, -1);
	};

	{
		SDE_PROF_EVENT("Draw nodes");
		for (auto node : g_testGraph.GetNodes())
		{
			const Graphs::NodeEditorData* ned = g_testGraph.GetNodeEditorData(node->GetID());
			if (ned)
			{
				const glm::vec2 borderOffset(2, 2);
				r2d.DrawQuad(ned->m_position - borderOffset, -1, ned->m_dimensions + (borderOffset * 2.0f), { 0,0 }, { 1,1 }, ned->m_borderColour, whiteTex);
				r2d.DrawQuad(ned->m_position, 0, ned->m_dimensions, { 0,0 }, { 1,1 }, ned->m_backgroundColour, whiteTex);
				
				float nodeHeaderY = ned->m_position.y + ned->m_dimensions.y - c_nodeHeaderHeight;
				r2d.DrawLine({ ned->m_position.x, nodeHeaderY }, 
							 { ned->m_position.x + ned->m_dimensions.x, nodeHeaderY },
							 2, 1, ned->m_borderColour, ned->m_borderColour);
				
				float inputPinsY = ned->m_position.y + ned->m_dimensions.y - c_nodeHeaderHeight - c_pinYShift;
				float outputPinsY = inputPinsY;
				for (const Graphs::PinDescriptor& pin : node->GetPins())
				{
					glm::vec2 pinPos;
					if (pin.m_isOutput)
					{
						pinPos = { ned->m_position.x + ned->m_dimensions.x - c_pinXOffset - (c_pinSize / 2), outputPinsY };
						outputPinsY -= c_pinYShift;
					}
					else
					{
						pinPos = { ned->m_position.x + c_pinXOffset + (c_pinSize / 2), inputPinsY };
						inputPinsY -= c_pinYShift;
					}
					r2d.DrawQuad(pinPos - (c_pinSize / 2), 1, { c_pinSize,c_pinSize }, { 0,0 }, { 1,1 }, { 1,1,1,1 }, circleTex);
				}
			}
		}
	}
	{
		SDE_PROF_EVENT("Draw connections");
		for (auto connection : g_testGraph.GetConnections())
		{
			if (connection.m_nodeID0 == -1 || connection.m_nodeID1 == -1)	// ignore graph in/outs for now
			{
				continue;
			}
			const Graphs::Node* n0 = g_testGraph.GetNode(connection.m_nodeID0);
			const Graphs::Node* n1 = g_testGraph.GetNode(connection.m_nodeID1);
			const Graphs::NodeEditorData* ned0 = g_testGraph.GetNodeEditorData(connection.m_nodeID0);
			const Graphs::NodeEditorData* ned1 = g_testGraph.GetNodeEditorData(connection.m_nodeID1);
			if (n0 && n1 && ned0 && ned1)
			{
				glm::vec2 pin0Pos = GetOutputPinPos(n0, ned0, connection.m_pinID0);
				glm::vec2 pin1Pos = GetInputPinPos(n1, ned1, connection.m_pinID1);
				glm::vec4 startCol = { 0, 0.95, 1,1 };
				glm::vec4 endCol = { 0.01, 0.5, 1, 1 };
				if (n0->GetOutputPin(connection.m_pinID0)->m_type == "Execution")
				{
					startCol = { 0.8, 0.01, 0.02,1 };
					endCol = { 0.5, 0.0, 0.1, 1 };
				}
				r2d.DrawLine(pin0Pos, pin1Pos, 2.5, 2, startCol, endCol);
			}
		}
	}
	return true;
}

void GraphSystem::Shutdown()
{
	SDE_PROF_EVENT();
}