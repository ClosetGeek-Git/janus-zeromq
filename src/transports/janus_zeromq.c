/*! \file   janus_zeromq.c
 * \author Lorenzo Miniero <lorenzo@meetecho.com>
 * \copyright GNU General Public License v3
 * \brief  Janus ZeroMQ transport plugin
 * \details  This is an implementation of a ZeroMQ transport for the
 * Janus API. This means that, with the help of this module, local and
 * remote applications can make use of ZeroMQ to make requests to the
 * Janus core. In particular, both the REQ/REP and DEALER/ROUTER patterns
 * are supported for bidirectional communication, while the PUB/SUB
 * pattern can be used for events.
 *
 * \ingroup transports
 * \ref transports
 */

#include <zmq.h>
#include <arpa/inet.h>

#include "transport.h"
#include "debug.h"
#include "apierror.h"
#include "config.h"
#include "mutex.h"
#include "utils.h"


/* Transport plugin information */
#define JANUS_ZEROMQ_VERSION			1
#define JANUS_ZEROMQ_VERSION_STRING		"0.0.1"
#define JANUS_ZEROMQ_DESCRIPTION		"This transport plugin adds ZeroMQ support to the Janus API via 0MQ."
#define JANUS_ZEROMQ_NAME				"JANUS ZeroMQ transport plugin"
#define JANUS_ZEROMQ_AUTHOR				"Meetecho s.r.l."
#define JANUS_ZEROMQ_PACKAGE			"janus.transport.zeromq"

/* Transport methods */
janus_transport *create(void);
int janus_zeromq_init(janus_transport_callbacks *callback, const char *config_path);
void janus_zeromq_destroy(void);
int janus_zeromq_get_api_compatibility(void);
int janus_zeromq_get_version(void);
const char *janus_zeromq_get_version_string(void);
const char *janus_zeromq_get_description(void);
const char *janus_zeromq_get_name(void);
const char *janus_zeromq_get_author(void);
const char *janus_zeromq_get_package(void);
gboolean janus_zeromq_is_janus_api_enabled(void);
gboolean janus_zeromq_is_admin_api_enabled(void);
int janus_zeromq_send_message(janus_transport_session *transport, void *request_id, gboolean admin, json_t *message);
void janus_zeromq_session_created(janus_transport_session *transport, guint64 session_id);
void janus_zeromq_session_over(janus_transport_session *transport, guint64 session_id, gboolean timeout, gboolean claimed);
void janus_zeromq_session_claimed(janus_transport_session *transport, guint64 session_id);
json_t *janus_zeromq_query_transport(json_t *request);

/* Transport setup */
static janus_transport janus_zeromq_transport =
	JANUS_TRANSPORT_INIT (
		.init = janus_zeromq_init,
		.destroy = janus_zeromq_destroy,

		.get_api_compatibility = janus_zeromq_get_api_compatibility,
		.get_version = janus_zeromq_get_version,
		.get_version_string = janus_zeromq_get_version_string,
		.get_description = janus_zeromq_get_description,
		.get_name = janus_zeromq_get_name,
		.get_author = janus_zeromq_get_author,
		.get_package = janus_zeromq_get_package,

		.is_janus_api_enabled = janus_zeromq_is_janus_api_enabled,
		.is_admin_api_enabled = janus_zeromq_is_admin_api_enabled,

		.send_message = janus_zeromq_send_message,
		.session_created = janus_zeromq_session_created,
		.session_over = janus_zeromq_session_over,
		.session_claimed = janus_zeromq_session_claimed,

		.query_transport = janus_zeromq_query_transport,
	);

/* Transport creator */
janus_transport *create(void) {
	JANUS_LOG(LOG_VERB, "%s created!\n", JANUS_ZEROMQ_NAME);
	return &janus_zeromq_transport;
}


/* Useful stuff */
static gint initialized = 0, stopping = 0;
static janus_transport_callbacks *gateway = NULL;
static gboolean zeromq_janus_api_enabled = FALSE;
static gboolean zeromq_admin_api_enabled = FALSE;

/* ZeroMQ context and sockets */
static void *zmq_context = NULL;
static void *zmq_socket = NULL;
static void *zmq_admin_socket = NULL;

/* Threads */
static GThread *zeromq_thread = NULL, *zeromq_admin_thread = NULL;
static void *janus_zeromq_thread(void *data);
static void *janus_zeromq_admin_thread(void *data);

