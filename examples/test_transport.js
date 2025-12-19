#!/usr/bin/env node
/**
 * Test script for Janus ZeroMQ Transport Plugin (Node.js)
 * 
 * This script demonstrates how to communicate with Janus via ZeroMQ transport.
 * 
 * Installation: npm install zeromq
 */

const zmq = require('zeromq');

async function testTransport() {
    console.log('Testing Janus ZeroMQ Transport...');
    
    // Create REQ socket
    const socket = new zmq.Request();
    
    try {
        // Connect to Janus
        socket.connect('tcp://127.0.0.1:5545');
        console.log('✓ Connected to tcp://127.0.0.1:5545');
        
        // Set timeout
        socket.receiveTimeout = 5000;
        
        // Test 1: Get server info
        console.log('\nTest 1: Get server info');
        const infoRequest = {
            janus: 'info',
            transaction: 'test-info'
        };
        console.log('Sending:', JSON.stringify(infoRequest, null, 2));
        await socket.send(JSON.stringify(infoRequest));
        
        const [infoResponse] = await socket.receive();
        const infoData = JSON.parse(infoResponse.toString());
        console.log('Response:', JSON.stringify(infoData, null, 2));
        
        if (infoData.janus === 'server_info') {
            console.log('✓ Server info received successfully');
        } else {
            console.log('✗ Unexpected response');
            return false;
        }
        
        // Test 2: Create session
        console.log('\nTest 2: Create session');
        const createRequest = {
            janus: 'create',
            transaction: 'test-create'
        };
        console.log('Sending:', JSON.stringify(createRequest, null, 2));
        await socket.send(JSON.stringify(createRequest));
        
        const [createResponse] = await socket.receive();
        const createData = JSON.parse(createResponse.toString());
        console.log('Response:', JSON.stringify(createData, null, 2));
        
        if (createData.janus === 'success') {
            const sessionId = createData.data.id;
            console.log(`✓ Session created: ${sessionId}`);
        } else {
            console.log('✗ Failed to create session');
            return false;
        }
        
        console.log('\n✓ All tests passed!');
        return true;
        
    } catch (error) {
        console.error('✗ Error:', error.message);
        return false;
    } finally {
        socket.close();
    }
}

// Run test
testTransport().then(success => {
    process.exit(success ? 0 : 1);
}).catch(error => {
    console.error('Fatal error:', error);
    process.exit(1);
});
