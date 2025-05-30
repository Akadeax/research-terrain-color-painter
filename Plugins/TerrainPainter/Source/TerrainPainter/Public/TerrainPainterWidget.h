// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EditorUtilityWidgetComponents.h"
#include "CoreMinimal.h"
#include "Editor/Blutility/Classes/EditorUtilityWidget.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SinglePropertyView.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "GraphHelpers.h"
#include "TerrainPainterWidget.generated.h"

class UCanvasRenderTarget2D;
class UButton;
class UDetailsView;


UENUM(BlueprintType)
enum class ETerrainColorPreset : uint8
{
	None,
	Grassy,
	Swamp,
	Steppe,
	Magma,
	Alien,
};


UCLASS()
class TERRAINPAINTER_API UTerrainPainterWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget))
	UDetailsView* TextureDetailsView; 
	
	UPROPERTY(meta=(BindWidget))
	UButton* BakeButton;

	UPROPERTY(meta=(BindWidget))
	UOverlay* ImageOverlay;
	
	UPROPERTY(meta=(BindWidget))
	UImage* PreviewImage;

	UPROPERTY(meta=(BindWidget))
	UImage* GraphImage;

	UPROPERTY(meta=(BindWidget))
	USinglePropertyView* ShowPreviewPV;

	UPROPERTY(meta=(BindWidget))
	USinglePropertyView* GraphModePV;
	
	UPROPERTY(meta=(BindWidget))
	UEditorUtilityScrollBox* GraphScrollBox;
	
	UPROPERTY(meta=(BindWidget))
	UDetailsView* GraphDataDetailsView;

	UPROPERTY(meta=(BindWidget))
	UButton* CleanupGraphButton;

	UPROPERTY(meta=(BindWidget))
	UButton* ApplyGraphColoringButton; 


	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	
protected:
	// Property View Props
	// Editor
	UPROPERTY(EditDefaultsOnly, Category=TextureDetails)
	FDirectoryPath TerrainColorOutputDirectory{ "/Game/" };
	
	UPROPERTY(EditDefaultsOnly, Category=TextureDetails)
	FString TerrainColorOutputAssetName{ "T_TerrainColorMap" };
	
	UPROPERTY(EditDefaultsOnly, Category=TextureDetails, meta=(UIMin=32, UIMax=4096, ClampMin=32, ClampMax=4096, FixedIncrement=32))
	FIntPoint TextureSize{ 512, 512 };
	
	UPROPERTY(EditDefaultsOnly, Category=GenerationData)
	TArray<FTerrainGraphNode> GenerationData{};

	UPROPERTY(EditDefaultsOnly) bool ShowPreview{ true };

	// Graphing
	UPROPERTY(EditDefaultsOnly)
	bool GraphMode{ true };
	
	UPROPERTY(EditDefaultsOnly, Category=GraphData)
	TArray<FTerrainGraphConnection> TerrainMapConnections{};

	UPROPERTY(EditDefaultsOnly, Category=GraphData)
	TArray<FLinearColor> TerrainColorSet{};

	UPROPERTY(EditDefaultsOnly, Category=GraphData)
	ETerrainColorPreset TerrainColorPreset{};

	UPROPERTY(EditDefaultsOnly, Category=GraphData)
	EGraphColoringAlgo GraphColoringAlgorithm{};
	
	// Props
	UPROPERTY() UTexture2D* PreviewImageTexture{};
	UPROPERTY() UCanvasRenderTarget2D* GraphImageRT{};
	UPROPERTY() UTexture2D* GraphImageTexture{};

	// Methods
	UFUNCTION() void OnBakeClicked();
	void CheckBakeEnabled();
	bool InputParametersValid() const;
	TTuple<bool, FString> TryBakeTexture();
	
	void UpdatePreviewTexture(bool forceAspectRecalc = false);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void UpdateGraphTexture();
	void RecreateGraphTexture();
	UFUNCTION() void DrawGraphTexture(UCanvas* Canvas, int32 Width, int32 Height);
	UFUNCTION() void CleanupGraph();
	UFUNCTION() void ApplyGraphColoring();
	
	/**
	 * Creates a texture and paints the terrain color on it. This must be both repackaged and object flags must be set.
	 * @return The texture with no flags in transient outer
	 */
	void FillTextureWithTerrainColorMap(UTexture2D* texture);
	
	FColor ComputeColorForPixel(int32 X, int32 Y);
	FColor ComputeCheckerboard(int32 X, int32 Y);
	FColor ComputeWeightedTerrainColor(int32 X, int32 Y);

	static const TMap<ETerrainColorPreset, TArray<FLinearColor>> TerrainColorPresets;
};

