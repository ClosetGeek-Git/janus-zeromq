/*! \file   utils.h (Mock for testing)
 * \brief  Utility functions (mock)
 * This is a simplified mock version for build verification only.
 */

#ifndef JANUS_UTILS_H
#define JANUS_UTILS_H

#include <glib.h>
#include <string.h>
#include <ctype.h>

/* Check if string represents true value */
static inline gboolean janus_is_true(const char *value) {
    return value && (
        !strcasecmp(value, "yes") ||
        !strcasecmp(value, "true") ||
        !strcasecmp(value, "1")
    );
}

#endif
