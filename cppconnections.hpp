/**
 * @file cppconnections.hpp
 * @version 1.1.0
 * @brief A minimal signal-slot style callback system for C++.
 * @note This library is NOT thread safe and should not be used in a threaded setting!
 *
 * This header defines a lightweight, standalone signal/connection mechanism
 * that allows callbacks to be registered and invoked without requiring the
 * C++ standard library or external dependencies.
 *
 * @copyright MIT License
 *
 * @details Copyright (c) 2025 warrenaustin2013
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef CPP_CONNECTIONS_HEADER_GUARD
#define CPP_CONNECTIONS_HEADER_GUARD

#ifndef CPP_CONNECTIONS_MAX_CONNECTIONS
 /**
  * @brief Defines the maximum number of simultaneous connections a single signal can hold.
  * @since 1.0.0
  *
  * This macro sets a compile-time constant limiting how many callback connections
  * can be registered on any one signal instance. It controls the size of the internal
  * fixed-size array that stores connection objects.
  *
  * Increasing this value allows more listeners but increases memory usage,
  * while lowering it restricts concurrency but reduces memory overhead.
  */
#define CPP_CONNECTIONS_MAX_CONNECTIONS 128
#endif

namespace connections {
    /**
     * @brief Custom implementation of move to not rely on the C++ standard library.
     * @since 1.1.0
     *
     * Converts an lvalue into an rvalue reference, enabling move semantics
     * without relying on the C++ standard library.
     *
     * This function is essential for implementing move constructors and
     * move assignment operators in environments where the standard library
     * is not used or is unavailable.
     *
     * @tparam T The type of the object being cast.
     * @param t The object to be cast to an rvalue reference.
     * @return An rvalue reference to the input object.
     */
    template<typename T>
    constexpr T&& move(T& t) noexcept {
        return static_cast<T&&>(t);
    }

    template<typename T>
    constexpr T&& move(T&& t) noexcept {
        static_assert(!__is_lvalue_reference(T), "Do not pass rvalue to move()");
        return static_cast<T&&>(t);
    }

    /**
     * @brief Represents an individual registered connection between a signal and a callback.
     * @since 1.0.0
     *
     * This structure encapsulates all state and information necessary to manage
     * a single callback registered to a signal. It tracks the connection's active
     * status, supports one-shot semantics, and stores the callback function pointer
     * alongside a user-provided context pointer.
     *
     * The connection can be disconnected to prevent further callback invocation
     * by marking it inactive, enabling the signal to efficiently skip disconnected slots.
     *
     * @tparam arguments Variadic template parameters representing the argument types
     *                   passed to the callback when invoked.
     */
    template<typename... arguments>
    struct connection {
        /**
         * @brief Indicates whether this connection is currently active and valid.
         * @since 1.0.0
         *
         * When set to `true`, this connection is considered live, and its callback
         * will be invoked when the signal fires. Setting this to `false` marks
         * the connection as disconnected, effectively disabling callback invocations
         * for this connection without removing the object.
         */
        bool connected;

        /**
         * @brief Specifies whether this connection is a one-shot connection.
         * @since 1.1.0
         *
         * When `true`, the connection automatically disconnects itself immediately
         * after the callback is invoked once. This is ideal for listeners that
         * should only respond to a single event occurrence and then stop.
         */
        bool once;

        /**
         * @brief Pointer to the callback function to invoke when the signal fires.
         * @since 1.0.0
         *
         * This function pointer must match the signature accepting a user context pointer
         * followed by the argument list defined by the template parameters.
         * It is called with the user context and the signal's event arguments.
         */
        void (*callback)(void* context, arguments...);

        /**
         * @brief User-defined context pointer passed to the callback.
         * @since 1.0.0
         *
         * This opaque pointer allows clients to associate arbitrary data or state
         * with the callback. It is passed unchanged as the first parameter to the
         * callback function on invocation.
         */
        void* context;

