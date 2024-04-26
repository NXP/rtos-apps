RTOS Application Layer
----------------------
The RTOS Application Layer provides several re-usable components that may be integrated into larger applications.
It aims to provide a common repository to maximize code sharing across different applications and/or environments.

Supported application components
-----------------------
- logging (component name: log)
- statistics (component name: stats)

Using the RTOS Application Layer
------------------------------
Each of the components provides:
- A cmake submodule file: rtos_apps_\<component\>.cmake
- An header file: rtos_apps/\<component\>.h

On the applications' CMake, add the following lines:

#### FreeRTOS
```cmake
set(RTOS_APPS_TARGET <application target name>)

set(CMAKE_MODULE_PATH ... <rtos_apps_top_directory>)

include(rtos_apps_<component_a>.cmake)
...
include(rtos_apps_<component_b>.cmake)
```

#### Zephyr
```cmake
set(RTOS_APPS_TARGET <application target name>)

set(CMAKE_MODULE_PATH ... <rtos_apps_top_directory>)

include(rtos_apps_<component_a>.cmake)
...
include(rtos_apps_<component_b>.cmake)
```

Sources will be added to the specified target, as well as required include paths.
To use the application layer, include the component specific header file:
```
#include <rtos_apps/\<component\>.h>
```