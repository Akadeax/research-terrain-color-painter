// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainPainter.h"

#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/Button.h"
#include "Editor/ScriptableEditorWidgets/Public/Components/SinglePropertyView.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UObject/SavePackage.h"
#include "Widgets/Notifications/SNotificationList.h"

void UTerrainPainter::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (TerrainColorOutputDirectorySelector)
	{
		TerrainColorOutputDirectorySelector->SetObject(this);
		TerrainColorOutputDirectorySelector->SetPropertyName(GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorOutputDirectory));
	}
	if (TerrainColorOutputAssetNameSelector)
	{
		TerrainColorOutputAssetNameSelector->SetObject(this);
		TerrainColorOutputAssetNameSelector->SetPropertyName(GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorOutputAssetName));
	}
}

void UTerrainPainter::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (BakeButton)
	{
		BakeButton->OnClicked.AddDynamic(this, &ThisClass::OnBakeClicked);
		CheckBakeEnabled();
	}

	CanvasRenderTarget = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this, UCanvasRenderTarget2D::StaticClass(), 512, 512);
	CanvasRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &ThisClass::RenderTerrainColor);
}

void UTerrainPainter::OnBakeClicked()
{
	const TTuple<bool, FString> state{ TryBakeTexture() };
	
	FNotificationInfo info(
		state.Key
		? FText::FromString(TEXT("Operation succeeded!"))
		: FText::FromString(state.Value)
	);
	info.ExpireDuration = 5.0f;
	info.bUseSuccessFailIcons = true;
	
	const TSharedPtr<SNotificationItem> notif{ FSlateNotificationManager::Get().AddNotification(info) };
	if (notif.IsValid())
	{
		notif->SetCompletionState(state.Key ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}
}

void UTerrainPainter::RenderTerrainColor(UCanvas* Canvas, int32 Width, int32 Height)
{
	Canvas->DrawText(GEngine->GetMediumFont(), TEXT("AH YES"), 0, 0, 10, 10);
	Canvas->DrawText(GEngine->GetMediumFont(), TEXT("The negotiator"), 0, Height / 2, 5, 5);
}

TTuple<bool, FString> UTerrainPainter::TryBakeTexture()
{
	CanvasRenderTarget->UpdateResource();

	if (!IsTerrainColorOutputAssetPathValid())
	{
		return { false, FString::Printf(
			TEXT("final asset path '%s' is invalid!"),
			*(TerrainColorOutputDirectory.Path + TerrainColorOutputAssetName))
		};
	}
	
	const FString longPackageName{ FPaths::Combine(TerrainColorOutputDirectory.Path, TerrainColorOutputAssetName) };
	
	if (StaticLoadObject(UObject::StaticClass(), nullptr, *longPackageName) != nullptr)
	{
		return { false, FString::Printf(TEXT("File at '%s' already exists!"), *longPackageName) };
	}

	UPackage* package{ CreatePackage(*longPackageName) };
	if (!package)
	{
		return { false, FString::Printf(TEXT("Failed to create package at %s. Check the file path."), *longPackageName) };
	}

	UTexture2D* tex{ CanvasRenderTarget->ConstructTexture2D(
		package,
		TerrainColorOutputAssetName,
		RF_Public | RF_Standalone
	) };

	if (!tex)
	{
		return { false, FString::Printf(TEXT("Failed to construct texture at %s."), *longPackageName) };
	}


	FAssetRegistryModule::AssetCreated(tex);
	const FString fileName{ FPackageName::LongPackageNameToFilename(longPackageName, FPackageName::GetAssetPackageExtension()) };
	if (!UPackage::SavePackage(package, tex, *fileName, {}))
	{
		return { false, FString::Printf(TEXT("Failed to save package at %s."), *fileName) };
	}
	
	return { true, TEXT("") };
}

void UTerrainPainter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	const FName changed{ PropertyChangedEvent.MemberProperty->GetFName() };
	if (changed == GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorOutputDirectory))
	{
		FString outPackageName;
		if (FPackageName::TryConvertFilenameToLongPackageName(TerrainColorOutputDirectory.Path, outPackageName))
		{
			TerrainColorOutputDirectory.Path = outPackageName;
		}
		
		CheckBakeEnabled();
	}
	else if (changed == GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorOutputAssetName))
	{
		CheckBakeEnabled();
	}
}

void UTerrainPainter::CheckBakeEnabled()
{
	BakeButton->SetIsEnabled(IsTerrainColorOutputAssetPathValid());
}

bool UTerrainPainter::IsTerrainColorOutputAssetPathValid() const
{
	FString folderAbsolutePath;
	const bool success{ FPackageName::TryConvertLongPackageNameToFilename(TerrainColorOutputDirectory.Path, folderAbsolutePath) };
	if (!success) return false;
	
	if (!FPaths::DirectoryExists(folderAbsolutePath)) return false;
	
	const FString full{ FPaths::Combine(TerrainColorOutputDirectory.Path, TerrainColorOutputAssetName) };
	return FPackageName::IsValidLongPackageName(full);
}
