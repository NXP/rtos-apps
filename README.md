RTOS Application Layer
----------------------
The RTOS Application Layer provides several re-usable components that may be integrated into larger applications.
It aims to provide a common repository to maximize code sharing across different applications and/or environments.

Supported application components
-----------------------
- asynchronous processing (component name: async)
- audio pipeline (component name: audio)
- logging (component name: log)
- statistics (component name: stats)
- tsn endpoint (component name: tsn)

Using the RTOS Application Layer
------------------------------
Each of the components provides:
- A cmake submodule file: rtos_apps_\<component\>.cmake
- One or more header file(s): rtos_apps/\<component\>.h or rtos_apps/\<component\>/\<file\>.h

On the applications' CMake, add the following lines:

#### FreeRTOS
```cmake
set(RTOS_APPS_DIR /path/to/rtos-apps)
set(RTOS_APPS_TARGET <application target name>)

include(${RTOS_APPS_DIR}/rtos_apps_<component_a>.cmake)
...
include(${RTOS_APPS_DIR}/rtos_apps_<component_b>.cmake)
```

#### Zephyr
```cmake
set(RTOS_APPS_DIR /path/to/rtos-apps)
set(RTOS_APPS_TARGET <application target name>)

include(${RTOS_APPS_DIR}/rtos_apps_<component_a>.cmake)
...
include(${RTOS_APPS_DIR}/rtos_apps_<component_b>.cmake)
```

Sources will be added to the specified target, as well as required include paths.
To use the application layer, include the component specific header file(s):
```c
#include <rtos_apps/<component>.h>
```
or
```c
#include <rtos_apps/<component>/<file>.h>
```