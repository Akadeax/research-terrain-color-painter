#pragma once


inline FLinearColor NormalizeToMax(const FLinearColor& Color)
{
	const float max{ Color.GetMax() };
	if (max <= 1.f)
	{
		return Color.GetClamped();
	}

	FLinearColor scaled{ Color / max };
	scaled.A = Color.A;
	return scaled;
}
