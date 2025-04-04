// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#include "ExtensibilityUnrealEd.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/EngineBuildSettings.h"
#include "Misc/PrivateAccessor.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
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

TSet<FString> FLightmassExporter::GetPluginBinaryDependencies(bool bIs64Bit, bool bIsOptional) const
{
#if PLATFORM_WINDOWS
	static const FString BinaryPlatform32 = TEXT("Win32");
	static const FString BinaryPlatform64 = TEXT("Win64");
	const FString& Platform = bIs64Bit ? BinaryPlatform64 : BinaryPlatform32;
	static const FString BinaryExtension = TEXT("dll");
	const FString BinaryPrefix;
#elif PLATFORM_MAC 
	static const FString Platform = TEXT("Mac");
	static const FString BinaryExtension = TEXT("dylib");
	const FString BinaryPrefix;
#elif PLATFORM_LINUX 
	static const FString Platform = TEXT("Linux");
	static const FString BinaryExtension = TEXT("so");
	const FString BinaryPrefix = TEXT("lib");
#else
#error "Unknown Lightmass platform"
#endif

	TSet<FString> Paths;
	TSharedPtr<FJsonObject> JsonParsed;
	FString ManifestPath = FString::Printf(TEXT("Binaries/%s/UnrealLightmass.modules"), *Platform);

	if (bIsOptional)
	{
		Paths.Emplace(FString::Printf(TEXT("Binaries/%s/%sUnrealLightmass-Core.pdb"), *Platform, *BinaryPrefix));
	}
	else
	{
		Paths.Emplace(ManifestPath);
		TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*(FPaths::EngineDir() / ManifestPath)));

		if (Ar)
		{
			auto Reader = TJsonReaderFactory<ANSICHAR>::Create(Ar.Get());
			FJsonSerializer::Deserialize(Reader, JsonParsed);
		}
	}

	TArray<FString> Modules;
	DependentPluginModules.ParseIntoArray(Modules, TEXT(" "));
	bool bManifestNeedsUpdate = false;

	for (FString& Pair : Modules)
	{
		FString Plugin, Module;
		check(Pair.Split(TEXT(":"), &Plugin, &Module));
		if (bIsOptional)
		{
			Paths.Emplace(FString::Printf(TEXT("Plugins/%s/Binaries/%s/%sUnrealLightmass-%s.pdb"), *Plugin, *Platform, *BinaryPrefix, *Module));
		}
		else
		{
			FString BinaryName = FString::Printf(TEXT("%sUnrealLightmass-%s.%s"), *BinaryPrefix, *Module, *BinaryExtension);
			Paths.Emplace(FString::Printf(TEXT("Plugins/%s/Binaries/%s/%s"), *Plugin, *Platform, *BinaryName));

			if (const auto& ModuleList = JsonParsed->GetObjectField(TEXT("Modules")))
			{
				if (!ModuleList->HasField(Module))
				{
					ModuleList->SetStringField(Module, BinaryName);
					bManifestNeedsUpdate = true;
				}
			}
		}
	}

	if (bManifestNeedsUpdate)
	{
		TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*(FPaths::EngineDir() / ManifestPath)));
		if (Ar)
		{
			auto Writer = TJsonWriterFactory<ANSICHAR>::Create(Ar.Get());
			FJsonSerializer::Serialize(JsonParsed.ToSharedRef(), *Writer);
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

#if UE_VERSION_NEWER_THAN(5, 5, 0)
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
