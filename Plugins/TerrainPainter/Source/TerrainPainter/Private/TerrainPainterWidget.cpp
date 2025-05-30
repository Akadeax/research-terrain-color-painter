// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainPainterWidget.h"

#include "ColorHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/Button.h"
#include "Framework/Notifications/NotificationManager.h"
#include "UObject/SavePackage.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "PropertyViewHelpers.h"
#include "Algo/RandomShuffle.h"
#include "Components/SizeBox.h"
#include "Engine/Canvas.h"

const TMap<ETerrainColorPreset, TArray<FLinearColor>> UTerrainPainterWidget::TerrainColorPresets =
{
    {
        ETerrainColorPreset::Grassy, {
            FLinearColor(0.0f, 1.0f, 0.0f),   // Neon green
            FLinearColor(1.0f, 1.0f, 0.0f),   // Pure yellow
            FLinearColor(0.0f, 0.5f, 0.0f),   // Deep green
            FLinearColor(0.4f, 1.0f, 0.4f),   // Mint green
            FLinearColor(0.6f, 1.0f, 0.0f)    // Chartreuse
        }
    },
	
    {
        ETerrainColorPreset::Swamp, {
            FLinearColor(0.1f, 0.2f, 0.0f),   // Bog sludge
            FLinearColor(0.2f, 0.4f, 0.1f),   // Algae green
            FLinearColor(0.5f, 0.0f, 0.3f),   // Murky violet
            FLinearColor(0.0f, 0.3f, 0.2f),   // Dirty teal
            FLinearColor(0.3f, 0.2f, 0.0f)    // Stagnant brown
        }
    },
	
    {
        ETerrainColorPreset::Steppe, {
            FLinearColor(1.0f, 0.8f, 0.0f),   // Super bright gold
            FLinearColor(1.0f, 0.4f, 0.0f),   // Orange blaze
            FLinearColor(0.8f, 0.6f, 0.1f),   // Yellow ochre
            FLinearColor(1.0f, 0.2f, 0.6f),   // Hot pink
            FLinearColor(0.6f, 0.3f, 0.0f)    // Burnt umber
        }
    },
	
    {
        ETerrainColorPreset::Magma, {
            FLinearColor(1.0f, 0.0f, 0.0f),   // Pure red
            FLinearColor(1.0f, 0.3f, 0.0f),   // Fiery orange
            FLinearColor(1.0f, 1.0f, 0.0f),   // Explosive yellow
            FLinearColor(0.9f, 0.0f, 1.0f),   // Intense magenta
            FLinearColor(0.0f, 0.0f, 0.0f)    // Pitch black
        }
    },
	
    {
        ETerrainColorPreset::Alien, {
            FLinearColor(0.0f, 1.0f, 1.0f),   // Cyan laser
            FLinearColor(1.0f, 0.0f, 1.0f),   // Electric pink
            FLinearColor(0.5f, 0.0f, 1.0f),   // Radioactive violet
            FLinearColor(0.2f, 1.0f, 0.8f),   // Glowing aqua
            FLinearColor(1.0f, 1.0f, 1.0f)    // Blinding white
        }
    },
	
    {
        ETerrainColorPreset::None, {}
    }
};


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
		GET_MEMBER_NAME_CHECKED(ThisClass, GraphColoringAlgorithm),
	});
	
	if (PreviewImage)
	{
		PreviewImageTexture = UTexture2D::CreateTransient(TextureSize.X, TextureSize.Y);
		if (!ensureAlwaysMsgf(PreviewImageTexture, TEXT("Failed to create Preview Image Texture!"))) return;

		// This texture will consistently be the same; the Mip inside will be rebuilt if needed, though
		PreviewImage->SetBrushFromTexture(PreviewImageTexture, true);
		UpdatePreviewTexture(true);
	}

	if (GraphImage)
	{
		// This texture is rebuilt from the CanvasRenderTarget every time
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

	// Send slate notification of result
	FNotificationInfo info(
		FText::FromString(state.Value)
	);
	info.ExpireDuration = state.Key ? 1.5f : 5.f;
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

	// Long package name we want to export to, e.g. '/Game/MyFolder/T_MyPackageName'
	const FString longPackageName{ FPaths::Combine(TerrainColorOutputDirectory.Path, TerrainColorOutputAssetName) };

	// The outer package that will contain the texture assest; creates or finds if it already exists
	UPackage* package{ CreatePackage(*longPackageName) };
	if (!package)
	{
		return { false, FString::Printf(TEXT("Failed to create package at %s. Check the file path."), *longPackageName) };
	}

	UTexture2D* texture{};
	bool didCreateNew{ false };
	if (StaticLoadObject(UObject::StaticClass(), nullptr, *longPackageName) == nullptr)
	{
		// If the texture asset doesn't exist yet, create a new one
		texture = NewObject<UTexture2D>(package, *TerrainColorOutputAssetName, RF_Public | RF_Standalone);
		if (!texture) return { false, FString::Printf(TEXT("Failed to create new texture object at %s."), *longPackageName) };
		didCreateNew = true;
	}
	else
	{
		// If the texture already exists, locate it in the package
		TArray<UObject*> objects;
		GetObjectsWithOuter(package, objects, false);
		for (UObject* object : objects)
		{
			if (!object || !object->IsA<UTexture2D>()) continue;
			texture = Cast<UTexture2D>(object);
		}

		if (!texture) return { false, "This package already exists and is not a Texture2D." };
	}

	// Whether newly created or just located, fill the first mip
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
		const ESlateVisibility vis{ GraphMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed };
		GraphScrollBox->SetVisibility(vis);
		
		if (GraphMode) UpdateGraphTexture();
	}
	else if (changed == GET_MEMBER_NAME_CHECKED(ThisClass, TerrainMapConnections))
	{
		for (FTerrainGraphConnection& conn : TerrainMapConnections)
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
		TerrainColorSet = TerrainColorPresets[TerrainColorPreset];
		Algo::RandomShuffle(TerrainColorSet);
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
	// have to change the var's content if we need to resize
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

	// Initialize the source data (don't need to initialize platform data/mips here, they will be generated)
	texture->Source.Init(
		TextureSize.X, TextureSize.Y, 1, 1,
		TSF_BGRA8, reinterpret_cast<uint8*>(pixelData.GetData())
	);
	texture->MipGenSettings = TMGS_NoMipmaps;

	// generate & update RHI resource
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

	// Always recreate graph texture from render target; has to be done, the only way
	// to go RT -> Tex is ConstructTexture, which can only create not replace
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
		FMath::FloorToInt32(512 * texAspect), 512
	);
	GraphImageRT->ClearColor = FColor(0, 0, 0, 0);

	GraphImageRT->OnCanvasRenderTargetUpdate.AddDynamic(this, &ThisClass::DrawGraphTexture);
}

