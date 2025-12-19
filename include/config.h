/*! \file   config.h (Mock for testing)
 * \brief  Configuration parsing structures (mock)
 * This is a simplified mock version for build verification only.
 */

#ifndef JANUS_CONFIG_H
#define JANUS_CONFIG_H

#include <glib.h>

/* Forward declarations */
typedef struct janus_config janus_config;
typedef struct janus_config_category janus_config_category;
typedef struct janus_config_item janus_config_item;

typedef enum janus_config_type {
    janus_config_type_item,
    janus_config_type_category,
    janus_config_type_array
} janus_config_type;

struct janus_config_item {
    char *name;
    char *value;
};

struct janus_config_category {
    char *name;
};

struct janus_config {
    void *data;
};

/* Mock functions */
static inline janus_config *janus_config_parse(const char *filename) {
    (void)filename;
    return NULL;
}

static inline void janus_config_print(janus_config *config) {
    (void)config;
}

static inline janus_config_category *janus_config_get_create(
    janus_config *config, 
    janus_config_category *parent,
    janus_config_type type,
    const char *name) {
    (void)config; (void)parent; (void)type; (void)name;
    return NULL;
}

static inline janus_config_item *janus_config_get(
    janus_config *config,
    janus_config_category *category,
    janus_config_type type,
    const char *name) {
    (void)config; (void)category; (void)type; (void)name;
    return NULL;
}

static inline void janus_config_destroy(janus_config *config) {
    (void)config;
}

#endif
