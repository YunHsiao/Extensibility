// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#include "ExtensibilityUnrealEd.h"

#include "Misc/EngineBuildSettings.h"
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

#if UE_VERSION_NEWER_THAN(5, 1, 0)
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

TSet<FString> FLightmassExporter::GetPluginBinaryDependencies(bool bIs64Bit) const
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

	TArray<FString> Modules;
	DependentPluginModules.ParseIntoArray(Modules, TEXT(" "));
	for (FString& Pair : Modules)
	{
		FString Plugin, Module;
		check(Pair.Split(TEXT(":"), &Plugin, &Module));
		Paths.Emplace(FString::Printf(TEXT("Plugins/%s/Binaries/%sUnrealLightmass-%s.%s"), *Plugin, *BinaryPlatform, *Module, *BinaryExtension));
		// Do we need to manually merge this?
		Paths.Emplace(Pair = FString::Printf(TEXT("Plugins/%s/Binaries/%sUnrealLightmass.modules"), *Plugin, *GeneralPlatform));
	}
	return Paths;
}

#undef LOCTEXT_NAMESPACE
