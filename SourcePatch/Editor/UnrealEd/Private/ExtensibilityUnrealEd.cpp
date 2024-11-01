// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#include "ExtensibilityUnrealEd.h"

#include "PackageHelperFunctions.h"
#include "Engine/LevelBounds.h"

#if !UE_VERSION_NEWER_THAN(5, 1, 0)
#include "BSPOps.h"
#include "Components/BrushComponent.h"
#include "Engine/BrushBuilder.h"
void CreateBrushForVolumeActor( AVolume* NewActor, UBrushBuilder* BrushBuilder )
{
	if ( NewActor != nullptr )
	{
		// this code builds a brush for the new actor
		NewActor->PreEditChange(nullptr);

		// Use the same object flags as the owner volume
		EObjectFlags ObjectFlags = NewActor->GetFlags() & (RF_Transient | RF_Transactional);

		NewActor->PolyFlags = 0;
		NewActor->Brush = NewObject<UModel>(NewActor, NAME_None, ObjectFlags);
		NewActor->Brush->Initialize(nullptr, true);
		NewActor->Brush->Polys = NewObject<UPolys>(NewActor->Brush, NAME_None, ObjectFlags);
		NewActor->GetBrushComponent()->Brush = NewActor->Brush;
		if(BrushBuilder != nullptr)
		{
			NewActor->BrushBuilder = DuplicateObject<UBrushBuilder>(BrushBuilder, NewActor);
		}

		BrushBuilder->Build( NewActor->GetWorld(), NewActor );

		FBSPOps::csgPrepMovingBrush( NewActor );

		// Set the texture on all polys to nullptr.  This stops invisible textures
		// dependencies from being formed on volumes.
		if ( NewActor->Brush )
		{
			for ( int32 poly = 0 ; poly < NewActor->Brush->Polys->Element.Num() ; ++poly )
			{
				FPoly* Poly = &(NewActor->Brush->Polys->Element[poly]);
				Poly->Material = nullptr;
			}
		}

		NewActor->PostEditChange();
	}
}
#endif

void UpdateLevelBounds(ULevel* Level)
{
	ALevelBounds* LevelBounds = Level->LevelBoundsActor.Get();
	if (!LevelBounds)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = Level;
		LevelBounds = Level->GetWorld()->SpawnActor<ALevelBounds>(SpawnParameters);
		Level->LevelBoundsActor = LevelBounds;
	}
	LevelBounds->UpdateLevelBoundsImmediately();

	if (UPackage* Package = Level->GetPackage())
	{
		FBox WorldBounds = LevelBounds->GetComponentsBoundingBox();
		FVector WorldPosition(int32(WorldBounds.GetCenter().X), int32(WorldBounds.GetCenter().Y), 0.f);
		FVector WorldPositionParent(0.f, 0.f, 0.f);

		FVector LocalPosition = WorldPosition - WorldPositionParent;
		FBox LocalBounds = WorldBounds.ShiftBy(-WorldPosition);

		Package->WorldTileInfo->Bounds = LocalBounds;
		Package->WorldTileInfo->AbsolutePosition = FIntVector(WorldPosition.X, WorldPosition.Y, WorldPosition.Z);
		Package->WorldTileInfo->Position = FIntVector(LocalPosition.X, LocalPosition.Y, LocalPosition.Z);

		(void)Level->MarkPackageDirty();
	}
}

extern UNREALED_API void SavePackageWithConsistentGuid(UPackage* Package, const FString& Filename)
{
	SavePackageHelper(Package, Filename, RF_Standalone, GWarn, nullptr, SAVE_KeepGUID);
}