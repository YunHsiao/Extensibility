// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

// Include this file in one and only one of your cpp file
// to work around potential link issues:

// #define EXTENSIBILITY_LIGHTMASS_POLYFILLS
// #include "ExtensibilityLightmass.cpp"

#ifdef EXTENSIBILITY_LIGHTMASS_POLYFILLS

#include "Exporter.h"
#include "LightingSystem.h"
#include "LightmassSwarm.h"

// ReSharper disable CppUnusedIncludeDirective
#include "Collision.cpp"
#include "Embree.cpp"
#include "Importer.cpp"
#include "LightmassSwarm.cpp"
#include "LMMAth.cpp"
#include "LMThreading.cpp"
#include "MonteCarlo.cpp"
#include "SFMT.cpp"
#include "StaticMesh.cpp"
// ReSharper restore CppUnusedIncludeDirective

DEFINE_LOG_CATEGORY(LogLightmass);
namespace Lightmass
{
	FGlobalStatistics GStatistics;
	FLightmassSwarm* GSwarm = NULL;
	bool GReportDetailedStats = false;
	bool GDebugMode = false;
	double GSecondPerCPUCycle = 1.0 / 3000000000.0;

	FBoxSphereBounds3f FScene::GetImportanceBounds() const
	{
		const FBoxSphereBounds3f ImportanceBoundSphere(ImportanceBoundingBox);
		return ImportanceBoundSphere;
	}

	FLightmassSwarm* FLightmassSolverExporter::GetSwarm()
	{
		return Swarm;
	}

	void FStaticLightingMesh::SetDebugMaterial(bool bInUseDebugMaterial, FLinearColor InDiffuse)
	{
		bUseDebugMaterial = bInUseDebugMaterial;
		DebugDiffuse = InDiffuse;
	}

	bool FStaticLightingMesh::DoesMeshBelongToLOD0() const
	{
		const uint32 GeoMeshLODIndex = GetLODIndices() & 0xFFFF;
		const uint32 GeoHLODTreeIndex = (GetLODIndices() & 0xFFFF0000) >> 16;
		const uint32 GeoHLODRange = GetHLODRange();
		const uint32 GeoHLODRangeStart = GeoHLODRange & 0xFFFF;
		const uint32 GeoHLODRangeEnd = (GeoHLODRange & 0xFFFF0000) >> 16;

		bool bMeshBelongsToLOD0 = GeoMeshLODIndex == 0;

		if (GeoHLODTreeIndex > 0)
		{
			bMeshBelongsToLOD0 = GeoHLODRangeStart == GeoHLODRangeEnd;
		}

		return bMeshBelongsToLOD0;
	}

	// Write your own implementation instead
	void FStaticLightingMesh::Import(FLightmassImporter&) { checkNoEntry(); }
	void FStaticLightingTextureMapping::Import(FLightmassImporter&) { checkNoEntry(); }
	void FBaseMesh::Import(FLightmassImporter&) { checkNoEntry(); }
	void FStaticMesh::Import(FLightmassImporter&) { checkNoEntry(); }
	void FStaticMeshLOD::Import(FLightmassImporter&) { checkNoEntry(); }

	template<typename DataType>
	void TCompleteTaskList<DataType>::ApplyAndClear(FStaticLightingSystem& LightingSystem)
	{
		while(this->FirstElement)
		{
			// Atomically read the complete list and clear the shared head pointer.
			TList<DataType>* LocalFirstElement;
			TList<DataType>* CurrentElement;
			uint32 ElementCount = 0;

			do { LocalFirstElement = this->FirstElement; }
			while(FPlatformAtomics::InterlockedCompareExchangePointer((void**)&this->FirstElement,NULL,LocalFirstElement) != LocalFirstElement);

			// Traverse the local list, count the number of entries, and find the minimum guid
			TList<DataType>* PreviousElement = NULL;
			TList<DataType>* MinimumElementLink = NULL;
			TList<DataType>* MinimumElement = NULL;

			CurrentElement = LocalFirstElement;
			MinimumElement = CurrentElement;
			FGuid MinimumGuid = MinimumElement->Element.Guid;

			while(CurrentElement)
			{
				ElementCount++;
				if (CurrentElement->Element.Guid < MinimumGuid)
				{
					MinimumGuid = CurrentElement->Element.Guid;
					MinimumElementLink = PreviousElement;
					MinimumElement = CurrentElement;
				}
				PreviousElement = CurrentElement;
				CurrentElement = CurrentElement->Next;
			}
			// Slice and dice the list to put the minimum at the head before we continue
			if (MinimumElementLink != NULL)
			{
				MinimumElementLink->Next = MinimumElement->Next;
				MinimumElement->Next = LocalFirstElement;
				LocalFirstElement = MinimumElement;
			}

			// Traverse the local list and export
			CurrentElement = LocalFirstElement;

			const double ExportTimeStart = FPlatformTime::Seconds();
			while(CurrentElement)
			{
				// write back to Unreal
				LightingSystem.GetExporter().ExportResults(CurrentElement->Element);

				// Move to the next element
				CurrentElement = CurrentElement->Next;
			}

			// Traverse again, cleaning up and notifying swarm
			FLightmassSwarm* Swarm = LightingSystem.GetExporter().GetSwarm();
			CurrentElement = LocalFirstElement;
			while(CurrentElement)
			{
				// Tell Swarm the task is complete (if we're not in debugging mode).
				if ( !LightingSystem.IsDebugMode() )
				{
					Swarm->TaskCompleted( CurrentElement->Element.Guid );
				}

				// Delete this link and advance to the next.
				TList<DataType>* NextElement = CurrentElement->Next;
				delete CurrentElement;
				CurrentElement = NextElement;
			}
		}
	}
}

#endif
