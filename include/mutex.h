/*! \file   mutex.h (Mock for testing)
 * \brief  Mutex wrapper macros (mock)
 * This is a simplified mock version for build verification only.
 */

#ifndef JANUS_MUTEX_H
#define JANUS_MUTEX_H

#include <glib.h>

/* Mutex wrapper */
typedef GMutex janus_mutex;

#define janus_mutex_init(a) g_mutex_init(a)
#define janus_mutex_lock(a) g_mutex_lock(a)
#define janus_mutex_unlock(a) g_mutex_unlock(a)
#define janus_mutex_clear(a) g_mutex_clear(a)

#endif
