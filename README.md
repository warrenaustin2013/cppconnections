# cppconnections (`1.0.0`)
A minimal self-contained signal-slot style callback system for C++.

# features

1. Signal-Slot Mechanism
    + Provides a lightweight signal-slot system similar to those found in GUI frameworks (like Qt), allowing multiple callbacks (slots) to be connected to a signal and invoked in order when the signal is fired.

2. No Dependencies / Standard Library-Free
    + The library is self-contained and does not rely on the C++ standard library or any other libraries, making it suitable for freestanding environments or embedded systems.

3. User Context Pointer Support
    + Each connection stores a user-defined void* context, which is passed into the callback, allowing state or metadata to be carried through the event system.

4. Fixed Maximum Connections
    + Uses a compile-time constant (`CPP_CONNECTIONS_MAX_CONNECTIONS`) to limit the number of active connections per signal, preventing dynamic memory allocation and ensuring deterministic behavior.

5. Manual Connection Management
    + Connections are represented as plain structs and can be explicitly connected or disconnected via functions or directly through method calls, giving full control over connection lifetimes and behavior.

# limitations

1. No dynamic allocation or resizing (connections are stored in a fixed-size array).

2. No automatic disconnection or scoped connection support.

3. No threading support (callbacks should be managed from a single-threaded context unless extended).

# usage
```
#include "cppconnections1.hpp"
#include <iostream>

void callback_event(void* context, int value) {
    const char* name = static_cast<const char*>(context);
    std::cout << name << " received value: " << value << "\n";
}

int main() {
    connections::signal<int> sig;

    // register a callback
    connections::connection<int>* conn = sig.connect(callback_event, (void*)"Listener");

    // fire the signal
    sig.fire(42);

    // disconnect the callback
    if (conn) conn->disconnect();

    // should not fire because the callback was disconnected
    sig.fire(99);
}
```

# customization
Define `CPP_CONNECTIONS_MAX_CONNECTIONS` before including the header to customize the maximum number of simultaneous connections per signal.

```
#define CPP_CONNECTIONS_MAX_CONNECTIONS 64
#include "cppconnections1.hpp"
```

# license
MIT License (see top of header for full terms)
