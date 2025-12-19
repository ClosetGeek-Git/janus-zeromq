/*! \file   janus_zmqevh.c
 * \author Lorenzo Miniero <lorenzo@meetecho.com>
 * \copyright GNU General Public License v3
 * \brief  Janus ZeroMQ event handler plugin
 * \details  This is an implementation of a ZeroMQ event handler for Janus.
 * This plugin allows you to receive Janus events (e.g., new sessions, media
 * statistics, errors, etc.) via ZeroMQ PUB/SUB sockets. The plugin publishes
 * events to a ZeroMQ PUB socket, which can be subscribed to by external
 * applications using ZeroMQ SUB sockets.
 *
 * \ingroup eventhandlers
 * \ref eventhandlers
 */

#include <zmq.h>

#include "eventhandler.h"
#include "debug.h"
#include "config.h"
#include "mutex.h"
#include "utils.h"


/* Plugin information */
#define JANUS_ZMQEVH_VERSION			1
#define JANUS_ZMQEVH_VERSION_STRING		"0.0.1"
#define JANUS_ZMQEVH_DESCRIPTION		"This event handler plugin publishes events via ZeroMQ PUB/SUB sockets."
#define JANUS_ZMQEVH_NAME				"JANUS ZeroMQ event handler plugin"
#define JANUS_ZMQEVH_AUTHOR				"Meetecho s.r.l."
#define JANUS_ZMQEVH_PACKAGE			"janus.eventhandler.zeromqevh"

/* Plugin methods */
janus_eventhandler *create(void);
int janus_zmqevh_init(const char *config_path);
void janus_zmqevh_destroy(void);
int janus_zmqevh_get_api_compatibility(void);
int janus_zmqevh_get_version(void);
const char *janus_zmqevh_get_version_string(void);
const char *janus_zmqevh_get_description(void);
const char *janus_zmqevh_get_name(void);
const char *janus_zmqevh_get_author(void);
const char *janus_zmqevh_get_package(void);
void janus_zmqevh_incoming_event(json_t *event);
json_t *janus_zmqevh_handle_request(json_t *request);

/* Event handler setup */
static janus_eventhandler janus_zmqevh =
	JANUS_EVENTHANDLER_INIT (
		.init = janus_zmqevh_init,
		.destroy = janus_zmqevh_destroy,

		.get_api_compatibility = janus_zmqevh_get_api_compatibility,
		.get_version = janus_zmqevh_get_version,
		.get_version_string = janus_zmqevh_get_version_string,
		.get_description = janus_zmqevh_get_description,
		.get_name = janus_zmqevh_get_name,
		.get_author = janus_zmqevh_get_author,
		.get_package = janus_zmqevh_get_package,

		.incoming_event = janus_zmqevh_incoming_event,
		.handle_request = janus_zmqevh_handle_request,

		.events_mask = JANUS_EVENT_TYPE_NONE
	);

/* Plugin creator */
janus_eventhandler *create(void) {
	JANUS_LOG(LOG_VERB, "%s created!\n", JANUS_ZMQEVH_NAME);
	return &janus_zmqevh;
}


/* Useful stuff */
static gint initialized = 0, stopping = 0;

/* ZeroMQ context and socket */
static void *zmq_context = NULL;
static void *zmq_publisher = NULL;

/* Configuration */
static char *address = NULL;
static uint16_t port = 0;
static gboolean enabled = FALSE;

/* Event queue and thread */
static GAsyncQueue *events = NULL;
static GThread *event_thread = NULL;
static void *janus_zmqevh_thread(void *data);

/* Event structure for queueing */
typedef struct janus_zmqevh_event {
	json_t *event;
} janus_zmqevh_event;


/* Plugin implementation */
int janus_zmqevh_get_api_compatibility(void) {
	return JANUS_EVENTHANDLER_API_VERSION;
}

int janus_zmqevh_get_version(void) {
	return JANUS_ZMQEVH_VERSION;
}

const char *janus_zmqevh_get_version_string(void) {
	return JANUS_ZMQEVH_VERSION_STRING;
}

const char *janus_zmqevh_get_description(void) {
	return JANUS_ZMQEVH_DESCRIPTION;
}

const char *janus_zmqevh_get_name(void) {
	return JANUS_ZMQEVH_NAME;
}

const char *janus_zmqevh_get_author(void) {
	return JANUS_ZMQEVH_AUTHOR;
}

const char *janus_zmqevh_get_package(void) {
	return JANUS_ZMQEVH_PACKAGE;
}