/* No need for custom session structure - we use basic transport_session */
static janus_mutex sessions_mutex;
static GHashTable *sessions = NULL;

/* Configuration */
static char *address = NULL;
static uint16_t port = 0;
static char *admin_address = NULL;
static uint16_t admin_port = 0;


/* Error codes */
#define JANUS_ZEROMQ_ERROR_UNKNOWN				499
#define JANUS_ZEROMQ_ERROR_INVALID_REQUEST		498
#define JANUS_ZEROMQ_ERROR_INITIALIZATION		497


/* Plugin implementation */
int janus_zeromq_get_api_compatibility(void) {
	return JANUS_TRANSPORT_API_VERSION;
}

int janus_zeromq_get_version(void) {
	return JANUS_ZEROMQ_VERSION;
}

const char *janus_zeromq_get_version_string(void) {
	return JANUS_ZEROMQ_VERSION_STRING;
}

const char *janus_zeromq_get_description(void) {
	return JANUS_ZEROMQ_DESCRIPTION;
}

const char *janus_zeromq_get_name(void) {
	return JANUS_ZEROMQ_NAME;
}

const char *janus_zeromq_get_author(void) {
	return JANUS_ZEROMQ_AUTHOR;
}

const char *janus_zeromq_get_package(void) {
	return JANUS_ZEROMQ_PACKAGE;
}

gboolean janus_zeromq_is_janus_api_enabled(void) {
	return zeromq_janus_api_enabled;
}

gboolean janus_zeromq_is_admin_api_enabled(void) {
	return zeromq_admin_api_enabled;
}

/* Session management */
static void janus_zeromq_transport_session_free(gpointer data) {
	janus_transport_session *session = (janus_transport_session *)data;
	if(session) {
		g_free(session);
	}
}

