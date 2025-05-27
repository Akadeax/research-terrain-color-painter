// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FTerrainPainterModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();

	void OpenTerrainPainterWidget();

	// TODO: Try this with scoped again, can now be docked. Wtf?
	FToolMenuOwner TerrainPainterOwner;
};