/* Initialization */
int janus_zmqevh_init(const char *config_path) {
	if(g_atomic_int_get(&stopping)) {
		/* Still stopping from before */
		return -1;
	}
	
	/* Initialize ZeroMQ context */
	zmq_context = zmq_ctx_new();
	if(zmq_context == NULL) {
		JANUS_LOG(LOG_FATAL, "Could not initialize ZeroMQ context: %s\n", zmq_strerror(errno));
		return -1;
	}

	/* Set context options */
	zmq_ctx_set(zmq_context, ZMQ_IO_THREADS, 2);
	zmq_ctx_set(zmq_context, ZMQ_MAX_SOCKETS, 256);

	/* Read configuration */
	char filename[255];
	g_snprintf(filename, 255, "%s/%s.jcfg", config_path, JANUS_ZMQEVH_PACKAGE);
	JANUS_LOG(LOG_VERB, "Configuration file: %s\n", filename);
	janus_config *config = janus_config_parse(filename);
	if(config == NULL) {
		JANUS_LOG(LOG_WARN, "Couldn't find .jcfg configuration file (%s), trying .cfg\n", JANUS_ZMQEVH_PACKAGE);
		g_snprintf(filename, 255, "%s/%s.cfg", config_path, JANUS_ZMQEVH_PACKAGE);
		JANUS_LOG(LOG_VERB, "Configuration file: %s\n", filename);
		config = janus_config_parse(filename);
	}
	
	if(config != NULL) {
		janus_config_print(config);
		
		janus_config_category *config_general = janus_config_get_create(config, NULL, janus_config_type_category, "general");
		
		/* Parse configuration */
		janus_config_item *item = janus_config_get(config, config_general, janus_config_type_item, "enabled");
		if(item && item->value && janus_is_true(item->value)) {
			enabled = TRUE;
			
			item = janus_config_get(config, config_general, janus_config_type_item, "address");
			if(item && item->value)
				address = g_strdup(item->value);
			else
				address = g_strdup("tcp://127.0.0.1");
			
			item = janus_config_get(config, config_general, janus_config_type_item, "port");
			if(item && item->value)
				port = atoi(item->value);
			else
				port = 5546;
				
			/* Check for events mask */
			item = janus_config_get(config, config_general, janus_config_type_item, "events");
			if(item && item->value) {
				if(!strcasecmp(item->value, "none")) {
					janus_zmqevh.events_mask = JANUS_EVENT_TYPE_NONE;
				} else if(!strcasecmp(item->value, "all")) {
					janus_zmqevh.events_mask = JANUS_EVENT_TYPE_ALL;
				} else {
					/* Parse individual event types */
					janus_zmqevh.events_mask = JANUS_EVENT_TYPE_NONE;
					gchar **list = g_strsplit(item->value, ",", -1);
					gchar *index = list[0];
					if(index != NULL) {
						int i=0;
						while(index != NULL) {
							while(isspace(*index))
								index++;
							if(strlen(index)) {
								if(!strcasecmp(index, "sessions")) {
									janus_zmqevh.events_mask |= JANUS_EVENT_TYPE_SESSION;
								} else if(!strcasecmp(index, "handles")) {
									janus_zmqevh.events_mask |= JANUS_EVENT_TYPE_HANDLE;
								} else if(!strcasecmp(index, "jsep")) {
									janus_zmqevh.events_mask |= JANUS_EVENT_TYPE_JSEP;
								} else if(!strcasecmp(index, "webrtc")) {
									janus_zmqevh.events_mask |= JANUS_EVENT_TYPE_WEBRTC;
								} else if(!strcasecmp(index, "media")) {
									janus_zmqevh.events_mask |= JANUS_EVENT_TYPE_MEDIA;
								} else if(!strcasecmp(index, "plugins")) {
									janus_zmqevh.events_mask |= JANUS_EVENT_TYPE_PLUGIN;
								} else if(!strcasecmp(index, "transports")) {
									janus_zmqevh.events_mask |= JANUS_EVENT_TYPE_TRANSPORT;
								} else if(!strcasecmp(index, "core")) {
									janus_zmqevh.events_mask |= JANUS_EVENT_TYPE_CORE;
								} else {
									JANUS_LOG(LOG_WARN, "Unknown event type '%s'\n", index);
								}
							}
							i++;
							index = list[i];
						}
					}
					g_strfreev(list);
				}
			} else {
				/* Default to all events */
				janus_zmqevh.events_mask = JANUS_EVENT_TYPE_ALL;
			}
		}
		
		janus_config_destroy(config);
	}

	if(!enabled) {
		JANUS_LOG(LOG_WARN, "ZeroMQ event handler disabled\n");
		return 0;
	}

	/* Create event queue */
	events = g_async_queue_new();

	/* Setup publisher socket */
	char bind_address[256];
	g_snprintf(bind_address, sizeof(bind_address), "%s:%d", address, port);
	
	zmq_publisher = zmq_socket(zmq_context, ZMQ_PUB);
	if(zmq_publisher == NULL) {
		JANUS_LOG(LOG_FATAL, "Could not create ZeroMQ publisher socket: %s\n", zmq_strerror(errno));
		return -1;
	}
	
	/* Set socket options */
	int linger = 0;
	zmq_setsockopt(zmq_publisher, ZMQ_LINGER, &linger, sizeof(linger));
	
	/* Set high water mark to prevent memory issues */
	int hwm = 1000;
	zmq_setsockopt(zmq_publisher, ZMQ_SNDHWM, &hwm, sizeof(hwm));
	
	if(zmq_bind(zmq_publisher, bind_address) < 0) {
		JANUS_LOG(LOG_FATAL, "Could not bind ZeroMQ publisher to %s: %s\n",
			bind_address, zmq_strerror(errno));
		return -1;
	}
	
	JANUS_LOG(LOG_INFO, "ZeroMQ event handler publisher bound to %s\n", bind_address);
	
	/* Start event thread */
	GError *error = NULL;
	event_thread = g_thread_try_new("zmqevh", janus_zmqevh_thread, NULL, &error);
	if(error != NULL) {
		JANUS_LOG(LOG_FATAL, "Got error %d (%s) trying to launch the ZeroMQ event thread...\n",
			error->code, error->message ? error->message : "??");
		g_error_free(error);
		return -1;
	}

	g_atomic_int_set(&initialized, 1);
	JANUS_LOG(LOG_INFO, "%s initialized!\n", JANUS_ZMQEVH_NAME);
	
	return 0;
}

