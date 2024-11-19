// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Lightmass/Lightmass.h"
#include "StaticLightingSystem/StaticLightingPrivate.h"

class UNREALED_API FCustomStaticLightingSystem : public FStaticLightingSystem
{
public:
	using FStaticLightingSystem::FStaticLightingSystem;
};

class UNREALED_API FCustomLightmassProcessor : public FLightmassProcessor
{
public:
	using FLightmassProcessor::FLightmassProcessor;
};

class UNREALED_API FCustomLightmassExporter : public FLightmassExporter
{
public:
	using FLightmassExporter::FLightmassExporter;
};

class FEditorExtensibilityUtils final
{
	// ReSharper disable CppFunctionIsNotImplemented
	template<typename T> static auto HasLaunchCustomSystemImpl(int) -> std::is_member_function_pointer<decltype(&T::LaunchCustomSystem)>;
	template<typename T> static auto HasLaunchCustomSystemImpl(long) -> std::false_type;
	// ReSharper restore CppFunctionIsNotImplemented
public:
	template<typename T>
	static constexpr bool HasLaunchCustomSystem = decltype(HasLaunchCustomSystemImpl<T>(0))::value;
	static constexpr bool IsPatched = HasLaunchCustomSystem<FStaticLightingManager>;
};

extern UNREALED_API void CreateBrushForVolumeActorHelper(AVolume* NewActor, UBrushBuilder* BrushBuilder);
extern UNREALED_API void UpdateLevelBounds(ULevel* Level);
extern UNREALED_API void SavePackageWithConsistentGuid(UPackage* Package);

template<typename ActorType>
static ActorType* FindOrCreateActor(ULevel* Level)
{
	ActorType* Result;
	if (auto* ActorIt = Level->Actors.FindByPredicate([](const AActor* Actor)
		{
			return Cast<ActorType>(Actor);
		}))
	{
		Result = Cast<ActorType>(*ActorIt);
	}
	else
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.OverrideLevel = Level;
		Result = GWorld->SpawnActor<ActorType>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}
	return Result;
}
