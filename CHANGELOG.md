# Changelog

All notable changes to this project will be documented in this file.

## [0.0.1] - 2025-12-19

### Added
- Initial implementation of ZeroMQ transport plugin (`janus_zeromq.c`)
- Initial implementation of ZeroMQ event handler plugin (`janus_zmqevh.c`)
- ZeroMQ 4.3.4+ API support with context and socket management
- REQ/REP pattern for bidirectional Janus API communication
- Separate Admin API socket support in transport plugin
- PUB/SUB pattern for event broadcasting in event handler
- Thread-safe request and event handling
- Comprehensive error logging for ZeroMQ operations
- Configuration files for both plugins
- Makefile for building plugins
- Complete documentation in DOCUMENTATION.md
- Example scripts for Python and Node.js
- .gitignore for common build artifacts and temporary files

### Changed
- Migrated from Nanomsg to ZeroMQ API
- Replaced `nn_socket()` with `zmq_socket()`
- Replaced `nn_bind()` with `zmq_bind()`
- Replaced `nn_send()/nn_recv()` with `zmq_send()/zmq_msg_recv()`
- Updated error handling to use `zmq_strerror(errno)`
- Changed socket patterns:
  - `NN_REP` → `ZMQ_REP` for transport
  - `NN_PUB` → `ZMQ_PUB` for events
- Improved context initialization with configurable options
- Enhanced socket options (linger, high water mark, timeouts)

### Technical Details

#### Transport Plugin (janus_zeromq.c)
- **ZeroMQ Context**: Initialized with `zmq_ctx_new()` and configured with IO threads and max sockets
- **Socket Creation**: Uses `ZMQ_REP` pattern for request-response
- **Binding**: Binds to configurable TCP addresses (default: 127.0.0.1:5545 for API, 127.0.0.1:7445 for Admin)
- **Message Reception**: Thread-based with timeout support using `zmq_msg_recv()`
- **JSON Processing**: Parse incoming messages and pass to Janus gateway
- **Response Sending**: Serialize and send JSON responses via `zmq_send()`
- **Lifecycle Management**: Proper initialization, thread management, and cleanup

#### Event Handler (janus_zmqevh.c)
- **ZeroMQ Context**: Initialized with optimized settings for event publishing
- **Socket Creation**: Uses `ZMQ_PUB` pattern for event broadcasting
- **Binding**: Binds to configurable TCP address (default: 127.0.0.1:5546)
- **Event Queue**: Asynchronous queue using GAsyncQueue for non-blocking event handling
- **Event Filtering**: Configurable event mask for selective event subscription
- **High Water Mark**: Set to 1000 to prevent memory issues
- **Thread Processing**: Dedicated thread for event serialization and publishing

#### Dependencies
- ZeroMQ 4.3.4 or later (`libzmq`)
- GLib 2.0 for threading and data structures
- Jansson for JSON parsing
- Janus WebRTC Server headers

#### Build System
- Makefile with targets: all, transport, event, clean, install, help
- pkg-config integration for dependency detection
- Builds shared libraries (.so) for both plugins

#### Configuration
- Sample configuration files with sensible defaults
- Support for both .jcfg and .cfg formats
- Flexible address and port configuration
- Event filtering configuration for event handler

### Security Considerations
- Default binding to localhost (127.0.0.1) for security
- Linger time set to 0 to prevent hanging sockets
- Timeout support for all receive operations
- Input validation for all JSON messages

### Performance Optimizations
- Configurable IO threads (default: 4 for transport, 2 for events)
- High water mark prevents unbounded memory growth
- Non-blocking event publishing with ZMQ_DONTWAIT
- Efficient thread-based message handling

### Documentation
- Comprehensive README.md with quick start guide
- Detailed DOCUMENTATION.md covering:
  - Installation instructions
  - Configuration guide
  - Usage examples (Python, Node.js)
  - Architecture overview
  - Migration guide from Nanomsg
  - Troubleshooting guide
  - Performance tuning tips
- Example test scripts for both plugins
- Inline code comments for maintainability

## Migration Notes

### From Nanomsg to ZeroMQ

This release completely replaces Nanomsg with ZeroMQ. Key changes:

1. **API Changes**: All Nanomsg API calls replaced with ZeroMQ equivalents
2. **Context Management**: Explicit context creation and configuration
3. **Socket Options**: Different option names and values
4. **Message API**: Using ZeroMQ message API for better control
5. **Error Handling**: Using `errno` and `zmq_strerror()` for errors

### Compatibility

- Maintains Janus plugin API compatibility
- Same configuration file structure
- Same functionality as original Nanomsg plugins
- Drop-in replacement for existing installations

### Testing

Manual testing recommended:
1. Build both plugins
2. Install to Janus plugins directory
3. Configure and enable plugins
4. Test with example scripts
5. Monitor Janus logs for errors

## Future Enhancements

Potential future improvements:
- CURVE security for encryption and authentication
- Additional socket patterns (DEALER/ROUTER, PUSH/PULL)
- Performance benchmarking tools
- Integration tests
- Docker container for testing
- CI/CD pipeline

## References

- [ZeroMQ Documentation](https://zeromq.org/documentation/)
- [Janus WebRTC Server](https://github.com/meetecho/janus-gateway)
- [ZeroMQ API Reference](http://api.zeromq.org/)