/* Initialization */
int janus_zeromq_init(janus_transport_callbacks *callback, const char *config_path) {
	if(g_atomic_int_get(&stopping)) {
		/* Still stopping from before */
		return -1;
	}
	if(callback == NULL || config_path == NULL) {
		/* Invalid arguments */
		return -1;
	}

	/* Initialize ZeroMQ context */
	zmq_context = zmq_ctx_new();
	if(zmq_context == NULL) {
		JANUS_LOG(LOG_FATAL, "Could not initialize ZeroMQ context: %s\n", zmq_strerror(errno));
		return -1;
	}

	/* Set context options */
	zmq_ctx_set(zmq_context, ZMQ_IO_THREADS, 4);
	zmq_ctx_set(zmq_context, ZMQ_MAX_SOCKETS, 1024);

	/* Store the callbacks and initialize sessions */
	gateway = callback;
	sessions = g_hash_table_new_full(g_int64_hash, g_int64_equal, 
		(GDestroyNotify)g_free, (GDestroyNotify)janus_zeromq_transport_session_free);
	janus_mutex_init(&sessions_mutex);

	/* Read configuration */
	char filename[255];
	g_snprintf(filename, 255, "%s/%s.jcfg", config_path, JANUS_ZEROMQ_PACKAGE);
	JANUS_LOG(LOG_VERB, "Configuration file: %s\n", filename);
	janus_config *config = janus_config_parse(filename);
	if(config == NULL) {
		JANUS_LOG(LOG_WARN, "Couldn't find .jcfg configuration file (%s), trying .cfg\n", JANUS_ZEROMQ_PACKAGE);
		g_snprintf(filename, 255, "%s/%s.cfg", config_path, JANUS_ZEROMQ_PACKAGE);
		JANUS_LOG(LOG_VERB, "Configuration file: %s\n", filename);
		config = janus_config_parse(filename);
	}
	if(config != NULL) {
		janus_config_print(config);
		
		janus_config_category *config_general = janus_config_get_create(config, NULL, janus_config_type_category, "general");
		
		/* Parse configuration */
		janus_config_item *item = janus_config_get(config, config_general, janus_config_type_item, "enabled");
		if(item && item->value && janus_is_true(item->value)) {
			zeromq_janus_api_enabled = TRUE;
			
			item = janus_config_get(config, config_general, janus_config_type_item, "address");
			if(item && item->value)
				address = g_strdup(item->value);
			else
				address = g_strdup("tcp://127.0.0.1");
			
			item = janus_config_get(config, config_general, janus_config_type_item, "port");
			if(item && item->value)
				port = atoi(item->value);
			else
				port = 5545;
		}
		
		janus_config_category *config_admin = janus_config_get_create(config, NULL, janus_config_type_category, "admin");
		item = janus_config_get(config, config_admin, janus_config_type_item, "admin_enabled");
		if(item && item->value && janus_is_true(item->value)) {
			zeromq_admin_api_enabled = TRUE;
			
			item = janus_config_get(config, config_admin, janus_config_type_item, "admin_address");
			if(item && item->value)
				admin_address = g_strdup(item->value);
			else
				admin_address = g_strdup("tcp://127.0.0.1");
			
			item = janus_config_get(config, config_admin, janus_config_type_item, "admin_port");
			if(item && item->value)
				admin_port = atoi(item->value);
			else
				admin_port = 7445;
		}
		
		janus_config_destroy(config);
	}

	/* Setup Janus API socket */
	if(zeromq_janus_api_enabled) {
		char bind_address[256];
		g_snprintf(bind_address, sizeof(bind_address), "%s:%d", address, port);
		
		zmq_socket = zmq_socket(zmq_context, ZMQ_REP);
		if(zmq_socket == NULL) {
			JANUS_LOG(LOG_FATAL, "Could not create ZeroMQ socket: %s\n", zmq_strerror(errno));
			return -1;
		}
		
		/* Set socket options */
		int linger = 0;
		zmq_setsockopt(zmq_socket, ZMQ_LINGER, &linger, sizeof(linger));
		
		/* Set receive timeout */
		int timeout = 1000; /* 1 second */
		zmq_setsockopt(zmq_socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
		
		if(zmq_bind(zmq_socket, bind_address) < 0) {
			JANUS_LOG(LOG_FATAL, "Could not bind ZeroMQ socket to %s: %s\n", 
				bind_address, zmq_strerror(errno));
			return -1;
		}
		
		JANUS_LOG(LOG_INFO, "ZeroMQ Janus API bound to %s\n", bind_address);
		
		/* Start thread */
		GError *error = NULL;
		zeromq_thread = g_thread_try_new("zeromq", janus_zeromq_thread, NULL, &error);
		if(error != NULL) {
			JANUS_LOG(LOG_FATAL, "Got error %d (%s) trying to launch the ZeroMQ thread...\n",
				error->code, error->message ? error->message : "??");
			g_error_free(error);
			return -1;
		}
	}

	/* Setup Admin API socket */
	if(zeromq_admin_api_enabled) {
		char bind_address[256];
		g_snprintf(bind_address, sizeof(bind_address), "%s:%d", admin_address, admin_port);
		
		zmq_admin_socket = zmq_socket(zmq_context, ZMQ_REP);
		if(zmq_admin_socket == NULL) {
			JANUS_LOG(LOG_FATAL, "Could not create ZeroMQ admin socket: %s\n", zmq_strerror(errno));
			return -1;
		}
		
		/* Set socket options */
		int linger = 0;
		zmq_setsockopt(zmq_admin_socket, ZMQ_LINGER, &linger, sizeof(linger));
		
		/* Set receive timeout */
		int timeout = 1000; /* 1 second */
		zmq_setsockopt(zmq_admin_socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
		
		if(zmq_bind(zmq_admin_socket, bind_address) < 0) {
			JANUS_LOG(LOG_FATAL, "Could not bind ZeroMQ admin socket to %s: %s\n",
				bind_address, zmq_strerror(errno));
			return -1;
		}
		
		JANUS_LOG(LOG_INFO, "ZeroMQ Admin API bound to %s\n", bind_address);
		
		/* Start thread */
		GError *error = NULL;
		zeromq_admin_thread = g_thread_try_new("zeromq_admin", janus_zeromq_admin_thread, NULL, &error);
		if(error != NULL) {
			JANUS_LOG(LOG_FATAL, "Got error %d (%s) trying to launch the ZeroMQ admin thread...\n",
				error->code, error->message ? error->message : "??");
			g_error_free(error);
			return -1;
		}
	}

	g_atomic_int_set(&initialized, 1);
	JANUS_LOG(LOG_INFO, "%s initialized!\n", JANUS_ZEROMQ_NAME);
	
	return 0;
}

/* Thread for handling Janus API messages */
static void *janus_zeromq_thread(void *data) {
	JANUS_LOG(LOG_VERB, "Joining ZeroMQ thread...\n");
	
	zmq_msg_t message;
	
	while(!g_atomic_int_get(&stopping)) {
		/* Initialize message */
		zmq_msg_init(&message);
		
		/* Receive message (timeout is already set) */
		int size = zmq_msg_recv(&message, zmq_socket, 0);
		if(size < 0) {
			if(errno == EAGAIN || errno == EINTR) {
				/* Timeout or interrupt, continue */
				zmq_msg_close(&message);
				continue;
			}
			JANUS_LOG(LOG_ERR, "Error receiving ZeroMQ message: %s\n", zmq_strerror(errno));
			zmq_msg_close(&message);
			continue;
		}
		
		/* Process message */
		char *payload = g_malloc0(size + 1);
		memcpy(payload, zmq_msg_data(&message), size);
		payload[size] = '\0';
		
		JANUS_LOG(LOG_HUGE, "Received ZeroMQ message: %s\n", payload);
		
		/* Parse JSON */
		json_error_t error;
		json_t *root = json_loads(payload, 0, &error);
		g_free(payload);
		zmq_msg_close(&message);
		
		if(!root) {
			JANUS_LOG(LOG_ERR, "JSON parsing error: %s\n", error.text);
			/* Send error response */
			char *error_response = "{\"janus\":\"error\",\"error\":{\"code\":498,\"reason\":\"Invalid JSON\"}}";
			zmq_send(zmq_socket, error_response, strlen(error_response), 0);
			continue;
		}
		
		/* Create a transport session - Note: ownership transfers to gateway */
		janus_transport_session *transport_session = g_malloc0(sizeof(janus_transport_session));
		transport_session->transport_p = &janus_zeromq_transport;
		
		/* Pass to gateway - gateway takes ownership of both root and transport_session */
		gateway->incoming_request(&janus_zeromq_transport, transport_session, NULL, FALSE, root, NULL);
		/* Note: Do not free transport_session or root here - gateway is responsible */
	}
	
	JANUS_LOG(LOG_VERB, "Leaving ZeroMQ thread...\n");
	return NULL;
}

/* Thread for handling Admin API messages */
static void *janus_zeromq_admin_thread(void *data) {
	JANUS_LOG(LOG_VERB, "Joining ZeroMQ Admin thread...\n");
	
	zmq_msg_t message;
	
	while(!g_atomic_int_get(&stopping)) {
		/* Initialize message */
		zmq_msg_init(&message);
		
		/* Receive message (timeout is already set) */
		int size = zmq_msg_recv(&message, zmq_admin_socket, 0);
		if(size < 0) {
			if(errno == EAGAIN || errno == EINTR) {
				/* Timeout or interrupt, continue */
				zmq_msg_close(&message);
				continue;
			}
			JANUS_LOG(LOG_ERR, "Error receiving ZeroMQ admin message: %s\n", zmq_strerror(errno));
			zmq_msg_close(&message);
			continue;
		}
		
		/* Process message */
		char *payload = g_malloc0(size + 1);
		memcpy(payload, zmq_msg_data(&message), size);
		payload[size] = '\0';
		
		JANUS_LOG(LOG_HUGE, "Received ZeroMQ admin message: %s\n", payload);
		
		/* Parse JSON */
		json_error_t error;
		json_t *root = json_loads(payload, 0, &error);
		g_free(payload);
		zmq_msg_close(&message);
		
		if(!root) {
			JANUS_LOG(LOG_ERR, "JSON parsing error: %s\n", error.text);
			/* Send error response */
			char *error_response = "{\"janus\":\"error\",\"error\":{\"code\":498,\"reason\":\"Invalid JSON\"}}";
			zmq_send(zmq_admin_socket, error_response, strlen(error_response), 0);
			continue;
		}
		
		/* Create a transport session - Note: ownership transfers to gateway */
		janus_transport_session *transport_session = g_malloc0(sizeof(janus_transport_session));
		transport_session->transport_p = &janus_zeromq_transport;
		
		/* Pass to gateway - gateway takes ownership of both root and transport_session */
		gateway->incoming_request(&janus_zeromq_transport, transport_session, NULL, TRUE, root, NULL);
		/* Note: Do not free transport_session or root here - gateway is responsible */
	}
	
	JANUS_LOG(LOG_VERB, "Leaving ZeroMQ Admin thread...\n");
	return NULL;
}

/* Send message */
int janus_zeromq_send_message(janus_transport_session *transport, void *request_id, gboolean admin, json_t *message) {
	if(message == NULL)
		return -1;
	if(g_atomic_int_get(&stopping))
		return -1;
		
	/* Serialize message */
	char *payload = json_dumps(message, JSON_COMPACT);
	if(payload == NULL) {
		JANUS_LOG(LOG_ERR, "Failed to serialize JSON message\n");
		json_decref(message);
		return -1;
	}
	
	JANUS_LOG(LOG_HUGE, "Sending ZeroMQ message: %s\n", payload);
	
	/* Send to appropriate socket */
	void *socket = admin ? zmq_admin_socket : zmq_socket;
	int ret = zmq_send(socket, payload, strlen(payload), 0);
	
	free(payload);
	json_decref(message);
	
	if(ret < 0) {
		JANUS_LOG(LOG_ERR, "Error sending ZeroMQ message: %s\n", zmq_strerror(errno));
		return -1;
	}
	
	return 0;
}

/* Session callbacks */
void janus_zeromq_session_created(janus_transport_session *transport, guint64 session_id) {
	/* We don't need to do anything here */
}

void janus_zeromq_session_over(janus_transport_session *transport, guint64 session_id, gboolean timeout, gboolean claimed) {
	/* We don't need to do anything here */
}

void janus_zeromq_session_claimed(janus_transport_session *transport, guint64 session_id) {
	/* We don't need to do anything here */
}

/* Query transport */
json_t *janus_zeromq_query_transport(json_t *request) {
	if(g_atomic_int_get(&stopping)) {
		return NULL;
	}
	
	/* Return information about the transport */
	json_t *info = json_object();
	json_object_set_new(info, "name", json_string(JANUS_ZEROMQ_NAME));
	json_object_set_new(info, "version", json_integer(JANUS_ZEROMQ_VERSION));
	json_object_set_new(info, "version_string", json_string(JANUS_ZEROMQ_VERSION_STRING));
	json_object_set_new(info, "author", json_string(JANUS_ZEROMQ_AUTHOR));
	json_object_set_new(info, "description", json_string(JANUS_ZEROMQ_DESCRIPTION));
	
	if(zeromq_janus_api_enabled) {
		json_object_set_new(info, "janus_api_enabled", json_true());
		char bind_address[256];
		g_snprintf(bind_address, sizeof(bind_address), "%s:%d", address, port);
		json_object_set_new(info, "janus_api_address", json_string(bind_address));
	} else {
		json_object_set_new(info, "janus_api_enabled", json_false());
	}
	
	if(zeromq_admin_api_enabled) {
		json_object_set_new(info, "admin_api_enabled", json_true());
		char bind_address[256];
		g_snprintf(bind_address, sizeof(bind_address), "%s:%d", admin_address, admin_port);
		json_object_set_new(info, "admin_api_address", json_string(bind_address));
	} else {
		json_object_set_new(info, "admin_api_enabled", json_false());
	}
	
	return info;
}

/* Cleanup */
void janus_zeromq_destroy(void) {
	if(!g_atomic_int_get(&initialized))
		return;
	g_atomic_int_set(&stopping, 1);

	/* Wait for threads to stop */
	if(zeromq_thread != NULL) {
		g_thread_join(zeromq_thread);
		zeromq_thread = NULL;
	}
	if(zeromq_admin_thread != NULL) {
		g_thread_join(zeromq_admin_thread);
		zeromq_admin_thread = NULL;
	}

	/* Close sockets */
	if(zmq_socket != NULL) {
		zmq_close(zmq_socket);
		zmq_socket = NULL;
	}
	if(zmq_admin_socket != NULL) {
		zmq_close(zmq_admin_socket);
		zmq_admin_socket = NULL;
	}

	/* Destroy context */
	if(zmq_context != NULL) {
		zmq_ctx_destroy(zmq_context);
		zmq_context = NULL;
	}

	/* Cleanup */
	janus_mutex_lock(&sessions_mutex);
	g_hash_table_destroy(sessions);
	sessions = NULL;
	janus_mutex_unlock(&sessions_mutex);

	g_free(address);
	g_free(admin_address);

	g_atomic_int_set(&initialized, 0);
	g_atomic_int_set(&stopping, 0);
	
	JANUS_LOG(LOG_INFO, "%s destroyed!\n", JANUS_ZEROMQ_NAME);
}