        /**
         * @brief Disconnects this connection by marking it as inactive.
         * @since 1.0.0
         *
         * This method disables the connection, causing the owning signal to
         * ignore it during future firing operations. This does not deallocate
         * memory but flags the connection as logically disconnected.
         */
        void disconnect() {
            connected = false;
        }
    };

    /**
     * @brief Manages a fixed-size container of connections and dispatches events to them.
     * @since 1.0.0
     *
     * This class implements a simple but efficient signal-slot event mechanism.
     * Clients can register multiple callbacks (connections) with this signal,
     * which will be invoked sequentially in the order they were added when the
     * signal is fired.
     *
     * The container has a fixed maximum capacity defined by `CPP_CONNECTIONS_MAX_CONNECTIONS`.
     * Attempting to add more connections beyond this limit will fail.
     *
     * Signals provide both persistent and one-shot connection registration, forwarding,
     * and management functions to control and modify connection behavior.
     *
     * @tparam arguments Template parameter pack specifying the argument types
     *                   that will be forwarded to each callback upon firing.
     */
    template<typename... arguments>
    class signal {
    public:
        /**
         * @brief Constructs a new signal instance with all connections initially disconnected.
         * @since 1.0.0
         *
         * The constructor initializes the internal connection array by marking
         * every slot as disconnected. The signal starts in an active state,
         * allowing callbacks to be invoked upon firing.
         */
        signal() : active(true) {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                connections[i].disconnect();
            }
        }