/* Event thread */
static void *janus_zmqevh_thread(void *data) {
	JANUS_LOG(LOG_VERB, "Joining ZeroMQ event handler thread...\n");
	
	while(!g_atomic_int_get(&stopping)) {
		/* Wait for event with timeout */
		janus_zmqevh_event *evt = g_async_queue_timeout_pop(events, 1000000); /* 1 second */
		if(evt == NULL)
			continue;
			
		if(evt->event == NULL) {
			g_free(evt);
			continue;
		}
		
		/* Serialize event */
		char *payload = json_dumps(evt->event, JSON_COMPACT);
		if(payload == NULL) {
			JANUS_LOG(LOG_ERR, "Failed to serialize JSON event\n");
			json_decref(evt->event);
			g_free(evt);
			continue;
		}
		
		JANUS_LOG(LOG_HUGE, "Publishing ZeroMQ event: %s\n", payload);
		
		/* Publish event with retry on EAGAIN */
		int ret = zmq_send(zmq_publisher, payload, strlen(payload), ZMQ_DONTWAIT);
		if(ret < 0) {
			if(errno == EAGAIN) {
				/* Socket buffer full - event dropped */
				JANUS_LOG(LOG_WARN, "ZeroMQ publisher buffer full, event dropped\n");
			} else {
				JANUS_LOG(LOG_ERR, "Error publishing ZeroMQ event: %s\n", zmq_strerror(errno));
			}
		}
		
		free(payload);
		json_decref(evt->event);
		g_free(evt);
	}
	
	JANUS_LOG(LOG_VERB, "Leaving ZeroMQ event handler thread...\n");
	return NULL;
}

/* Handle incoming event */
void janus_zmqevh_incoming_event(json_t *event) {
	if(!enabled || g_atomic_int_get(&stopping))
		return;
		
	if(event == NULL)
		return;
	
	/* Queue the event */
	janus_zmqevh_event *evt = g_malloc(sizeof(janus_zmqevh_event));
	evt->event = event;
	json_incref(event);
	
	g_async_queue_push(events, evt);
}

/* Handle request */
json_t *janus_zmqevh_handle_request(json_t *request) {
	if(g_atomic_int_get(&stopping)) {
		return NULL;
	}
	
	/* Return information about the event handler */
	json_t *info = json_object();
	json_object_set_new(info, "name", json_string(JANUS_ZMQEVH_NAME));
	json_object_set_new(info, "version", json_integer(JANUS_ZMQEVH_VERSION));
	json_object_set_new(info, "version_string", json_string(JANUS_ZMQEVH_VERSION_STRING));
	json_object_set_new(info, "author", json_string(JANUS_ZMQEVH_AUTHOR));
	json_object_set_new(info, "description", json_string(JANUS_ZMQEVH_DESCRIPTION));
	
	json_object_set_new(info, "enabled", enabled ? json_true() : json_false());
	
	if(enabled) {
		char bind_address[256];
		g_snprintf(bind_address, sizeof(bind_address), "%s:%d", address, port);
		json_object_set_new(info, "address", json_string(bind_address));
		json_object_set_new(info, "events_mask", json_integer(janus_zmqevh.events_mask));
	}
	
	return info;
}

/* Cleanup */
void janus_zmqevh_destroy(void) {
	if(!g_atomic_int_get(&initialized))
		return;
	g_atomic_int_set(&stopping, 1);

	/* Wait for event thread to stop */
	if(event_thread != NULL) {
		g_thread_join(event_thread);
		event_thread = NULL;
	}

	/* Clear event queue */
	if(events != NULL) {
		janus_zmqevh_event *evt = NULL;
		while((evt = g_async_queue_try_pop(events)) != NULL) {
			if(evt->event)
				json_decref(evt->event);
			g_free(evt);
		}
		g_async_queue_unref(events);
		events = NULL;
	}

	/* Close publisher socket */
	if(zmq_publisher != NULL) {
		zmq_close(zmq_publisher);
		zmq_publisher = NULL;
	}

	/* Destroy context */
	if(zmq_context != NULL) {
		zmq_ctx_destroy(zmq_context);
		zmq_context = NULL;
	}

	/* Cleanup */
	g_free(address);

	g_atomic_int_set(&initialized, 0);
	g_atomic_int_set(&stopping, 0);
	
	JANUS_LOG(LOG_INFO, "%s destroyed!\n", JANUS_ZMQEVH_NAME);
}
