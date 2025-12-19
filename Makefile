# Makefile for Janus ZeroMQ plugins
#
# This Makefile builds the ZeroMQ transport and event handler plugins for Janus

CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC $(shell pkg-config --cflags glib-2.0 jansson libzmq)
LDFLAGS = -shared $(shell pkg-config --libs glib-2.0 jansson libzmq)

# Output directories
BUILD_DIR = build
TRANSPORT_DIR = $(BUILD_DIR)/transports
EVENT_DIR = $(BUILD_DIR)/events

# Source files
TRANSPORT_SRC = src/transports/janus_zeromq.c
EVENT_SRC = src/events/janus_zmqevh.c

# Output files
TRANSPORT_OUT = $(TRANSPORT_DIR)/libjanus_zeromq.so
EVENT_OUT = $(EVENT_DIR)/libjanus_zmqevh.so

.PHONY: all clean install transport event dirs

all: dirs transport event

dirs:
	mkdir -p $(TRANSPORT_DIR)
	mkdir -p $(EVENT_DIR)

transport: $(TRANSPORT_OUT)

event: $(EVENT_OUT)

$(TRANSPORT_OUT): $(TRANSPORT_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(EVENT_OUT): $(EVENT_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

install: all
	@echo "Installing plugins..."
	@echo "Note: You need to manually copy the .so files to your Janus plugins directory"
	@echo "Transport plugin: $(TRANSPORT_OUT)"
	@echo "Event handler plugin: $(EVENT_OUT)"

help:
	@echo "Janus ZeroMQ Plugins Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build both transport and event handler plugins (default)"
	@echo "  transport - Build only the ZeroMQ transport plugin"
	@echo "  event     - Build only the ZeroMQ event handler plugin"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Show installation instructions"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - ZeroMQ 4.3.4 or later (libzmq)"
	@echo "  - GLib 2.0"
	@echo "  - Jansson (JSON library)"
	@echo "  - Janus WebRTC Server headers"
