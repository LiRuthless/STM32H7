#ifndef BRIDGE_JSON_GUARD_H
#define BRIDGE_JSON_GUARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BRIDGE_JSON_MAX_BYTES 992u
/* Every container consumes two bytes, so this covers all legal payloads.
 * Caller owns and exclusively lends this workspace for the duration of a call.
 * No heap, recursion, global parser state or borrowed scratch string escapes. */
typedef struct {
    uint16_t object_start[BRIDGE_JSON_MAX_BYTES / 2u + 1u];
    uint8_t state[BRIDGE_JSON_MAX_BYTES / 2u + 1u];
} bridge_json_workspace_t;

typedef enum {
    BRIDGE_JSON_OK = 0, BRIDGE_JSON_ARGUMENT, BRIDGE_JSON_SYNTAX,
    BRIDGE_JSON_UTF8, BRIDGE_JSON_DUPLICATE_KEY, BRIDGE_JSON_NONFINITE
} bridge_json_error_t;

typedef struct {
    const uint8_t *bytes;
    size_t length;
} bridge_json_view_t;

bridge_json_error_t bridge_json_validate(const uint8_t *bytes, size_t length,
                                        bridge_json_workspace_t *workspace);
/* Validates the complete object before returning a raw, bounded value view.
 * name is an ASCII key. The view borrows input, never workspace storage.
 * Missing/invalid input clears view and returns false; no partial dispatch. */
bool bridge_json_object_get(const uint8_t *bytes, size_t length, const char *name,
                            bridge_json_workspace_t *workspace, bridge_json_view_t *view);

#endif
