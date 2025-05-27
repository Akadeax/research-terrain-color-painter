#pragma once

#include "Components/SinglePropertyView.h"
#include "Components/DetailsView.h"

inline void SetupSinglePropertyView(UObject* Object, USinglePropertyView* PropertyView, const FName& PropertyName)
{
	if (PropertyView)
	{
		PropertyView->SetObject(Object);
		PropertyView->SetPropertyName(PropertyName);
	}
}

inline void SetupDetailsView(UObject* Object, UDetailsView* DetailsView, const TArray<FName>& Categories, const TArray<FName>& Properties)
{
	if (DetailsView)
	{
		DetailsView->SetObject(Object);
		DetailsView->CategoriesToShow  = Categories;
		DetailsView->PropertiesToShow = Properties;
	}
}