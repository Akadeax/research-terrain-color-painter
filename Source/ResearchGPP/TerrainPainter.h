// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Classes/EditorUtilityWidget.h"
#include "TerrainPainter.generated.h"

class UCanvasRenderTarget2D;
class UButton;
class USinglePropertyView;

UCLASS()
class RESEARCHGPP_API UTerrainPainter : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget))
	USinglePropertyView* TerrainColorOutputDirectoryPV;

	UPROPERTY(meta=(BindWidget))
	USinglePropertyView* TerrainColorOutputAssetNamePV;

	UPROPERTY(meta=(BindWidget))
	USinglePropertyView* TextureSizePV;

	
	UPROPERTY(meta=(BindWidget))
	UButton* BakeButton;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	
protected:
	UPROPERTY(EditDefaultsOnly) FDirectoryPath TerrainColorOutputDirectory{ "/Game/" };
	UPROPERTY(EditDefaultsOnly) FString TerrainColorOutputAssetName{ "T_" };
	UPROPERTY(EditDefaultsOnly) FIntPoint TextureSize{ 512, 512 };
	
	UFUNCTION() void OnBakeClicked();
	UFUNCTION() void RenderTerrainColor(UCanvas* Canvas, int32 Width, int32 Height);

	TTuple<bool, FString> TryBakeTexture();
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void CheckBakeEnabled();
	bool InputParametersValid() const;

	/**
	 * Creates a texture and paints the terrain color on it. This must be both repackaged and object flags must be set.
	 * @return The texture with no flags in transient outer
	 */
	UTexture2D* CreateTerrainColorTexture();
	FColor ComputeColorForPixel(int32 X, int32 Y);
};
