#pragma once

inline void SetupSinglePropertyView(UObject* Object, USinglePropertyView* PropertyView, const FName& PropertyName)
{
	if (PropertyView)
	{
		PropertyView->SetObject(Object);
		PropertyView->SetPropertyName(PropertyName);
	}
}