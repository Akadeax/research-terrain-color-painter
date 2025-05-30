#include "GraphHelpers.h"

#include "Algo/IndexOf.h"

GraphHelper::GraphHelper(TArray<FTerrainGraphNode>& nodes, TArray<FTerrainGraphConnection>& connections, TArray<FLinearColor>& colors)
	: Nodes{ nodes }
	, Connections{ connections }
	, Colors{ colors }
{
}

void GraphHelper::ColorGraph(EGraphColoringAlgo algo)
{
	switch (algo)
	{
	case EGraphColoringAlgo::Greedy:
		GreedyColoring(); break;
	case EGraphColoringAlgo::WelshPowell:
		WelshPowell(); break;
	case EGraphColoringAlgo::DSatur:
		DSatur(); break;
	}
}

void GraphHelper::GreedyColoring()
{
	if (Nodes.IsEmpty()) return;

	TArray<int32> colors;
	colors.Init(-1, Nodes.Num());

	for (int32 i{}; i < Nodes.Num(); ++i)
	{
		FTerrainGraphNode& node{ Nodes[i] };
		ColorNode(node, Nodes.IndexOfByKey(node), colors);
	}
}

void GraphHelper::WelshPowell()
{
	if (Nodes.IsEmpty()) return;

	Algo::SortBy(Nodes, [this](const FTerrainGraphNode& node)
	{
		return GetConnectedNodes(node).Num();
	});
	Algo::Reverse(Nodes);
	
	GreedyColoring();
}

void GraphHelper::DSatur()
{
	if (Nodes.IsEmpty()) return;
	
	TArray<int32> colors;
	colors.Init(-1, Nodes.Num());

	for (int32 i{}; i < Nodes.Num(); ++i)
	{
		const FTerrainGraphNode* nextNode{ Algo::MinElementBy(Nodes, [this, &colors](const FTerrainGraphNode& innerNode)
		{
			// Don't consider already-colored nodes
			if (colors[Algo::IndexOf(Nodes, innerNode)] == -1) return -1;
			
			TArray<int32> nodeConns{ GetConnectedNodes(innerNode) };
			int32 alreadyColoredNeighborCount{};
			for (const int32 conn : nodeConns)
			{
				if (colors[conn] == -1) continue;
				++alreadyColoredNeighborCount;
			}
			return alreadyColoredNeighborCount;
		}) };
		check(nextNode);

		ColorNode(*nextNode, Nodes.IndexOfByKey(*nextNode), colors);
	}
}

TArray<int32> GraphHelper::GetConnectedNodes(const FTerrainGraphNode& node)
{
	TArray<int32> connected{};

	for (const FTerrainGraphConnection& connection : Connections)
	{
		const FTerrainGraphNode* n1{ &Nodes[connection.Element1] };
		const FTerrainGraphNode* n2{ &Nodes[connection.Element2] };

		if (n1 == &node)
		{
			connected.Emplace(connection.Element2);
		}
		if (n2 == &node)
		{
			connected.Emplace(connection.Element1);
		}
	}

	return connected;
}


void GraphHelper::ColorNode(const FTerrainGraphNode& Node, int index, TArray<int32>& ColorsList)
{
	TArray<int32> connectedList{ GetConnectedNodes(Node) };
	TArray<int32> connectedColors{};
	for (const int32 connectedNodeIndex : connectedList)
	{
		connectedColors.Add(ColorsList[connectedNodeIndex]);
	}
		
	int32 newColor{ 0 };
	while (Algo::Find(connectedColors, newColor) != nullptr) ++newColor;
		
	ColorsList[index] = newColor;

	if (newColor >= Colors.Num())
	{
		Nodes[index].Color = FLinearColor::Black;
	}
	Nodes[index].Color = Colors[newColor];
}