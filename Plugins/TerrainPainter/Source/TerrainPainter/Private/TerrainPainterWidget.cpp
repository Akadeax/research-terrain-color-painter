// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainPainterWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/Button.h"
#include "Engine/Canvas.h"
#include "Framework/Notifications/NotificationManager.h"
#include "UObject/SavePackage.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "PropertyViewHelpers.h"

void UTerrainPainterWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	SetupDetailsView(this, TextureDetailsView, { "TextureDetails" }, {
		GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorOutputDirectory),
		GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorOutputAssetName),
		GET_MEMBER_NAME_CHECKED(ThisClass, TextureSize),
	});
}

void UTerrainPainterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (BakeButton)
	{
		BakeButton->OnClicked.AddDynamic(this, &ThisClass::OnBakeClicked);
		CheckBakeEnabled();
	}
}

void UTerrainPainterWidget::OnBakeClicked()
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

void UTerrainPainterWidget::RenderTerrainColor(UCanvas* Canvas, int32 Width, int32 Height)
{
	Canvas->DrawText(GEngine->GetMediumFont(), TEXT("AH YES"), 0, 0, 10, 10);
	Canvas->DrawText(GEngine->GetMediumFont(), TEXT("The negotiator"), 0, Height / 2, 5, 5);
}

TTuple<bool, FString> UTerrainPainterWidget::TryBakeTexture()
{
	if (!InputParametersValid())
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

	UTexture2D* tex{ CreateTerrainColorTexture() };
	tex->Rename(nullptr, package);
	tex->SetFlags(RF_Public | RF_Standalone);

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

void UTerrainPainterWidget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
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

void UTerrainPainterWidget::CheckBakeEnabled()
{
	BakeButton->SetIsEnabled(InputParametersValid());
}

bool UTerrainPainterWidget::InputParametersValid() const
{
	FString folderAbsolutePath;
	const bool folderPathValid{ FPackageName::TryConvertLongPackageNameToFilename(TerrainColorOutputDirectory.Path, folderAbsolutePath) };
	if (!folderPathValid) return false;
	
	const bool folderExists{ FPaths::DirectoryExists(folderAbsolutePath) };
	if (!folderExists) return false;
	
	const FString combinedPath{ FPaths::Combine(TerrainColorOutputDirectory.Path, TerrainColorOutputAssetName) };
	const bool combinedPathValid{ FPackageName::IsValidLongPackageName(combinedPath) };
	if (!combinedPathValid) return false;

	const bool textureSizeValid{ TextureSize.X > 0 && TextureSize.Y > 0 && TextureSize.X < 8192 && TextureSize.Y < 8192 };
	if (!textureSizeValid) return false;
	
	return true;
}

UTexture2D* UTerrainPainterWidget::CreateTerrainColorTexture()
{
	UTexture2D* tex{ NewObject<UTexture2D>(GetTransientPackage(), *TerrainColorOutputAssetName, RF_NoFlags) };
	if (!tex) return nullptr;

	const int32 numPixels{ TextureSize.X * TextureSize.Y };
	
	TArray<FColor> pixelData;
	pixelData.SetNumUninitialized(numPixels);
	
	for (int32 y{}; y < TextureSize.Y; ++y)
	{
		for (int32 x{}; x < TextureSize.X; ++x)
		{
			const int32 index{ y * TextureSize.X + x };
			pixelData[index] = ComputeColorForPixel(x, y);
		}
	}

	tex->Source.Init(
		TextureSize.X, TextureSize.Y, 1, 1,
		TSF_BGRA8, reinterpret_cast<uint8*>(pixelData.GetData())
	);
	tex->MipGenSettings = TMGS_NoMipmaps;

	tex->UpdateResource();

	return tex;
}

FColor UTerrainPainterWidget::ComputeColorForPixel(int32 X, int32 Y)
{
	const float normX{ static_cast<float>(X) / static_cast<float>(TextureSize.X) };
	const float normY{ static_cast<float>(Y) / static_cast<float>(TextureSize.Y) };
	const float blend{ FMath::Clamp((normX + normY) / 2.f, 0.0f, 1.0f) };
	const FLinearColor color{ 1.0f, 1.0f - blend, 1.0f - blend, 1.0f };
	return color.ToFColor(false);
}
