/*******************************************************************************
 * settings.h - Settings window (C interface for Objective-C)
 ******************************************************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Show the settings window (creates it on first call, fronts it after).
 * Edits are written to the config JSON via config_save(); the driver's
 * hot-reload applies them. `driver_ctx` is a DriverContext*.
 * Must be called from the main thread.
 */
void settings_show(void *driver_ctx);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H
