// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
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

class FShakeoutEditorUtils final
{
public:
	template<typename T>
	using THasLaunchCustomSystem = std::is_member_function_pointer<decltype(&T::LaunchCustomSystem)>;
	static constexpr bool bIsPatched = THasLaunchCustomSystem<FStaticLightingManager>::value;
};
