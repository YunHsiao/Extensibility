<!--
SPDX-FileCopyrightText: 2024 Yun Hsiao Wu <yunhsiaow@gmail.com>
SPDX-License-Identifier: MIT
-->

# Extensibility+

A set of extensibility updates for Unreal Engine.

Rely on the [Crysknife](https://github.com/YunHsiao/Crysknife) plugin under the hood.

By design this is a general-purpose infrastructure, all feature-specific implementations should be placed into a separate plugin, and declare this one as a dependency:

In your `SourcePatch/Crysknife.ini`:
```ini
[Dependencies]
Extensibility=EXTENSIBILITY_EDITOR=${YOUR_PLUGIN_EDITOR}
+Extensibility=EXTENSIBILITY_RUNTIME=${YOUR_PLUGIN_RUNTIME}
```

## Feature List

* Plugin support for the `UnrealLightmass` program
* Framework to initiate custom Lightmass build from plugin
