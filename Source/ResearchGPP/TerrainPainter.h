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
	USinglePropertyView* TerrainColorOutputDirectorySelector;

	UPROPERTY(meta=(BindWidget))
	USinglePropertyView* TerrainColorOutputAssetNameSelector;

	UPROPERTY(meta=(BindWidget))
	UButton* BakeButton;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	
protected:
	UPROPERTY(EditDefaultsOnly) FDirectoryPath TerrainColorOutputDirectory;
	UPROPERTY(EditDefaultsOnly) FString TerrainColorOutputAssetName;
	UPROPERTY() UCanvasRenderTarget2D* CanvasRenderTarget{};
	
	UFUNCTION() void OnBakeClicked();
	UFUNCTION() void RenderTerrainColor(UCanvas* Canvas, int32 Width, int32 Height);

	TTuple<bool, FString> TryBakeTexture();
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void CheckBakeEnabled();
	bool IsTerrainColorOutputAssetPathValid() const;
};
