# Non-Blocking I/O Implementation - Week 10

## Overview
This project has been refactored to use **non-blocking I/O** with **select()** for I/O multiplexing instead of the fork-based approach.

## Key Changes

### Server (server.c)
- **No more fork()**: Replaced multi-process architecture with single-process event-driven model
- **select() for multiplexing**: Uses `select()` to monitor multiple client connections simultaneously
- **Client state management**: Each client has a `ClientState_s` structure tracking:
  - Socket file descriptor
  - Login status
  - Username
  - Failed login attempts
  - Receive buffer for partial messages
- **Non-blocking sockets**: All sockets set to O_NONBLOCK mode
- **Event-driven architecture**: Main loop waits for events using `select()`, then handles:
  - New client connections
  - Data from existing clients
  - Client disconnections

### Client (client_nonblocking.c)
- **Non-blocking socket**: Client socket set to non-blocking mode
- **Raw terminal mode**: Uses termios to enable character-by-character input without buffering
- **select() for multiplexing**: Simultaneously monitors:
  - User input from stdin
  - Server responses from socket
- **Interactive experience**: User can see server responses immediately while typing

### Protocol Utils (protocol_utils.c)
- **Compatible with non-blocking**: Already uses per-username state tracking
- **Shared memory**: Uses shared memory for tracking logged-in clients across the single server process

## Building

```bash
make clean
make
```

This builds three executables:
- `server` - Non-blocking server using select()
- `client` - Original blocking client (for backward compatibility)
- `client_nonblocking` - New non-blocking client with select()

## Running

### Start the server:
```bash
./server <port>
```
Example:
```bash
./server 8080
```

### Start one or more clients:
```bash
./client_nonblocking <server_ip> <port>
```
Example:
```bash
./client_nonblocking 127.0.0.1 8080
```

## Benefits of Non-Blocking I/O

1. **Scalability**: Single server process can handle up to 100 concurrent clients
2. **Efficiency**: No overhead of fork() system calls and process context switching
3. **Resource usage**: Lower memory footprint (single process vs. multiple processes)
4. **Responsiveness**: Server responds to all clients without blocking on any single client
5. **Simplicity**: Easier to manage shared state (no IPC complexity)

## Technical Details

### Server Architecture
- Main loop uses `select()` to wait for I/O events
- `fd_set` tracks all active file descriptors
- When listenfd is ready: accept new connection, set to non-blocking, add to client list
- When client socket is ready: read message, process, send response
- Handles partial messages by tracking bytes received per client

### Client Architecture
- Uses `select()` to monitor both stdin and socket
- Raw terminal mode allows immediate character-by-character input
- Server responses appear instantly without interrupting user input
- Handles backspace, printable characters, and line editing

### Performance Considerations
- Maximum 100 concurrent clients (configurable via MAX_CLIENTS)
- Each client uses ~300 bytes of state memory
- No process creation overhead
- Efficient event notification via select()

## Testing Multiple Clients

Open multiple terminals and run the client in each:

Terminal 1:
```bash
./server 8080
```

Terminal 2:
```bash
./client_nonblocking 127.0.0.1 8080
# Login as: admin
```

Terminal 3:
```bash
./client_nonblocking 127.0.0.1 8080
# Login as: namnk
```

Terminal 4:
```bash
./client_nonblocking 127.0.0.1 8080
# Login as: locdinh
```

Use the `who` command to see all connected users.

## Known Limitations

1. `select()` has a maximum file descriptor limit (typically 1024)
2. For larger scale applications, consider using `epoll()` (Linux) or `kqueue()` (BSD/macOS)
3. Terminal raw mode may not work in all terminal emulators

## Compatibility

- Original blocking client (`./client`) still works with the new server
- The non-blocking server is backward compatible with the protocol
