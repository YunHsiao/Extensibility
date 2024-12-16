// SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "UObject/UObjectBaseUtility.h"
#include "GameFramework/Actor.h"
#include "ExtensibilityCore.h"

inline void MarkAsGarbage(UObject* Object)
{
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	Object->MarkPendingKill();
#else
	Object->MarkAsGarbage();
#endif
}

inline void MarkComponentsAsGarbage(AActor* Actor)
{
#if UE_VERSION_OLDER_THAN(5, 3, 0)
	Actor->MarkComponentsAsPendingKill();
#else
	Actor->MarkComponentsAsGarbage();
#endif
}
