Shell Component
----------------------
The Shell component provides generic commands to interact with the GenAVB/TSN Stack.

This includes:
 - common
 - fdb
 - fp
 - frer
 - hsr
 - psfp
 - qbv
 - stream identification
 - vlan

Requirements
----------------------
Applications must provide a `shell_config.h` header that defines:
 - `shell_printf()`: Shell output function

Example shell_config.h for Zephyr:
```c
#include <zephyr/shell/shell.h>

#define shell_printf(sh, ...) shell_fprintf(sh, SHELL_NORMAL, __VA_ARGS__)
```

Component API
----------------------
Each component provides a public header in `rtos_apps/shell/<component>.h` containing:
 - Command handler functions
 - Help text macros
 - Init function (if needed)

Example of a command - FDB component (`rtos_apps/shell/fdb.h`):

**Commands:**
 - `cmd_fdb_update()`: Add/update forwarding database entry

**Help macros:**
 - `CMD_FDB_UPDATE_HELP`: Usage text for update command

All components follow this pattern. See individual headers for component-specific details.

Usage in Zephyr Applications
----------------------
Example integration using FDB commands:

```c
#include <zephyr/shell/shell.h>
#include "rtos_apps/shell/fdb.h"

SHELL_STATIC_SUBCMD_SET_CREATE(genavb_cmd_fdb,
    SHELL_CMD(update, NULL, CMD_FDB_UPDATE_HELP, &cmd_fdb_update),
    SHELL_SUBCMD_SET_END
);
```

This pattern applies to all shell components (vlan, qbv, psfp, etc.).

Usage in FreeRTOS Applications
----------------------
Example integration using FDB commands:

```c
#include "fsl_shell.h"
#include "rtos_apps/shell/fdb.h"

#define SHELL_WRAPPER(cmd_name) \
	static shell_status_t _##cmd_name(shell_handle_t shell, int32_t argc, char **argv) \
	{ \
    	if (cmd_name((void *)shell, argc, argv) < 0) \
        	return kStatus_SHELL_Error; \
    	return kStatus_SHELL_Success; \
	}

SHELL_WRAPPER(cmd_fdb_update);


SHELL_COMMAND_DEFINE(fdb_update,
                     CMD_FDB_UPDATE_HELP,
                     &_cmd_fdb_update,
                     SHELL_IGNORE_PARAMETER_COUNT);
```

This pattern applies to all shell components (vlan, qbv, psfp, etc.).
