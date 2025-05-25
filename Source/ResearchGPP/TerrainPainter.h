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
	USinglePropertyView* TerrainColorRenderTargetAssetSelector;

	UPROPERTY(meta=(BindWidget))
	UButton* BakeButton;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	
protected:
	UPROPERTY(EditInstanceOnly) FFilePath TerrainColorOutputAsset;
	UPROPERTY() UCanvasRenderTarget2D* CanvasRenderTarget{};
	
	UFUNCTION() void OnBakeClicked();
	UFUNCTION() void RenderTerrainColor(UCanvas* Canvas, int32 Width, int32 Height);
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	bool IsTerrainColorOutputAssetPathValid() const;
};
