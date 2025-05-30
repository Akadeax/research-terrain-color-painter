#pragma once

#include "GraphHelpers.generated.h"

USTRUCT(BlueprintType)
struct FTerrainGraphNode
{
	GENERATED_BODY()
	
	UPROPERTY(EditInstanceOnly, meta=(UIMin=0, UIMax=1))
	FVector2f UVCoordinates{ 0.5f, 0.5f };
	
	UPROPERTY(EditInstanceOnly)
	FLinearColor Color{ 1.f, 1.f, 1.f };
	
	UPROPERTY(EditInstanceOnly, meta=(UIMin=0, UIMax=2))
	float Intensity{ 1.f };
	
	UPROPERTY(EditInstanceOnly, meta=(UIMin=0, UIMax=2))
	float DistanceModifier{ 1.f };

	bool operator==(const FTerrainGraphNode& rhs) const
	{
		return UVCoordinates == rhs.UVCoordinates && Color == rhs.Color && Intensity == rhs.Intensity && DistanceModifier == rhs.DistanceModifier;
	}
};

USTRUCT(BlueprintType)
struct FTerrainGraphConnection
{
	GENERATED_BODY()
	
	UPROPERTY(EditInstanceOnly, meta=(ClampMin=0)) int Element1{};
	UPROPERTY(EditInstanceOnly, meta=(ClampMin=0)) int Element2{};

	bool operator==(const FTerrainGraphConnection& other) const
	{
		return (Element1 == other.Element1 && Element2 == other.Element2) || (Element1 == other.Element2 && Element2 == other.Element1);
	}

	void Swap()
	{
		const int temp{ Element1 };
		Element1 = Element2;
		Element2 = temp;
	}
};

UENUM(BlueprintType)
enum class EGraphColoringAlgo : uint8
{
	Greedy,
	WelshPowell,
	DSatur
};

class GraphHelper
{
public:
	GraphHelper(TArray<FTerrainGraphNode>& nodes, TArray<FTerrainGraphConnection>& connections, TArray<FLinearColor>& colors);

	void ColorGraph(EGraphColoringAlgo algo);
	
private:
	TArray<FTerrainGraphNode>& Nodes;
	TArray<FTerrainGraphConnection>& Connections;
	TArray<FLinearColor>& Colors;

	void GreedyColoring();
	void WelshPowell();
	void DSatur();
	
	TArray<int32> GetConnectedNodes(const FTerrainGraphNode& node);
	void ColorNode(const FTerrainGraphNode& Node, int index, TArray<int32>& ColorsList);
	
};