void UTerrainPainterWidget::DrawGraphTexture(UCanvas* Canvas, int32 Width, int32 Height)
{
	if (GenerationData.IsEmpty()) return;

	for (const FTerrainGraphConnection& conn : TerrainMapConnections)
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
		DrawDebugCanvas2DLine(Canvas, pos1, pos2, FLinearColor::Black, 2.f);
		DrawDebugCanvas2DLine(Canvas, pos1, pos2, FLinearColor::White, 1.f);
	}
	
	for (int32 i{}; i < GenerationData.Num(); ++i)
	{
		const FTerrainGraphNode& data{ GenerationData[i] };
	
		FVector2D pos{ data.UVCoordinates.X * Width, data.UVCoordinates.Y * Height };

		// Draw background circle for node
		for (int32 j{ 1 }; j <= 10; ++j)
		{
			DrawDebugCanvas2DCircle(Canvas, pos, j, 20, FLinearColor::White);
		}
		DrawDebugCanvas2DCircle(Canvas, pos, 12, 20, FLinearColor::Black);

		// Draw text of index on top of circle
		FString text{ FString::FromInt(i) };
		FCanvasTextItem TextItem(
			pos - FVector2D{ 6, 12 },
			FText::FromString(text),
			GEditor->GetMediumFont(),
			FLinearColor::Black
		);
		TextItem.Scale = FVector2D(1.5f, 1.5f);
		TextItem.EnableShadow(FLinearColor::Gray);
		Canvas->DrawItem(TextItem);
	}
}

void UTerrainPainterWidget::CleanupGraph()
{
	TArray<FTerrainGraphConnection> newArray{};
	for (FTerrainGraphConnection& conn : TerrainMapConnections)
	{
		if (conn.Element1 == conn.Element2) continue;
		if (conn.Element1 > conn.Element2) conn.Swap();
		newArray.AddUnique(conn);
	}
	Algo::SortBy(newArray, [](const FTerrainGraphConnection& a){ return a.Element1; });
	TerrainMapConnections = newArray;
}

void UTerrainPainterWidget::ApplyGraphColoring()
{
	CleanupGraph();
	
	GraphHelper helper(GenerationData, TerrainMapConnections, TerrainColorSet);
	helper.ColorGraph(GraphColoringAlgorithm);

	UpdatePreviewTexture();
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
	for (const FTerrainGraphNode& data : GenerationData)
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
