//Copyright Tencent Timi-J1&F1 Studio, Inc. All Rights Reserved

#pragma once

#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5, 0, 0)

using FVector3d = FVector;
using FVector2d = FVector2D;
using FVector4d = FVector4;
using FQuat4d = FQuat;
using FMatrix44d = FMatrix;
using FPlane4d = FPlane;
using FTransform3d = FTransform;
using FSphere3d = FSphere;
using FBox3d = FBox;
using FBox2d = FBox2D;
using FRotator3d = FRotator;
using FRay3d = FRay;

using FVector3f = FVector;
using FVector2f = FVector2D;
using FVector4f = FVector4;
using FQuat4f = FQuat;
using FMatrix44f = FMatrix;
using FPlane4f = FPlane;
using FTransform3f = FTransform;
using FSphere3f = FSphere;
using FBox3f = FBox;
using FBox2f = FBox2D;
using FRotator3f = FRotator;
using FRay3f = FRay;
	
using FIntVector2 = FIntPoint;
using FIntVector3 = FIntVector;
using FInt32Vector2 = FIntPoint;
using FInt32Vector3 = FIntVector;
using FInt32Vector4 = FIntVector4;
using FInt32Vector = FIntVector;
using FInt64Vector2 = FIntPoint;
using FInt64Vector3 = FIntVector;
using FInt64Vector4 = FIntVector4;
using FInt64Vector = FIntVector;

using FUintVector2 = FIntPoint; 
using FUintVector3 = FIntVector;
using FUint32Vector2 = FIntPoint;
using FUint32Vector3 = FIntVector;
using FUint32Vector4 = FUintVector4;
using FUint32Vector = FIntVector;
using FUint64Vector2 = FIntPoint;
using FUint64Vector3 = FIntVector;
using FUint64Vector4 = FUintVector4;
using FUint64Vector = FIntVector;

using FInt32Point = FIntPoint;
using FInt64Point = FIntPoint;
using FUint32Point = FIntPoint;
using FUint64Point = FIntPoint;
using FUintPoint = FIntPoint;
using FInt32Rect = FIntRect;
using FInt64Rect = FIntRect;
using FUint32Rect = FIntRect;
using FUint64Rect = FIntRect;

using FBoxSphereBounds3f = FBoxSphereBounds;
using FBoxSphereBounds3d = FBoxSphereBounds;
using FCompactBoxSphereBounds3d = FBoxSphereBounds;
using FCompactBoxSphereBounds = FBoxSphereBounds;

#endif
