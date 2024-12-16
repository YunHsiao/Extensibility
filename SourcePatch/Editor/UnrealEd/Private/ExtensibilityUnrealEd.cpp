// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#include "ExtensibilityUnrealEd.h"

#include "ISourceControlModule.h"
#include "PackageHelperFunctions.h"
#include "SourceControlHelpers.h"
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

static FWorldTileInfo* GetWorldTileInfo(UPackage* Package)
{
#if UE_VERSION_NEWER_THAN(5, 0, 0)
	return Package ? Package->GetWorldTileInfo() : nullptr;
#else
	return Package ? Package->WorldTileInfo.Get() : nullptr;
#endif
}

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

		GetWorldTileInfo(Package)->Bounds = LocalBounds;
		GetWorldTileInfo(Package)->AbsolutePosition = FIntVector(WorldPosition.X, WorldPosition.Y, WorldPosition.Z);
		GetWorldTileInfo(Package)->Position = FIntVector(LocalPosition.X, LocalPosition.Y, LocalPosition.Z);

		(void)Level->MarkPackageDirty();
	}
}

class FSourceControlHelper
{
public:
	FSourceControlHelper()
	{
		auto& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		SourceControlProvider.Init();
		bActive = SourceControlProvider.IsEnabled();
	}

	void Checkout(const FString& FilePath) const
	{
		if (bActive)
		{
			const FSourceControlState State = USourceControlHelpers::QueryFileState(FilePath);

			if (State.bIsSourceControlled && !State.bIsCheckedOut && !State.bIsAdded && State.bCanCheckOut)
			{
				USourceControlHelpers::CheckOutFile(FilePath);
			}
		}
	}

private:
	bool bActive;
};

void SavePackageWithConsistentGuid(UPackage* Package)
{
	static FSourceControlHelper SourceControlHelper;

	FString LevelFileName;
	ensure(FPackageName::TryConvertLongPackageNameToFilename(Package->GetPathName(), LevelFileName, FPackageName::GetMapPackageExtension()));
	SourceControlHelper.Checkout(LevelFileName);

#if UE_VERSION_OLDER_THAN(5, 0, 0)
	SavePackageHelper(Package, LevelFileName, RF_Standalone, GWarn, nullptr, SAVE_KeepGUID);
#else
	SavePackageHelper(Package, LevelFileName, RF_Standalone, GWarn, SAVE_KeepGUID);
#endif
}