        /**
         * @brief Copy constructor.
         * @since 1.1.0
         *
         * Creates a new signal instance as a copy of another signal.
         * All internal connection states, callbacks, contexts, and flags
         * are duplicated, preserving the exact signal state.
         *
         * This allows independent copies of signals where connections remain consistent,
         * without sharing pointers or references.
         *
         * @param other The signal instance to copy from.
         */
        signal(const signal& other) : active(other.active) {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                connections[i] = other.connections[i];
            }
        }

        /**
         * @brief Copy assignment operator.
         * @since 1.1.0
         *
         * Assigns the contents and state of another signal instance to this one.
         * Existing connections are overwritten by the copied signal’s connections,
         * and the active state is updated accordingly.
         *
         * Self-assignment is safely handled by checking the address before copying.
         *
         * @param other The signal instance to copy from.
         * @return Reference to this signal after assignment.
         */
        signal& operator=(const signal& other) {
            if (this != &other) {
                active = other.active;
                for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                    connections[i] = other.connections[i];
                }
            }
            return *this;
        }

        /**
         * @brief Move constructor.
         * @since 1.1.0
         *
         * Moves the state of another signal instance into this one.
         * The other instance is left in a valid but unspecified state.
         *
         * This efficiently transfers ownership of all connection states,
         * callback pointers, contexts, and the active flag without copying.
         *
         * @param other The signal instance to move from.
         */
        signal(signal&& other) noexcept : active(other.active) {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                connections[i] = move(other.connections[i]);
            }
            other.active = false;
        }

        /**
         * @brief Move assignment operator.
         * @since 1.1.0
         *
         * Moves the state of another signal instance into this one, overwriting
         * the current state. The other instance is left in a valid but unspecified state.
         *
         * Self-move assignment is safely handled by checking the address before moving.
         *
         * @param other The signal instance to move from.
         * @return Reference to this signal after assignment.
         */
        signal& operator=(signal&& other) noexcept {
            if (this != &other) {
                active = other.active;
                for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                    connections[i] = move(other.connections[i]);
                }
                other.active = false;
            }
            return *this;
        }

        /**
         * @brief Destructor automatically disconnects all connections managed by this signal.
         * @since 1.1.0
         *
         * Upon destruction, this destructor calls `disconnect_all()` to ensure
         * no lingering active connections remain, preventing potential callbacks
         * to destroyed or invalid contexts.
         */
        ~signal() {
            disconnect_all();
        }

        /**
         * @brief Registers a persistent callback function with an associated user context.
         * @since 1.0.0
         *
         * This method searches for an available slot in the internal connection array.
         * If a free slot is found, it activates the connection, stores the callback
         * function pointer and the user context, and returns a pointer to the new connection.
         *
         * If the maximum number of connections has been reached, it returns nullptr.
         *
         * @param function Pointer to the callback function to invoke on signal firing.
         * @param context User-defined pointer passed to the callback when invoked.
         * @return Pointer to the newly created connection if successful, nullptr if full.
         */
        connection<arguments...>* connect(void (*function)(void* context, arguments...), void* context) {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                if (!connections[i].connected) {
                    connections[i].connected = true;
                    connections[i].once = false;
                    connections[i].callback = function;
                    connections[i].context = context;
                    return &connections[i];
                }
            }
            return nullptr;
        }

        /**
         * @brief Registers a one-shot callback function with an associated user context.
         * @since 1.1.0
         *
         * Similar to `connect()`, but the resulting connection will automatically
         * disconnect itself after the callback is invoked once, ensuring the listener
         * responds to only a single event.
         *
         * @param function Pointer to the callback function to invoke on signal firing.
         * @param context User-defined pointer passed to the callback when invoked.
         * @return Pointer to the new connection if successful, nullptr if full.
         */
        connection<arguments...>* once(void (*function)(void* context, arguments...), void* context) {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                if (!connections[i].connected) {
                    connections[i].connected = true;
                    connections[i].once = true;
                    connections[i].callback = function;
                    connections[i].context = context;
                    return &connections[i];
                }
            }
            return nullptr;
        }

        /**
         * @brief Sets up forwarding from this signal to another signal.
         * @since 1.1.0
         *
         * This method registers a connection on this signal that, when fired,
         * will invoke the `fire()` method on the target signal with the same arguments.
         * This effectively links the two signals so that events propagate from this
         * signal to the target.
         *
         * @param target Pointer to the signal instance that should receive forwarded events.
         * @return Pointer to the internal connection object responsible for forwarding.
         */
        connection<arguments...>* forward_to(signal<arguments...>* target) {
            return connect(
                [](void* context, arguments... args) {
                    auto* tgt = static_cast<signal<arguments...>*>(context);
                    tgt->fire(args...);
                },
                static_cast<void*>(target)
            );
        }

        /**
         * @brief Disconnects all currently active connections from this signal.
         * @since 1.1.0
         *
         * This method iterates over the entire internal connection array and
         * marks all active connections as disconnected. After calling this,
         * the signal will have no listeners and invoking `fire()` will result
         * in no callbacks being called.
         */
        void disconnect_all() {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                if (connections[i].connected) {
                    connections[i].disconnect();
                }
            }
        }

        /**
         * @brief Disconnects all connections whose callback function pointer matches the given pointer.
         * @since 1.1.0
         *
         * This method searches all active connections and disconnects those
         * whose callback function pointer is equal to the specified function pointer.
         * Useful for bulk removing listeners that share the same callback.
         *
         * @param callback The callback function pointer to match and disconnect.
         */
        void disconnect_by_callback(void (*callback)(void*, arguments...)) {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                if (connections[i].connected && connections[i].callback == callback) {
                    connections[i].disconnect();
                }
            }
        }

        /**
         * @brief Disconnects all connections whose user context pointer matches the given pointer.
         * @since 1.1.0
         *
         * This method iterates through all active connections and disconnects
         * those whose context pointer matches the provided context.
         * This is helpful for removing all listeners associated with a particular object or context.
         *
         * @param context The user-defined context pointer to match and disconnect.
         */
        void disconnect_by_context(void* context) {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                if (connections[i].connected && connections[i].context == context) {
                    connections[i].disconnect();
                }
            }
        }

        /**
         * @brief Suspends the signal, preventing any callbacks from being invoked during `fire()`.
         * @since 1.1.0
         *
         * When a signal is suspended, it maintains its list of active connections,
         * but temporarily disables callback invocation. This allows pausing event
         * dispatch without disconnecting or removing listeners.
         */
        void suspend() {
            active = false;
        }

        /**
         * @brief Resumes the signal, allowing callbacks to be invoked normally during `fire()`.
         * @since 1.1.0
         *
         * This re-enables callback dispatch after a prior suspension, restoring
         * normal signal behavior without needing to reconnect listeners.
         */
        void resume() {
            active = true;
        }

        /**
         * @brief Fires the signal, invoking all active callbacks with the provided arguments if active.
         * @since 1.0.0
         *
         * This function iterates through all registered connections and invokes
         * the callbacks for each connection that is currently active and connected.
         * One-shot connections are automatically disconnected immediately after invocation.
         *
         * If the signal is suspended (not active), this function returns immediately
         * without invoking any callbacks.
         *
         * @param args The argument pack forwarded to each callback function.
         */
        void fire(arguments... args) {
            if (!active) {
                return;
            }

            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                if (connections[i].connected && connections[i].callback) {
                    connections[i].callback(connections[i].context, args...);

                    if (connections[i].once) {
                        connections[i].disconnect();
                    }
                }
            }
        }

        /**
         * @brief Returns the compile-time maximum number of connections this signal can manage.
         * @since 1.1.0
         *
         * This value reflects the fixed-size capacity of the internal connections array,
         * as defined by the macro `CPP_CONNECTIONS_MAX_CONNECTIONS`.
         *
         * @return The maximum number of simultaneous connections supported by this signal.
         */
        int max_connections() const {
            return CPP_CONNECTIONS_MAX_CONNECTIONS;
        }

        /**
         * @brief Returns the current number of active connections registered to this signal.
         * @since 1.1.0
         *
         * Iterates through the internal connection array and counts how many
         * connections are marked as connected (active and not disconnected).
         *
         * @return The count of currently connected callbacks.
         */
        unsigned int connection_count() const {
            unsigned int count = 0;

            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                if (connections[i].connected) {
                    count++;
                }
            }
            return count;
        }
    private:
        /**
         * @brief Flag indicating whether the signal is currently active and firing callbacks.
         * @since 1.1.0
         *
         * When this flag is set to false, calls to `fire()` will immediately return
         * without invoking any callbacks, effectively suspending event dispatch.
         * Connections remain registered and can be resumed later.
         */
        bool active;

        /**
         * @brief Fixed-size array storing all possible connection slots managed by this signal.
         * @since 1.0.0
         *
         * Each element represents a possible registered callback connection,
         * including its active status, callback pointer, and context.
         * The size is defined by `CPP_CONNECTIONS_MAX_CONNECTIONS`.
         */
        connection<arguments...> connections[CPP_CONNECTIONS_MAX_CONNECTIONS];
    };

    /**
     * @brief RAII-style scoped wrapper for managing a single connection's lifetime.
     * @since 1.1.0
     *
     * This utility struct provides automatic management of a pointer to a
     * `connection` object, ensuring that the connection is properly disconnected
     * when the scoped_connection instance is destroyed. This pattern helps prevent
     * dangling connections and resource leaks by tying the connection's lifetime
     * to the scoped_connection's lifetime, following RAII (Resource Acquisition Is Initialization).
     *
     * The class disables copy operations to avoid multiple owners of the same
     * connection, which could lead to double-disconnect errors or undefined behavior.
     * Move semantics are supported to allow transferring ownership safely.
     *
     * Typical usage includes creating a scoped_connection from a raw connection pointer
     * returned by a signal's `connect()` method, allowing the connection to be
     * automatically disconnected when the scoped_connection goes out of scope.
     *
     * @tparam arguments The variadic template parameter pack representing the
     *                   argument types that the underlying connection's callback accepts.
     */
    template<typename... arguments>
    struct scoped_connection {
        /**
         * @brief Pointer to the managed connection object.
         * @since 1.1.0
         *
         * This raw pointer holds the address of the connection being managed by this
         * scoped_connection instance. It may be nullptr, indicating no connection
         * is currently managed. When valid and the connection is active (`connected == true`),
         * this pointer is used to invoke `disconnect()` automatically upon destruction.
         *
         * Ownership semantics:
         * - The scoped_connection assumes responsibility for disconnecting the connection,
         *   but does not delete or free the pointer itself.
         * - After destruction or move, this pointer is reset to nullptr to avoid dangling references.
         */
        connection<arguments...>* conn = nullptr;

        /**
         * @brief Default constructor creates an empty scoped_connection managing no connection.
         * @since 1.1.0
         *
         * The constructed object will hold a null connection pointer,
         * meaning no connection is managed and no disconnection will occur
         * upon destruction unless a connection is assigned later.
         */
        scoped_connection() = default;

        /**
         * @brief Constructs a scoped_connection that takes ownership of the given connection pointer.
         * @since 1.1.0
         *
         * The provided connection pointer is assumed to point to a valid connection
         * object whose lifetime is managed externally. This constructor does not
         * allocate or create a connection; it only takes responsibility for calling
         * `disconnect()` on destruction.
         *
         * @param c Pointer to the connection to manage. Must not be deleted
         *          by this scoped_connection, but will be disconnected on destruction.
         */
        explicit scoped_connection(connection<arguments...>* c) : conn(c) {}

        /**
         * @brief Destructor automatically disconnects the managed connection if it is active.
         * @since 1.1.0
         *
         * When the scoped_connection goes out of scope or is destroyed, this destructor
         * checks whether it currently owns a non-null pointer to an active connection.
         * If so, it calls the connection's `disconnect()` method to mark the connection
         * as inactive and prevent further callback invocations.
         *
         * This automatic disconnection helps ensure that dangling callbacks or
         * unexpected invocations do not occur after the scoped_connection is destroyed,
         * thereby improving safety and resource management.
         */
        ~scoped_connection() {
            if (conn && conn->connected) {
                conn->disconnect();
            }
        }

        /**
         * @brief Copy constructor is explicitly deleted to prevent copying ownership.
         * @since 1.1.0
         *
         * Copying scoped_connection objects is disallowed because multiple instances
         * managing the same connection pointer could lead to double disconnection attempts
         * and undefined behavior. This ensures unique ownership semantics.
         */
        scoped_connection(const scoped_connection&) = delete;

        /**
         * @brief Copy assignment operator is explicitly deleted to prevent copying ownership.
         * @since 1.1.0
         *
         * Copy assignment is disabled for the same reasons as the copy constructor:
         * to enforce exclusive ownership and avoid resource management errors.
         */
        scoped_connection& operator=(const scoped_connection&) = delete;

        /**
         * @brief Move constructor transfers ownership of the managed connection pointer.
         * @since 1.1.0
         *
         * This constructor enables transferring the management responsibility from
         * an existing scoped_connection to a new instance, leaving the moved-from
         * instance in a valid but empty state with a null pointer.
         *
         * Ownership is transferred by moving the raw connection pointer,
         * preventing disconnection from being called multiple times.
         *
         * @param other The scoped_connection instance to move from.
         */
        scoped_connection(scoped_connection&& other) noexcept : conn(other.conn) {
            other.conn = nullptr;
        }

        /**
         * @brief Move assignment operator disconnects current connection if active,
         *        then takes ownership from the given scoped_connection.
         * @since 1.1.0
         *
         * This operator enables assignment of a scoped_connection by transferring
         * ownership of the managed connection pointer from the `other` instance.
         * If this instance currently manages a connection, it will first disconnect it
         * to prevent leaks or lingering callbacks before taking over the new connection.
         *
         * After assignment, the moved-from instance is left in a valid empty state.
         *
         * @param other The scoped_connection instance to move from.
         * @return Reference to this scoped_connection after assignment.
         */
        scoped_connection& operator=(scoped_connection&& other) noexcept {
            if (this != &other) {
                if (conn && conn->connected) {
                    conn->disconnect();
                }
                conn = other.conn;
                other.conn = nullptr;
            }
            return *this;
        }

        /**
         * @brief Releases ownership of the managed connection without disconnecting it.
         * @since 1.1.0
         *
         * This function relinquishes management responsibility over the connection,
         * returning the raw pointer to the caller and setting the internal pointer
         * to nullptr to prevent disconnection on destruction.
         *
         * After calling `release()`, the scoped_connection no longer affects the connection's lifetime,
         * and the caller assumes full responsibility for managing the connection manually.
         *
         * @return The raw pointer to the managed connection prior to release, or nullptr if none.
         */
        connection<arguments...>* release() {
            auto temp = conn;
            conn = nullptr;
            return temp;
        }
    };

    /**
     * @brief Connects a pre-existing connection object’s callback to a given signal.
     * @since 1.0.0
     *
     * This free function facilitates connecting a callback described by a
     * `connection` struct to a specified signal instance, forwarding the
     * callback function pointer and user context stored in the connection.
     *
     * This is useful when managing or copying connection descriptors externally
     * before registering them to signals.
     *
     * @param connection The connection struct containing the callback and context.
     * @param signal Pointer to the signal instance where the callback should be connected.
     * @return Pointer to the internal connection object inside the signal, or nullptr if the signal is full.
     */
    template<typename... arguments>
    connection<arguments...>* connect(const connection<arguments...>& connection, signal<arguments...>* signal) {
        return signal->connect(connection.callback, connection.context);
    }

    /**
     * @brief Connects a one-shot connection object’s callback to a signal.
     * @since 1.1.0
     *
     * Similar to `connect()`, but this function registers the callback as a one-shot listener,
     * which will disconnect itself automatically after the first invocation.
     *
     * This facilitates easily setting up single-use event listeners externally.
     *
     * @param connection The connection struct containing the callback and context.
     * @param signal Pointer to the signal instance to register with.
     * @return Pointer to the internal connection object inside the signal, or nullptr if full.
     */
    template<typename... arguments>
    connection<arguments...>* connect_once(const connection<arguments...>& connection, signal<arguments...>* signal) {
        return signal->once(connection.callback, connection.context);
    }

    /**
     * @brief Establishes forwarding of events from one signal to another.
     * @note This is a convenience function that calls the `forward_to` member on the source signal.
     * @since 1.1.0
     *
     * When invoked, it sets up the source signal to automatically invoke
     * the target signal whenever it is fired, forwarding all event arguments transparently.
     *
     * This is useful for composing signals or chaining event propagation.
     *
     * @param from Pointer to the source signal that will forward its events.
     * @param to Pointer to the destination signal that will receive forwarded events.
     * @return Pointer to the connection object responsible for the forwarding, allowing management or disconnection.
     */
    template<typename... arguments>
    connection<arguments...>* forward_to(signal<arguments...>* from, signal<arguments...>* to) {
        return from->forward_to(to);
    }

    /**
     * @brief Disconnects a specific connection by marking it inactive.
     * @note This function takes the connection by reference to modify the original object.
     * @since 1.0.0
     *
     * Calling this function ensures that the given connection will no longer
     * be considered active by any signal, effectively preventing its callback
     * from being invoked on future signal firings.
     *
     * @param connection The connection object to disconnect.
     */
    template<typename... arguments>
    void disconnect(connection<arguments...>& connection) {
        connection.disconnect();
    }
}

#endif // !CPP_CONNECTIONS_HEADER_GUARD
