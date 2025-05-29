// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainPainterWidget.h"

#include "ColorHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/Button.h"
#include "Framework/Notifications/NotificationManager.h"
#include "UObject/SavePackage.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "PropertyViewHelpers.h"
#include "Algo/RemoveIf.h"
#include "Algo/Unique.h"
#include "Components/SizeBox.h"
#include "Engine/Canvas.h"

void UTerrainPainterWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	SetupDetailsView(this, TextureDetailsView, {}, {
		GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorOutputDirectory),
		GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorOutputAssetName),
		GET_MEMBER_NAME_CHECKED(ThisClass, TextureSize),
		
		GET_MEMBER_NAME_CHECKED(ThisClass, GenerationData),
	});

	SetupSinglePropertyView(this, ShowPreviewPV, GET_MEMBER_NAME_CHECKED(ThisClass, ShowPreview));
	SetupSinglePropertyView(this, GraphModePV, GET_MEMBER_NAME_CHECKED(ThisClass, GraphMode));

	SetupDetailsView(this, GraphDataDetailsView, {}, {
		GET_MEMBER_NAME_CHECKED(ThisClass, TerrainMapConnections),
		GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorPreset),
		GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorSet),
	});
	
	if (PreviewImage)
	{
		PreviewImageTexture = UTexture2D::CreateTransient(TextureSize.X, TextureSize.Y);
		if (!ensureAlwaysMsgf(PreviewImageTexture, TEXT("Failed to create Preview Image Texture!"))) return;
		
		PreviewImage->SetBrushFromTexture(PreviewImageTexture, true);
		UpdatePreviewTexture(true);
	}

	if (GraphImage)
	{
		UpdateGraphTexture();
		GraphImage->SetBrushFromTexture(GraphImageTexture, true);
	}
}

void UTerrainPainterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (BakeButton)
	{
		BakeButton->OnClicked.AddDynamic(this, &ThisClass::OnBakeClicked);
		CheckBakeEnabled();
	}

	if (CleanupGraphButton)
	{
		CleanupGraphButton->OnClicked.AddDynamic(this, &ThisClass::CleanupGraph);
	}

	if (ApplyGraphColoringButton)
	{
		ApplyGraphColoringButton->OnClicked.AddDynamic(this, &ThisClass::ApplyGraphColoring);
	}
}

