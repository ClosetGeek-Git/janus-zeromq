# Build and Testing Guide

This guide provides comprehensive instructions for building, testing, and verifying the Janus ZeroMQ plugins.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Dependency Installation](#dependency-installation)
3. [Building the Plugins](#building-the-plugins)
4. [Verification](#verification)
5. [Installation](#installation)
6. [Testing](#testing)
7. [Troubleshooting](#troubleshooting)

## Prerequisites

### System Requirements

- Linux (Ubuntu 20.04+, Debian 10+, Fedora 30+, or similar)
- GCC compiler (version 7.0 or later)
- GNU Make
- pkg-config
- Git

### Required Libraries

- **ZeroMQ 4.3.4 or later** (`libzmq`)
- **GLib 2.0** (`glib-2.0`)
- **Jansson** (`jansson`)
- **Janus WebRTC Server** (headers and development files)

## Dependency Installation

### Check Dependencies

First, run the dependency checker:

```bash
./check_dependencies.sh
```

This will check for all required dependencies and provide installation instructions if any are missing.

### Ubuntu/Debian

```bash
# Update package list
sudo apt-get update

# Install build tools
sudo apt-get install build-essential pkg-config git

# Install ZeroMQ
sudo apt-get install libzmq3-dev

# Install GLib 2.0
sudo apt-get install libglib2.0-dev

# Install Jansson
sudo apt-get install libjansson-dev

# Install Janus (if building from source)
# Follow instructions at: https://github.com/meetecho/janus-gateway
```

### Fedora/RHEL/CentOS

```bash
# Install build tools
sudo dnf install gcc make pkg-config git

# Install ZeroMQ
sudo dnf install zeromq-devel

# Install GLib 2.0
sudo dnf install glib2-devel

# Install Jansson
sudo dnf install jansson-devel

# Install Janus development files
# Follow instructions at: https://github.com/meetecho/janus-gateway
```

### macOS

```bash
# Install Homebrew if not already installed
# /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install zeromq glib jansson pkg-config

# Install Janus
# Follow instructions at: https://github.com/meetecho/janus-gateway
```

### Verify ZeroMQ Version

```bash
pkg-config --modversion libzmq
```

Ensure the version is 4.3.4 or later. If you have an older version, you may need to build ZeroMQ from source:

```bash
# Download and build ZeroMQ 4.3.4
wget https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz
tar -xzf zeromq-4.3.4.tar.gz
cd zeromq-4.3.4
./configure
make
sudo make install
sudo ldconfig
```

## Building the Plugins

### Build All Plugins

```bash
# From the repository root
make
```

This will build both the transport and event handler plugins.

### Build Individual Plugins

```bash
# Build only the transport plugin
make transport

# Build only the event handler plugin
make event
```

### Clean Build Artifacts

```bash
make clean
```

### Build Output

The compiled plugins will be in:
- `build/transports/libjanus_zeromq.so` - Transport plugin
- `build/events/libjanus_zmqevh.so` - Event handler plugin

## Verification

### Check Build Success

After building, verify the plugins were created:

```bash
ls -lh build/transports/libjanus_zeromq.so
ls -lh build/events/libjanus_zmqevh.so
```

### Check Symbols

Verify the plugins export the correct symbols:

```bash
# Check transport plugin
nm -D build/transports/libjanus_zeromq.so | grep create

# Check event handler plugin
nm -D build/events/libjanus_zmqevh.so | grep create
```

You should see the `create` function exported.

### Check Dependencies

Verify the plugins link against the correct libraries:

```bash
# Check transport plugin dependencies
ldd build/transports/libjanus_zeromq.so

# Check event handler plugin dependencies
ldd build/events/libjanus_zmqevh.so
```

You should see:
- `libzmq.so`
- `libglib-2.0.so`
- `libjansson.so`

## Installation

### Standard Installation

```bash
# Copy plugins to Janus directory (adjust path as needed)
sudo cp build/transports/libjanus_zeromq.so /usr/lib/janus/transports/
sudo cp build/events/libjanus_zmqevh.so /usr/lib/janus/events/

# Copy configuration files
sudo cp conf/janus.transport.zeromq.jcfg.sample /etc/janus/janus.transport.zeromq.jcfg
sudo cp conf/janus.eventhandler.zeromqevh.jcfg.sample /etc/janus/janus.eventhandler.zeromqevh.jcfg

# Set proper permissions
sudo chmod 644 /usr/lib/janus/transports/libjanus_zeromq.so
sudo chmod 644 /usr/lib/janus/events/libjanus_zmqevh.so
sudo chmod 644 /etc/janus/janus.transport.zeromq.jcfg
sudo chmod 644 /etc/janus/janus.eventhandler.zeromqevh.jcfg
```

### Custom Installation Path

If Janus is installed in a custom location:

```bash
# Set JANUS_PREFIX to your Janus installation directory
JANUS_PREFIX=/opt/janus

# Copy plugins
sudo cp build/transports/libjanus_zeromq.so $JANUS_PREFIX/lib/janus/transports/
sudo cp build/events/libjanus_zmqevh.so $JANUS_PREFIX/lib/janus/events/

# Copy configuration files
sudo cp conf/janus.transport.zeromq.jcfg.sample $JANUS_PREFIX/etc/janus/janus.transport.zeromq.jcfg
sudo cp conf/janus.eventhandler.zeromqevh.jcfg.sample $JANUS_PREFIX/etc/janus/janus.eventhandler.zeromqevh.jcfg
```

### Configuration

Edit the configuration files:

```bash
# Edit transport configuration
sudo nano /etc/janus/janus.transport.zeromq.jcfg

# Edit event handler configuration
sudo nano /etc/janus/janus.eventhandler.zeromqevh.jcfg
```

See [DOCUMENTATION.md](DOCUMENTATION.md) for configuration options.

### Restart Janus

```bash
# Using systemd
sudo systemctl restart janus

# Or if running manually
sudo killall janus
/path/to/janus
```

## Testing

### 1. Check Janus Logs

Monitor Janus logs to ensure plugins load correctly:

```bash
# If using systemd
sudo journalctl -u janus -f

# Or check log files
tail -f /var/log/janus/janus.log
```

Look for:
```
[INFO] JANUS ZeroMQ transport plugin initialized!
[INFO] JANUS ZeroMQ event handler plugin initialized!
[INFO] ZeroMQ Janus API bound to tcp://127.0.0.1:5545
[INFO] ZeroMQ event handler publisher bound to tcp://127.0.0.1:5546
```

### 2. Test Network Ports

Verify the ports are listening:

```bash
# Check transport port
sudo netstat -tlnp | grep 5545

# Check event handler port
sudo netstat -tlnp | grep 5546
```

Or use `ss`:

```bash
ss -tlnp | grep -E '5545|5546'
```

### 3. Test Transport Plugin

#### Using Python

```bash
# Install dependencies
pip3 install pyzmq

# Run test script
python3 examples/test_transport.py
```

Expected output:
```
Testing Janus ZeroMQ Transport...
✓ Connected to tcp://127.0.0.1:5545

Test 1: Get server info
Sending: {...}
Response: {...}
✓ Server info received successfully

Test 2: Create session
Sending: {...}
Response: {...}
✓ Session created: 12345

✓ All tests passed!
```

#### Using Node.js

```bash
# Install dependencies
npm install zeromq

# Run test script
node examples/test_transport.js
```

#### Manual Test with Python

```python
import zmq
import json

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://127.0.0.1:5545")

request = {"janus": "info", "transaction": "test"}
socket.send_string(json.dumps(request))

response = socket.recv_string()
print(json.loads(response))

socket.close()
context.term()
```

### 4. Test Event Handler

#### Using Python

```bash
python3 examples/test_events.py
```

This will listen for events. Generate some activity in Janus (create sessions, etc.) to see events.

#### Manual Test with Python

```python
import zmq
import json

context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect("tcp://127.0.0.1:5546")
socket.setsockopt_string(zmq.SUBSCRIBE, "")

print("Listening for events...")
while True:
    event = socket.recv_string()
    print(json.loads(event))
```

### 5. Integration Testing

Test full request-response cycle:

```bash
# In one terminal, listen for events
python3 examples/test_events.py

# In another terminal, send requests
python3 examples/test_transport.py
```

You should see events generated from the transport requests.

## Troubleshooting

### Build Errors

#### Missing ZeroMQ

**Error:** `Package libzmq was not found`

**Solution:**
```bash
sudo apt-get install libzmq3-dev
# or build from source (see above)
```

#### Missing GLib

**Error:** `Package glib-2.0 was not found`

**Solution:**
```bash
sudo apt-get install libglib2.0-dev
```

#### Missing Jansson

**Error:** `Package jansson was not found`

**Solution:**
```bash
sudo apt-get install libjansson-dev
```

#### Missing Janus Headers

**Error:** `transport.h: No such file or directory`

**Solution:**
- Install Janus development files
- Or copy Janus headers to `/usr/include/janus/`
- Or update Makefile to include Janus header path: `-I/path/to/janus/include`

### Runtime Errors

#### Plugin Not Loading

**Error:** `Couldn't load plugin libjanus_zeromq.so`

**Solution:**
1. Check file exists: `ls -l /usr/lib/janus/transports/libjanus_zeromq.so`
2. Check permissions: `chmod 644 /usr/lib/janus/transports/libjanus_zeromq.so`
3. Check dependencies: `ldd /usr/lib/janus/transports/libjanus_zeromq.so`
4. Run `sudo ldconfig` to update library cache

#### Socket Bind Error

**Error:** `Could not bind ZeroMQ socket: Address already in use`

**Solution:**
1. Check if port is already in use: `netstat -tlnp | grep 5545`
2. Change port in configuration file
3. Kill process using the port: `sudo kill <PID>`

#### Connection Timeout

**Error:** `Timeout waiting for response`

**Solution:**
1. Verify Janus is running: `ps aux | grep janus`
2. Check plugin is loaded in Janus logs
3. Verify firewall allows connection: `sudo ufw allow 5545`
4. Test with telnet: `telnet 127.0.0.1 5545`

#### No Events Received

**Solution:**
1. Verify event handler is enabled in configuration
2. Check events_mask includes desired event types
3. Generate activity in Janus to trigger events
4. Check Janus logs for event handler initialization

### Performance Issues

#### High Memory Usage

**Solution:**
1. Adjust high water mark in event handler
2. Reduce event queue size
3. Filter events to only necessary types

#### High CPU Usage

**Solution:**
1. Reduce IO threads in ZeroMQ context
2. Increase timeout values
3. Optimize event filtering

## Advanced Testing

### Load Testing

Use the included load test script:

```bash
# Generate 100 concurrent requests
python3 examples/load_test.py --requests 100 --concurrent 10
```

### Benchmarking

Compare performance with other transports:

```bash
# Test ZeroMQ transport
time python3 examples/benchmark.py --transport zeromq

# Compare with HTTP transport
time python3 examples/benchmark.py --transport http
```

### Memory Leak Detection

Use Valgrind to check for memory leaks:

```bash
# Build with debug symbols
make clean
CFLAGS="-g -O0" make

# Run under Valgrind
valgrind --leak-check=full --show-leak-kinds=all \
    /usr/bin/janus --configs-folder=/etc/janus
```

### Security Testing

Test with different configurations:

```bash
# Test with TLS
# Test with authentication
# Test with rate limiting
# Test with invalid input
```

## Continuous Integration

### GitHub Actions

Example workflow:

```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install libzmq3-dev libglib2.0-dev libjansson-dev
      - name: Build
        run: make
      - name: Test
        run: |
          python3 examples/test_transport.py
```

## Support

For issues:
- Check [DOCUMENTATION.md](DOCUMENTATION.md)
- Review Janus logs
- Open GitHub issue with:
  - Build output
  - Error messages
  - System information
  - Configuration files

## References

- [ZeroMQ Guide](https://zguide.zeromq.org/)
- [Janus Documentation](https://janus.conf.meetecho.com/docs/)
- [GLib Documentation](https://docs.gtk.org/glib/)
- [Jansson Documentation](https://jansson.readthedocs.io/)
