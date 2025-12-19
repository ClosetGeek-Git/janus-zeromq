# Janus ZeroMQ Plugins

This repository contains ZeroMQ-based transport and event handler plugins for the [Janus WebRTC Server](https://github.com/meetecho/janus-gateway).

## Overview

This implementation provides:

1. **ZeroMQ Transport Plugin** (`janus_zeromq.c`) - Enables communication with Janus via ZeroMQ sockets
2. **ZeroMQ Event Handler** (`janus_zmqevh.c`) - Publishes Janus events via ZeroMQ PUB/SUB pattern

## Features

### ZeroMQ Transport Plugin

- **REQ/REP Pattern**: Bidirectional request-response communication with Janus API
- **Separate Admin API**: Optional dedicated socket for administrative operations
- **Thread-safe**: Handles concurrent requests safely
- **Configurable**: Flexible address and port configuration
- **Error Handling**: Comprehensive error logging for ZeroMQ operations

### ZeroMQ Event Handler

- **PUB/SUB Pattern**: Publish Janus events to multiple subscribers
- **Event Filtering**: Subscribe to specific event types or all events
- **Asynchronous Processing**: Non-blocking event publishing
- **High Water Mark**: Prevents memory issues with event queue management
- **Configurable**: Flexible event subscription and address configuration

## Dependencies

- **ZeroMQ 4.3.4 or later** - The messaging library
- **GLib 2.0** - For basic data structures and threading
- **Jansson** - JSON parsing and generation library
- **Janus WebRTC Server** - Required headers and APIs

### Installing Dependencies

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libzmq3-dev libglib2.0-dev libjansson-dev
```

#### Fedora/RHEL
```bash
sudo dnf install zeromq-devel glib2-devel jansson-devel
```

#### macOS
```bash
brew install zeromq glib jansson
```

## Building

```bash
# Build both plugins
make

# Build only transport plugin
make transport

# Build only event handler plugin
make event

# Clean build artifacts
make clean

# Show help
make help
```

The compiled plugins will be in:
- `build/transports/libjanus_zeromq.so` - Transport plugin
- `build/events/libjanus_zmqevh.so` - Event handler plugin

## Installation

1. Copy the plugin files to your Janus plugins directory:
```bash
sudo cp build/transports/libjanus_zeromq.so /usr/lib/janus/transports/
sudo cp build/events/libjanus_zmqevh.so /usr/lib/janus/events/
```

2. Copy the configuration files:
```bash
sudo cp conf/janus.transport.zeromq.jcfg.sample /etc/janus/janus.transport.zeromq.jcfg
sudo cp conf/janus.eventhandler.zeromqevh.jcfg.sample /etc/janus/janus.eventhandler.zeromqevh.jcfg
```

3. Edit the configuration files as needed and restart Janus.

## Configuration

### Transport Plugin Configuration

Edit `/etc/janus/janus.transport.zeromq.jcfg`:

```
general: {
    enabled = true
    address = "tcp://127.0.0.1"
    port = 5545
}

admin: {
    admin_enabled = true
    admin_address = "tcp://127.0.0.1"
    admin_port = 7445
}
```

### Event Handler Configuration

Edit `/etc/janus/janus.eventhandler.zeromqevh.jcfg`:

```
general: {
    enabled = true
    address = "tcp://127.0.0.1"
    port = 5546
    events = "all"  # or: "sessions,handles,jsep,webrtc,media,plugins,transports,core"
}
```

## Usage

### Sending Requests (Python Example)

```python
import zmq
import json

# Create ZeroMQ context and socket
context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://127.0.0.1:5545")

# Send a request
request = {
    "janus": "info",
    "transaction": "test-transaction"
}
socket.send_string(json.dumps(request))

# Receive response
response = socket.recv_string()
print(json.loads(response))

socket.close()
context.term()
```

### Receiving Events (Python Example)

```python
import zmq
import json

# Create ZeroMQ context and socket
context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect("tcp://127.0.0.1:5546")

# Subscribe to all events
socket.setsockopt_string(zmq.SUB_FILTER, "")

# Receive events
while True:
    event = socket.recv_string()
    print("Event:", json.loads(event))

socket.close()
context.term()
```

### Node.js Example

```javascript
const zmq = require('zeromq');

// Create REQ socket for requests
async function sendRequest() {
    const sock = new zmq.Request();
    sock.connect("tcp://127.0.0.1:5545");
    
    const request = {
        janus: "info",
        transaction: "test-transaction"
    };
    
    await sock.send(JSON.stringify(request));
    const [response] = await sock.receive();
    console.log(JSON.parse(response.toString()));
    
    sock.close();
}

// Create SUB socket for events
async function receiveEvents() {
    const sock = new zmq.Subscriber();
    sock.connect("tcp://127.0.0.1:5546");
    sock.subscribe("");
    
    for await (const [msg] of sock) {
        console.log("Event:", JSON.parse(msg.toString()));
    }
}
```

## Architecture

### Transport Plugin

The transport plugin follows the Janus transport plugin API:

1. **Initialization**: Creates ZeroMQ context and sockets
2. **Socket Binding**: Binds REP sockets for Janus API and Admin API
3. **Message Reception**: Receives JSON messages from ZeroMQ clients
4. **Request Processing**: Passes requests to Janus core via callbacks
5. **Response Sending**: Sends JSON responses back to clients
6. **Cleanup**: Properly closes sockets and destroys context

### Event Handler

The event handler follows the Janus event handler API:

1. **Initialization**: Creates ZeroMQ context and PUB socket
2. **Socket Binding**: Binds PUB socket for event publishing
3. **Event Queueing**: Queues incoming events from Janus core
4. **Event Publishing**: Publishes events to all subscribers
5. **Event Filtering**: Only processes events matching the configured mask
6. **Cleanup**: Properly closes socket and destroys context

## ZeroMQ Patterns Used

- **REQ/REP**: Used for request-response communication in the transport plugin
- **PUB/SUB**: Used for event broadcasting in the event handler

## Migration from Nanomsg

This implementation replaces Nanomsg with ZeroMQ, providing:

- **Better Performance**: ZeroMQ is generally faster and more scalable
- **Active Development**: ZeroMQ has a larger community and more frequent updates
- **More Patterns**: ZeroMQ supports additional messaging patterns
- **Better Documentation**: Comprehensive documentation and examples

### Key API Changes

| Nanomsg | ZeroMQ |
|---------|--------|
| `nn_socket()` | `zmq_socket()` |
| `nn_bind()` | `zmq_bind()` |
| `nn_connect()` | `zmq_connect()` |
| `nn_send()` | `zmq_send()` |
| `nn_recv()` | `zmq_recv()` / `zmq_msg_recv()` |
| `nn_close()` | `zmq_close()` |
| `NN_REP` | `ZMQ_REP` |
| `NN_PUB` | `ZMQ_PUB` |

## Testing

### Manual Testing

1. Start Janus with the plugins enabled
2. Use the Python or Node.js examples above to test connectivity
3. Monitor Janus logs for errors

### Integration Testing

```bash
# Test transport plugin
python3 examples/test_transport.py

# Test event handler
python3 examples/test_events.py
```

## Troubleshooting

### Plugin Not Loading

- Check that ZeroMQ library is installed: `ldconfig -p | grep zmq`
- Verify plugin file permissions: `ls -l /usr/lib/janus/transports/`
- Check Janus logs: `journalctl -u janus -f`

### Connection Issues

- Verify firewall settings allow the configured ports
- Check that the address format is correct: `tcp://host:port`
- Test with `telnet` or `nc` to verify port availability

### Event Not Received

- Ensure event handler is enabled in configuration
- Verify event mask includes desired event types
- Check that subscriber is using correct filter (empty string for all)

## Security Considerations

- **Bind to localhost**: By default, bind to `127.0.0.1` for security
- **Firewall**: Use firewall rules to restrict access
- **Authentication**: Consider implementing ZeroMQ CURVE for encryption/auth
- **Input Validation**: All JSON input is validated before processing

## Performance Tuning

### Context Options

```c
zmq_ctx_set(zmq_context, ZMQ_IO_THREADS, 4);  // Adjust based on load
zmq_ctx_set(zmq_context, ZMQ_MAX_SOCKETS, 1024);  // Adjust based on needs
```

### Socket Options

```c
// High water mark for event publishing
int hwm = 1000;
zmq_setsockopt(zmq_publisher, ZMQ_SNDHWM, &hwm, sizeof(hwm));

// Linger time on close
int linger = 0;
zmq_setsockopt(zmq_socket, ZMQ_LINGER, &linger, sizeof(linger));
```

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This project follows the same license as Janus WebRTC Server (GNU General Public License v3).

## Credits

- Based on the Janus WebRTC Server plugin architecture
- Uses the ZeroMQ messaging library
- Adapted from the original Nanomsg implementations

## Support

For issues and questions:

- GitHub Issues: https://github.com/ClosetGeek-Git/janus-zeromq/issues
- Janus Mailing List: https://groups.google.com/forum/#!forum/meetecho-janus

## References

- [Janus WebRTC Server](https://github.com/meetecho/janus-gateway)
- [ZeroMQ Documentation](https://zeromq.org/documentation/)
- [ZeroMQ API Reference](http://api.zeromq.org/)
- [Janus Plugin Development](https://janus.conf.meetecho.com/docs/plugin_8h.html)