void UTerrainPainterWidget::OnBakeClicked()
{
	const TTuple<bool, FString> state{ TryBakeTexture() };
	
	FNotificationInfo info(
		FText::FromString(state.Value)
	);
	info.ExpireDuration = 5.0f;
	info.bUseSuccessFailIcons = true;
	
	const TSharedPtr<SNotificationItem> notif{ FSlateNotificationManager::Get().AddNotification(info) };
	if (notif.IsValid())
	{
		notif->SetCompletionState(state.Key ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}
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

	UPackage* package{ CreatePackage(*longPackageName) };
	if (!package)
	{
		return { false, FString::Printf(TEXT("Failed to create package at %s. Check the file path."), *longPackageName) };
	}

	UTexture2D* texture{};
	bool didCreateNew{ false };
	if (StaticLoadObject(UObject::StaticClass(), nullptr, *longPackageName) == nullptr)
	{
		texture = NewObject<UTexture2D>(package, *TerrainColorOutputAssetName, RF_Public | RF_Standalone);
		if (!texture) return { false, FString::Printf(TEXT("Failed to create new texture object at %s."), *longPackageName) };
		didCreateNew = true;
	}
	else
	{
		TArray<UObject*> objects;
		GetObjectsWithOuter(package, objects, false);
		for (UObject* object : objects)
		{
			if (!object || !object->IsA<UTexture2D>()) continue;
			texture = Cast<UTexture2D>(object);
		}

		if (!texture) return { false, "This package already exists and is not a Texture2D." };
	}
	
	FillTextureWithTerrainColorMap(texture);
	FAssetRegistryModule::AssetCreated(texture);
	
	const FString fileName{ FPackageName::LongPackageNameToFilename(longPackageName, FPackageName::GetAssetPackageExtension()) };
	if (!UPackage::SavePackage(package, texture, *fileName, {}))
	{
		return { false, FString::Printf(TEXT("Failed to save package at %s."), *fileName) };
	}
	
	return { true, didCreateNew ? TEXT("Successfully created new texture!") : TEXT("Successfully overwrote texture!") };
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
	else if (changed == GET_MEMBER_NAME_CHECKED(ThisClass, TextureSize) ||
			 changed == GET_MEMBER_NAME_CHECKED(ThisClass, GenerationData))
	{
		if (ShowPreview) UpdatePreviewTexture();
		if (GraphMode) UpdateGraphTexture();
		CheckBakeEnabled();
	}
	else if (changed == GET_MEMBER_NAME_CHECKED(ThisClass, ShowPreview))
	{
		PreviewImage->SetVisibility(ShowPreview ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		if (ShowPreview) UpdatePreviewTexture(true);
	}
	else if (changed == GET_MEMBER_NAME_CHECKED(ThisClass, GraphMode))
	{
		const ESlateVisibility vis{ GraphMode ? ESlateVisibility::Visible : ESlateVisibility::Hidden };
		GraphImage->SetVisibility(vis);
		GraphDataDetailsView->SetVisibility(vis);
		CleanupGraphButton->SetVisibility(vis);
		
		if (GraphMode) UpdateGraphTexture();
	}
	else if (changed == GET_MEMBER_NAME_CHECKED(ThisClass, TerrainMapConnections))
	{
		for (FTerrainMapConnection& conn : TerrainMapConnections)
		{
			if (conn.Element1 < 0) conn.Element1 = 0;
			else if (conn.Element1 >= GenerationData.Num()) conn.Element1 = GenerationData.Num() - 1;
			
			if (conn.Element2 < 0) conn.Element2 = 0;
			else if (conn.Element2 >= GenerationData.Num()) conn.Element2 = GenerationData.Num() - 1;
		}
		
		UpdateGraphTexture();
	}
	else if (changed == GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorSet))
	{
		TerrainColorPreset = ETerrainColorPreset::None;
	}
	else if (changed == GET_MEMBER_NAME_CHECKED(ThisClass, TerrainColorPreset))
	{
		switch (TerrainColorPreset)
		{
		case ETerrainColorPreset::None:
			TerrainColorSet.Empty();
			break;
		case ETerrainColorPreset::Grassy:
			TerrainColorSet = {
				{ 1, 0, 0 },
			};
			break;
		case ETerrainColorPreset::Magma:
			TerrainColorSet = {
				{ 0, 1, 0 },
			};
			break;
		case ETerrainColorPreset::Alien:
			TerrainColorSet = {
				{ 0, 0, 1 },
			};
			break;
		}
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

	if (GenerationData.Num() == 0) return false;
	
	return true;
}

void UTerrainPainterWidget::UpdatePreviewTexture(bool forceAspectRecalc)
{
	if (!ShowPreview) return;
	
	// Make this a pointer instead of a ref (which it normally should be) because we might
	// have to change the mip's content
	FTexture2DMipMap* mip{ &PreviewImageTexture->GetPlatformData()->Mips[0] };

	const bool didSizeChange{ (TextureSize.X != mip->SizeX || TextureSize.Y != mip->SizeY) };
	bool doResize{ didSizeChange };
	if (doResize)
	{
		doResize = true;
		// Rebuild this mip; our texture size has changed
		PreviewImageTexture->ReleaseResource();
		PreviewImageTexture->GetPlatformData()->Mips.Empty();

		PreviewImageTexture->GetPlatformData()->SizeX = TextureSize.X;
		PreviewImageTexture->GetPlatformData()->SizeY = TextureSize.Y;
		
		mip = new FTexture2DMipMap(TextureSize.X, TextureSize.Y);
		PreviewImageTexture->GetPlatformData()->Mips.Add(mip);
		
		(void)mip->BulkData.Lock(LOCK_READ_WRITE);
		mip->BulkData.Realloc(TextureSize.X * TextureSize.Y * sizeof(FColor));
		mip->BulkData.Unlock();
	}

	USizeBox* box{ Cast<USizeBox>(ImageOverlay->GetParent()) };
	if (box && (doResize || forceAspectRecalc))
	{
		const float aspect{ TextureSize.X / static_cast<float>(TextureSize.Y) };
		box->SetMinAspectRatio(aspect);
		box->SetMaxAspectRatio(aspect);
	}

	void* rawData{ mip->BulkData.Lock(LOCK_READ_WRITE) };
	FColor* pixelData{ static_cast<FColor*>(rawData) };
	for (int32 y{}; y < TextureSize.Y; ++y)
	{
		for (int32 x{}; x < TextureSize.X; ++x)
		{
			const int32 index{ y * TextureSize.X + x };
			pixelData[index] = ComputeColorForPixel(x, y);
		}
	}
	mip->BulkData.Unlock();
	PreviewImageTexture->UpdateResource();
}

void UTerrainPainterWidget::FillTextureWithTerrainColorMap(UTexture2D* texture)
{
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

	texture->Source.Init(
		TextureSize.X, TextureSize.Y, 1, 1,
		TSF_BGRA8, reinterpret_cast<uint8*>(pixelData.GetData())
	);
	texture->MipGenSettings = TMGS_NoMipmaps;

	texture->UpdateResource();
}

void UTerrainPainterWidget::UpdateGraphTexture()
{
	if (!GraphImageRT)
	{
		RecreateGraphTexture();
	}
	
	const float texAspect{ static_cast<float>(TextureSize.X) / TextureSize.Y };
	const float rtAspect{ GraphImageRT->GetSurfaceWidth() / GraphImageRT->GetSurfaceHeight() };
	
	// Tex exists, but aspect is wrong; recreate
	if (!FMath::IsNearlyEqual(texAspect, rtAspect))
	{
		RecreateGraphTexture();
	}

	GraphImageRT->UpdateResource();

	if (GraphImageTexture)
	{
		GraphImageTexture->ReleaseResource();
	}
	UTexture* tex{ GraphImageRT->ConstructTexture(
		GetTransientPackage(),
		TEXT("GraphImageTexture"),
		RF_Transient
	) };
	GraphImageTexture = Cast<UTexture2D>(tex);
	
}

void UTerrainPainterWidget::RecreateGraphTexture()
{
	if (GraphImageRT)
	{
		GraphImageRT->ReleaseResource();
	}

	const float texAspect{ static_cast<float>(TextureSize.X) / TextureSize.Y };
	
	GraphImageRT = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(
		this, UCanvasRenderTarget2D::StaticClass(),
		FMath::FloorToInt32(256 * texAspect), 256
	);
	GraphImageRT->ClearColor = FColor(0, 0, 0, 0);

	GraphImageRT->OnCanvasRenderTargetUpdate.AddDynamic(this, &ThisClass::DrawGraphTexture);
}

void UTerrainPainterWidget::DrawGraphTexture(UCanvas* Canvas, int32 Width, int32 Height)
{
	if (GenerationData.IsEmpty()) return;
	
	for (const FTerrainMapGenerationDataEntry& data : GenerationData)
	{
		FVector2D pos{ data.UVCoordinates.X * Width, data.UVCoordinates.Y * Height };
		DrawDebugCanvas2DCircle(Canvas, pos, 10, 10, FLinearColor::White);
		DrawDebugCanvas2DCircle(Canvas, pos, 12, 10, FLinearColor::Black);
	}
	
	for (const FTerrainMapConnection& conn : TerrainMapConnections)
	{
		if (conn.Element1 == conn.Element2 ||
			conn.Element1 < 0 || conn.Element2 < 0 ||
			conn.Element1 >= GenerationData.Num() ||
			conn.Element2 >= GenerationData.Num())
		{
			continue;
		}
		
		const FVector2f uv1{ GenerationData[conn.Element1].UVCoordinates };
		const FVector2f uv2{ GenerationData[conn.Element2].UVCoordinates };
		const FVector2D pos1{ uv1.X * Width, uv1.Y * Height };
		const FVector2D pos2{ uv2.X * Width, uv2.Y * Height };
		DrawDebugCanvas2DLine(Canvas, pos1, pos2, FLinearColor::White);
	}
}

void UTerrainPainterWidget::CleanupGraph()
{
	TArray<FTerrainMapConnection> newArray{};
	for (FTerrainMapConnection& conn : TerrainMapConnections)
	{
		if (conn.Element1 == conn.Element2) continue;
		if (conn.Element1 > conn.Element2) conn.Swap();
		newArray.AddUnique(conn);
	}
	TerrainMapConnections = newArray;
}

void UTerrainPainterWidget::ApplyGraphColoring()
{
	
}

FColor UTerrainPainterWidget::ComputeColorForPixel(int32 X, int32 Y)
{
	if (GenerationData.IsEmpty()) return ComputeCheckerboard(X, Y);	

	return ComputeWeightedTerrainColor(X, Y); 
}

FColor UTerrainPainterWidget::ComputeCheckerboard(int32 X, int32 Y)
{
	const int min{ FMath::Min(TextureSize.X, TextureSize.Y) };
	const FVector2f aspectedUV{ static_cast<float>(X) / static_cast<float>(min), static_cast<float>(Y) / static_cast<float>(min) };

	// Draw checkerboard pattern if no generation data
	constexpr float checkerSize{ 25.f };
	const FVector2f pos{ aspectedUV * checkerSize };
	const FIntPoint intPos{ FMath::FloorToInt32(pos.X), FMath::FloorToInt32(pos.Y) };
	const FIntPoint mod{ intPos.X % 2, intPos.Y % 2 };
	const bool doColor{ mod.X == 0 || mod.Y == 0 };
	return doColor ? FColor::Purple : FColor::Black;
}

FColor UTerrainPainterWidget::ComputeWeightedTerrainColor(int32 X, int32 Y)
{
	const FVector2f uv{ static_cast<float>(X) / static_cast<float>(TextureSize.X), static_cast<float>(Y) / static_cast<float>(TextureSize.Y) };
	constexpr float MAX_UV_DIST{ 1.414213f };
	
	FLinearColor result{};
	for (const FTerrainMapGenerationDataEntry& data : GenerationData)
	{
		const float dist{ FMath::Clamp(FVector2f::Distance(uv, data.UVCoordinates) * (1.f / data.DistanceModifier), 0, MAX_UV_DIST) };
		const float normDist{ dist / MAX_UV_DIST };
		const float invNormDist{ 1.f - normDist };
		
		result += data.Color * invNormDist * data.Intensity;
	}
	result = NormalizeToMax(result);
	result.A = 1.f;
	return result.ToFColor(false);
}
