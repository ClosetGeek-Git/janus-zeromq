/*! \file   transport.h (Mock for testing)
 * \brief  Transport plugin structures and callbacks (mock)
 * This is a simplified mock version for build verification only.
 */

#ifndef JANUS_TRANSPORT_H
#define JANUS_TRANSPORT_H

#include <glib.h>
#include <jansson.h>

#define JANUS_TRANSPORT_API_VERSION 9

/* Forward declarations */
typedef struct janus_transport janus_transport;
typedef struct janus_transport_session janus_transport_session;
typedef struct janus_transport_callbacks janus_transport_callbacks;

/* Transport session structure */
struct janus_transport_session {
    void *transport_p;
    void *transport_data;
};

/* Transport callbacks */
struct janus_transport_callbacks {
    void (*incoming_request)(janus_transport *plugin, janus_transport_session *transport, void *request_id, gboolean admin, json_t *message, json_error_t *error);
};

/* Transport plugin structure */
struct janus_transport {
    int (*init)(janus_transport_callbacks *callback, const char *config_path);
    void (*destroy)(void);
    int (*get_api_compatibility)(void);
    int (*get_version)(void);
    const char *(*get_version_string)(void);
    const char *(*get_description)(void);
    const char *(*get_name)(void);
    const char *(*get_author)(void);
    const char *(*get_package)(void);
    gboolean (*is_janus_api_enabled)(void);
    gboolean (*is_admin_api_enabled)(void);
    int (*send_message)(janus_transport_session *transport, void *request_id, gboolean admin, json_t *message);
    void (*session_created)(janus_transport_session *transport, guint64 session_id);
    void (*session_over)(janus_transport_session *transport, guint64 session_id, gboolean timeout, gboolean claimed);
    void (*session_claimed)(janus_transport_session *transport, guint64 session_id);
    json_t *(*query_transport)(json_t *request);
};

#define JANUS_TRANSPORT_INIT(...) { __VA_ARGS__ }

#endif
