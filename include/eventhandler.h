/*! \file   eventhandler.h (Mock for testing)
 * \brief  Event handler plugin structures (mock)
 * This is a simplified mock version for build verification only.
 */

#ifndef JANUS_EVENTHANDLER_H
#define JANUS_EVENTHANDLER_H

#include <glib.h>
#include <jansson.h>

#define JANUS_EVENTHANDLER_API_VERSION 5

/* Event types */
#define JANUS_EVENT_TYPE_NONE        0
#define JANUS_EVENT_TYPE_SESSION     (1 << 0)
#define JANUS_EVENT_TYPE_HANDLE       (1 << 1)
#define JANUS_EVENT_TYPE_JSEP         (1 << 2)
#define JANUS_EVENT_TYPE_WEBRTC       (1 << 3)
#define JANUS_EVENT_TYPE_MEDIA        (1 << 4)
#define JANUS_EVENT_TYPE_PLUGIN       (1 << 5)
#define JANUS_EVENT_TYPE_TRANSPORT    (1 << 6)
#define JANUS_EVENT_TYPE_CORE         (1 << 7)
#define JANUS_EVENT_TYPE_ALL          0xFFFFFFFF

/* Forward declaration */
typedef struct janus_eventhandler janus_eventhandler;

/* Event handler plugin structure */
struct janus_eventhandler {
    int (*init)(const char *config_path);
    void (*destroy)(void);
    int (*get_api_compatibility)(void);
    int (*get_version)(void);
    const char *(*get_version_string)(void);
    const char *(*get_description)(void);
    const char *(*get_name)(void);
    const char *(*get_author)(void);
    const char *(*get_package)(void);
    void (*incoming_event)(json_t *event);
    json_t *(*handle_request)(json_t *request);
    guint32 events_mask;
};

#define JANUS_EVENTHANDLER_INIT(...) { __VA_ARGS__ }

#endif
