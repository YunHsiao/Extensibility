// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#include "ExtensibilityUnrealEd.h"

#include "PackageHelperFunctions.h"
#include "Engine/LevelBounds.h"

#if UE_VERSION_NEWER_THAN(5, 1, 0)
#include "ActorFactories/ActorFactory.h"
void CreateBrushForVolumeActorHelper(AVolume* NewActor, UBrushBuilder* BrushBuilder)
{
	UActorFactory::CreateBrushForVolumeActor(NewActor, BrushBuilder);
}
#else
extern void CreateBrushForVolumeActor(AVolume* NewActor, UBrushBuilder* BrushBuilder);
void CreateBrushForVolumeActorHelper(AVolume* NewActor, UBrushBuilder* BrushBuilder)
{
	CreateBrushForVolumeActor(NewActor, BrushBuilder);
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

void SavePackageWithConsistentGuid(UPackage* Package)
{
	FString LevelFileName;
	ensure(FPackageName::TryConvertLongPackageNameToFilename(Package->GetPathName(), LevelFileName, FPackageName::GetMapPackageExtension()));
	SavePackageHelper(Package, LevelFileName, RF_Standalone, GWarn, nullptr, SAVE_KeepGUID);
}
