#pragma once

/**
 * ChimeraPhoenix Debug Flags
 *
 * Compile-time debug control for GPIO/hardware development.
 * Set CHI_DEV_LOG to 1 to enable targeted diagnostic logging.
 */

// Enable/disable development logging (set to 0 for production builds)
#define CHI_DEV_LOG 1

#if CHI_DEV_LOG
    #define CHI_LOG(...) DBG(__VA_ARGS__)
#else
    #define CHI_LOG(...)
#endif
