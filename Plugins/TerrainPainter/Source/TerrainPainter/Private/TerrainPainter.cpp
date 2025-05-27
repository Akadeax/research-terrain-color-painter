// Copyright Epic Games, Inc. All Rights Reserved.

#include "TerrainPainter.h"

#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"

#define LOCTEXT_NAMESPACE "FTerrainPainterModule"

void FTerrainPainterModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTerrainPainterModule::RegisterMenus));
}

void FTerrainPainterModule::ShutdownModule()
{
	UToolMenus::UnregisterOwner(FToolMenuOwner(this));
	UToolMenus::UnRegisterStartupCallback(this);
}

void FTerrainPainterModule::RegisterMenus()
{
	FToolMenuOwnerScoped menuOwnerScoped(this);
	
	// Extend an existing menu
	UToolMenu* menu{ UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window") };

	// Add a new section to that menu (grouping your entries)
	FToolMenuSection& section{ menu->FindOrAddSection(
		"MenuSectionTerrainPainter",
		FText::FromString(TEXT("Terrain Painter"))
	) };

	// Add a menu entry in that section
	section.AddMenuEntry(
		"MenuEntryOpenTerrainPainter",
		FText::FromString(TEXT("Open Terrain Painter")),
		FText::FromString(TEXT("Open Terrain Painter, which can bake terrain color textures from a node graph")),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Toolbar.Play"),
		FToolUIActionChoice(FExecuteAction::CreateRaw(this, &FTerrainPainterModule::OpenTerrainPainterWidget))
	);
}

void FTerrainPainterModule::OpenTerrainPainterWidget()
{
	UEditorUtilitySubsystem* editorUtility{ GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>() };
	checkf(editorUtility, TEXT("Editor utility subsystem couldn't be loaded."));

	const FString widgetClassPath{ TEXT("/TerrainPainter/Widgets/EUW_TerrainPainterWidget.EUW_TerrainPainterWidget") };
	
	UEditorUtilityWidgetBlueprint* widgetClass{ LoadObject<UEditorUtilityWidgetBlueprint>(nullptr, *widgetClassPath) };
	if (!ensureAlwaysMsgf(widgetClass,
		TEXT("Couldn't find Terrain Painter Widget class! Make sure the package is located at the correct path.")
		)) return;
	
	editorUtility->SpawnAndRegisterTab(widgetClass);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTerrainPainterModule, TerrainPainter)