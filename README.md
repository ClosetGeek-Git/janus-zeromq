"# Janus ZeroMQ Transport and Event Handler Plugins

This repository provides ZeroMQ-based transport and event handler plugins for the Janus WebRTC Server, replacing the legacy Nanomsg implementations with modern ZeroMQ 4.3.4+ API.

## Quick Start

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libzmq3-dev libglib2.0-dev libjansson-dev

# Build plugins
make

# Install (adjust paths as needed)
sudo cp build/transports/libjanus_zeromq.so /usr/lib/janus/transports/
sudo cp build/events/libjanus_zmqevh.so /usr/lib/janus/events/
sudo cp conf/*.jcfg.sample /etc/janus/
```

## Features

- **ZeroMQ Transport Plugin**: REQ/REP pattern for Janus API communication
- **ZeroMQ Event Handler**: PUB/SUB pattern for event broadcasting
- **Full ZeroMQ 4.3.4+ Support**: Modern API with improved performance
- **Thread-safe**: Proper concurrency handling
- **Configurable**: Flexible configuration options
- **Well-documented**: Comprehensive documentation and examples

## Documentation

See [DOCUMENTATION.md](DOCUMENTATION.md) for complete documentation including:
- Installation instructions
- Configuration guide
- Usage examples (Python, Node.js)
- Architecture overview
- Troubleshooting guide
- Performance tuning

## Components

### Transport Plugin (`src/transports/janus_zeromq.c`)
- ZeroMQ REP socket for Janus API
- Optional separate Admin API socket
- JSON message processing
- Thread-based request handling

### Event Handler (`src/events/janus_zmqevh.c`)
- ZeroMQ PUB socket for event publishing
- Configurable event filtering
- Asynchronous event processing
- Queue-based event handling

## Requirements

- ZeroMQ 4.3.4 or later
- GLib 2.0
- Jansson JSON library
- Janus WebRTC Server

## License

GNU General Public License v3 (same as Janus WebRTC Server)

## Author

Based on Janus plugin architecture by Meetecho s.r.l." 
