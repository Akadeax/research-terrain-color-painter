// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Classes/EditorUtilityWidget.h"
#include "Components/Image.h"
#include "TerrainPainterWidget.generated.h"

class UCanvasRenderTarget2D;
class UButton;
class UDetailsView;

USTRUCT(BlueprintType)
struct FTerrainMapGenerationDataEntry
{
	GENERATED_BODY()
	UPROPERTY(EditInstanceOnly, meta=(UIMin=0, UIMax=1)) FVector2f UVCoordinates{ 0.5f, 0.5f };
	UPROPERTY(EditInstanceOnly) FLinearColor Color{ 1.f, 1.f, 1.f };
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
	UImage* PreviewImage;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category=TextureDetails) FDirectoryPath TerrainColorOutputDirectory{ "/Game/" };
	UPROPERTY(EditDefaultsOnly, Category=TextureDetails) FString TerrainColorOutputAssetName{ "T_TerrainColorMap" };
	UPROPERTY(EditDefaultsOnly, Category=TextureDetails) FIntPoint TextureSize{ 512, 512 };
	UPROPERTY(EditDefaultsOnly, Category=GenerationData) TArray<FTerrainMapGenerationDataEntry> GenerationData{};

	UPROPERTY() UTexture2D* PreviewImageTexture{};
	
	UFUNCTION() void OnBakeClicked();

	TTuple<bool, FString> TryBakeTexture();
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void CheckBakeEnabled();
	bool InputParametersValid() const;

	void UpdatePreviewTexture(bool forceAspectRecalc = false);
	
	/**
	 * Creates a texture and paints the terrain color on it. This must be both repackaged and object flags must be set.
	 * @return The texture with no flags in transient outer
	 */
	void FillTextureWithTerrainColorMap(UTexture2D* texture);
	
	FColor ComputeColorForPixel(int32 X, int32 Y);
};
