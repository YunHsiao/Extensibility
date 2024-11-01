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
	template<typename T> static auto HasLaunchCustomSystemImpl(int) -> std::is_member_function_pointer<decltype(&T::LaunchCustomSystem)>;
	template<typename T> static auto HasLaunchCustomSystemImpl(long) -> std::false_type;
public:
	template<typename T>
	static constexpr bool HasLaunchCustomSystem = decltype(HasLaunchCustomSystemImpl<T>(0))::value;
	static constexpr bool IsPatched = HasLaunchCustomSystem<FStaticLightingManager>;
};

#if UE_VERSION_NEWER_THAN(5, 1, 0)
#include "ActorFactories/ActorFactory.h"
using CreateBrushForVolumeActor = UActorFactory::CreateBrushForVolumeActor;
#else
extern UNREALED_API void CreateBrushForVolumeActor(AVolume* NewActor, UBrushBuilder* BrushBuilder);
#endif

extern UNREALED_API void UpdateLevelBounds(ULevel* Level);
extern UNREALED_API void SavePackageWithConsistentGuid(UPackage* Package, const FString& Filename);
