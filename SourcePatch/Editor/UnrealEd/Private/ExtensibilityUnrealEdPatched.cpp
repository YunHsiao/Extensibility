// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#include "ExtensibilityUnrealEd.h"

#include "Misc/EngineBuildSettings.h"
#include "Misc/PrivateAccessor.h"
#include "StaticLightingSystem/StaticLightingPrivate.h"

#define LOCTEXT_NAMESPACE "Lightmass"

void FStaticLightingManager::LaunchCustomSystem(FStaticLightingSystem* CustomSystem)
{
	if (StaticLightingSystems.Num() == 0)
	{
		check(!ActiveStaticLightingSystem);

		bBuildReflectionCapturesOnFinish = false;
		StaticLightingSystems.Emplace(CustomSystem);
		ActiveStaticLightingSystem = StaticLightingSystems[0].Get();

#if !UE_VERSION_OLDER_THAN(5, 1, 0)
		if (ActiveStaticLightingSystem->CheckLightmassExecutableVersion())
#else
		if (true)
#endif
		{
			if (ActiveStaticLightingSystem->BeginLightmassProcess())
			{
				SendProgressNotification();
			}
			else
			{
				Get()->FailLightingBuild();
			}
		}
		else
		{
			if (FEngineBuildSettings::IsSourceDistribution())
			{
				Get()->FailLightingBuild(LOCTEXT("LightmassExecutableOutdatedMessage", "Unreal Lightmass executable is outdated. Recompile UnrealLightmass project with Development configuration in Visual Studio."));
			}
			else
			{
				// Lightmass should never be outdated in a launcher binary build.
				Get()->FailLightingBuild(LOCTEXT("LauncherBuildNeedsVerificationMessage", "Unreal Lightmass executable is damaged. Try verifying your engine installation in Epic Games Launcher."));
			}
		}
	}
	else
	{
		// Tell the user that they must close their current build first.
		FNotificationInfo Info( LOCTEXT("LightBuildInProgressWarning", "A lighting build is already in progress! Please cancel it before triggering a new build.") );
		Info.ExpireDuration = 5.0f;
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(SNotificationItem::CS_Fail);
		}
		delete CustomSystem;
	}
}

void FLightmassExporter::WriteCustomData(int32 Channel, bool bForceContentExport)
{
	// Extensibility+: Lightmass
	// Always use subclasses instead of modifying this function directly.
	// Subclass implementations should always begin with:
	/*
	DependentPluginModules = TEXT("YourPlugin:Module1 YourPlugin:Module2");
	FLightmassExporter::WriteCustomData(Channel, bForceContentExport);
	*/

	int32 NameLength = DependentPluginModules.GetCharArray().Num();
	Swarm.WriteChannel(Channel, &NameLength, sizeof(NameLength));
	Swarm.WriteChannel(Channel, *DependentPluginModules, NameLength * sizeof(TCHAR));
}

TSet<FString> FLightmassExporter::GetPluginBinaryDependencies(bool bIs64Bit, bool bIsOptional) const
{
#if PLATFORM_WINDOWS
	static const FString BinaryPlatform32 = TEXT("Win32/");
	static const FString BinaryPlatform64 = TEXT("Win64/");
	static const FString BinaryExtension = TEXT("dll");
	const FString& BinaryPlatform = bIs64Bit ? BinaryPlatform64 : BinaryPlatform32;
	const FString& GeneralPlatform = BinaryPlatform;
#elif PLATFORM_MAC 
	static const FString BinaryPlatform = TEXT("Mac/");
	static const FString BinaryExtension = TEXT("dylib");
	const FString& GeneralPlatform = BinaryPlatform;
#elif PLATFORM_LINUX 
	static const FString BinaryPlatform = TEXT("Linux/lib");
	static const FString GeneralPlatform = TEXT("Linux/");
	static const FString BinaryExtension = TEXT("so");
#else
#error "Unknown Lightmass platform"
#endif
	TSet<FString> Paths;
	if (bIsOptional)
	{
		Paths.Emplace(FString::Printf(TEXT("Binaries/%sUnrealLightmass-Core.pdb"), *BinaryPlatform));
	}
	else
	{
		Paths.Emplace(FString::Printf(TEXT("Binaries/%sUnrealLightmass.modules"), *GeneralPlatform));
	}

	TArray<FString> Modules;
	DependentPluginModules.ParseIntoArray(Modules, TEXT(" "));
	for (FString& Pair : Modules)
	{
		FString Plugin, Module;
		check(Pair.Split(TEXT(":"), &Plugin, &Module));
		if (bIsOptional)
		{
			Paths.Emplace(FString::Printf(TEXT("Plugins/%s/Binaries/%sUnrealLightmass-%s.pdb"), *Plugin, *BinaryPlatform, *Module));
		}
		else
		{
			Paths.Emplace(FString::Printf(TEXT("Plugins/%s/Binaries/%sUnrealLightmass-%s.%s"), *Plugin, *BinaryPlatform, *Module, *BinaryExtension));
			// Do we need to manually merge this?
			Paths.Emplace(Pair = FString::Printf(TEXT("Plugins/%s/Binaries/%sUnrealLightmass.modules"), *Plugin, *GeneralPlatform));
		}
	}
	return Paths;
}

FCustomStaticLightingSystem::FCustomStaticLightingSystem(const FLightingBuildOptions& InOptions, UWorld* InWorld, ULevel* InLightingScenario)
#if UE_VERSION_OLDER_THAN(5, 5, 0)
	: FStaticLightingSystem(InOptions, InWorld, InLightingScenario)
#else
	: FStaticLightingSystem(InOptions, {InWorld, InLightingScenario})
#endif
{}

FCustomLightmassProcessor::FCustomLightmassProcessor(const FStaticLightingSystem& InSystem, bool bInDumpBinaryResults, bool bInOnlyBuildVisibility)
	: FLightmassProcessor(InSystem, bInDumpBinaryResults, bInOnlyBuildVisibility)
{}

#if !UE_VERSION_OLDER_THAN(5, 5, 0)
DEFINE_PRIVATE_ACCESSOR_VARIABLE(GetLightingContext, FStaticLightingSystem, FStaticLightingBuildContext, LightingContext);
#endif

FCustomLightmassExporter::FCustomLightmassExporter(const FStaticLightingSystem& InSystem)
#if UE_VERSION_OLDER_THAN(5, 5, 0)
	: FLightmassExporter(InSystem.GetWorld())
#else
	: FLightmassExporter(PrivateAccess(InSystem, GetLightingContext))
#endif
{}

FCustomLightmassExporter::~FCustomLightmassExporter() {}
FCustomLightmassProcessor::~FCustomLightmassProcessor() {}
FCustomStaticLightingSystem::~FCustomStaticLightingSystem() {}

#undef LOCTEXT_NAMESPACE
