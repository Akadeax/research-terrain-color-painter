// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainPainter.h"

#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/Button.h"
#include "Editor/ScriptableEditorWidgets/Public/Components/SinglePropertyView.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UObject/SavePackage.h"

void UTerrainPainter::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (TerrainColorRenderTargetAssetSelector)
	{
		TerrainColorRenderTargetAssetSelector->SetObject(this);
		TerrainColorRenderTargetAssetSelector->SetPropertyName(GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorOutputAsset));
	}
}

void UTerrainPainter::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (BakeButton)
	{
		BakeButton->OnClicked.AddDynamic(this, &ThisClass::OnBakeClicked);
	}

	CanvasRenderTarget = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this, UCanvasRenderTarget2D::StaticClass(), 512, 512);
	CanvasRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &ThisClass::RenderTerrainColor);
}

void UTerrainPainter::OnBakeClicked()
{
	CanvasRenderTarget->UpdateResource();

	if (!IsTerrainColorOutputAssetPathValid()) return;
	
	const FString rawPath{ TerrainColorOutputAsset.FilePath };
	const FString fileWithExt{ FPaths::GetCleanFilename(rawPath) };
	const FString packageFileName{ FPackageName::FilenameToLongPackageName(rawPath) };
	const FString longPath{ FPackageName::LongPackageNameToFilename(packageFileName, FPackageName::GetAssetPackageExtension()) };
	
	UPackage* package{ CreatePackage(*packageFileName) };
	if (!package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package for texture!"));
		return;
	}

	UTexture2D* tex{ CanvasRenderTarget->ConstructTexture2D(package, fileWithExt, RF_Public | RF_Standalone) };

	FSavePackageArgs args{};
	args.TopLevelFlags = RF_Public | RF_Standalone;
	args.bForceByteSwapping = true;
	
	const bool success{ UPackage::SavePackage(
		package, tex,
		*longPath,
		args
	) };
	
	if (!success)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save texture as package."));
	}
	
	FAssetRegistryModule::AssetCreated(tex);
	TArray<UObject*> obj{};
	obj.Add(tex);
	IContentBrowserSingleton::Get().SyncBrowserToAssets(obj);
}

void UTerrainPainter::RenderTerrainColor(UCanvas* Canvas, int32 Width, int32 Height)
{
	Canvas->DrawText(GEngine->GetMediumFont(), TEXT("AH YES"), 0, 0, 10, 10);
}

void UTerrainPainter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property->GetFName() != GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorOutputAsset)) return;
	BakeButton->SetIsEnabled(IsTerrainColorOutputAssetPathValid());
}

bool UTerrainPainter::IsTerrainColorOutputAssetPathValid() const
{
	const FString& filePath{ TerrainColorOutputAsset.FilePath };
	const FString& dirPath{ FPaths::GetPath(filePath) };
	const FString& ext{ FPaths::GetExtension(filePath) };
	
	return !filePath.IsEmpty() && FPaths::DirectoryExists(dirPath) && !ext.IsEmpty();
}
