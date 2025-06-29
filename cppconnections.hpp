/**
 * @file cppconnections.hpp
 * @version 1.0.0
 * @brief A minimal signal-slot style callback system for C++.
 * 
 * This header defines a lightweight, standalone signal/connection mechanism
 * that allows callbacks to be registered and invoked without requiring the
 * C++ standard library or external dependencies.
 * 
 * @copyright MIT License
 *
 * @details Copyright (c) 2025 Warren Austin
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
     * @brief The maximum number of connections that a signal can store.
     */
    #define CPP_CONNECTIONS_MAX_CONNECTIONS 128
#endif

namespace connections {
    /**
     * @brief Represents a single connection between a signal and a callback.
     *
     * This structure stores the state and information needed to manage
     * a callback registered to a signal. It holds a pointer to the callback
     * function, a user-defined context pointer passed to the callback, and
     * a flag indicating whether the connection is currently active.
     *
     * When a signal fires, only active connections are invoked.
     * The connection can be disconnected by calling `disconnect()`.
     *
     * @tparam arguments The types of arguments the callback accepts.
     */
    template<typename... arguments>
    struct connection {
        /**
         * @brief Indicates if the connection is active.
         *
         * When false, the connection is considered disconnected and
         * will be ignored by the signal when firing.
         */
        bool connected;

        /**
         * @brief Callback function pointer.
         *
         * The function to call when the signal is fired. It receives
         * the user context pointer and the event arguments.
         */
        void (*callback)(void* context, arguments...);

        /**
         * @brief User-defined context pointer.
         *
         * This pointer is passed as the first argument to the callback,
         * allowing user data to be carried along with the callback.
         */
        void* context;

        /**
         * @brief Disconnects this connection.
         *
         * Marks the connection as inactive by changing an internal flag causing
         * signals to ignore it when firing connections.
         */
        void disconnect() {
            connected = false;
        }
    };

    /**
     * @brief Manages a fixed-size list of connections and fires events to them.
     *
     * This class implements a signal (or event) system that allows clients
     * to register callback functions (connections) which will be invoked
     * when the signal is fired. It supports up to `CPP_CONNECTIONS_MAX_CONNECTIONS`
     * simultaneous active connections.
     *
     * Callbacks are functions taking a user context pointer and a variable
     * argument list specified by the template parameters.
     *
     * When the signal is fired, all active connections are invoked in order.
     * Connections can be added using `connect()`, which returns a pointer to
     * the new connection, or `nullptr` if the maximum number of connections
     * has been reached.
     *
     * Connections can be individually disconnected to stop receiving events.
     *
     * @tparam arguments The types of arguments forwarded to the callbacks.
     */
    template<typename... arguments>
    class signal {
    public:
        /**
         * @brief Constructs an empty signal with no active connections.
         */
        signal() {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                connections[i].connected = false;
            }
        }

        /**
         * @brief Registers a callback function with a context to this signal.
         *
         * Finds an inactive connection slot, activates it, and stores the
         * callback and context.
         *
         * @param function Pointer to the callback function.
         * @param context User-defined pointer passed back during callback.
         * @return Pointer to the connection object if successful, or nullptr if full.
         */
        connection<arguments...>* connect(void (*function)(void* context, arguments...), void* context) {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                if (!connections[i].connected) {
                    connections[i].connected = true;
                    connections[i].callback = function;
                    connections[i].context = context;
                    return &connections[i];
                }
            }
            return nullptr;
        }

        /**
         * @brief Fires the signal, invoking all active callbacks with the given arguments.
         *
         * @param args The arguments forwarded to each callback.
         */
        void fire(arguments... args) {
            for (int i = 0; i < CPP_CONNECTIONS_MAX_CONNECTIONS; ++i) {
                if (connections[i].connected && connections[i].callback) {
                    connections[i].callback(connections[i].context, args...);
                }
            }
        }

    private:
        /**
         * @brief Storage array for all possible connections managed by this signal.
         *
         * This fixed-size array holds the connection objects that represent
         * registered callbacks. Each element tracks whether it is active,
         * along with the callback function and user context.
         *
         * The maximum number of connections is defined by
         * `CPP_CONNECTIONS_MAX_CONNECTIONS`.
         */
        connection<arguments...> connections[CPP_CONNECTIONS_MAX_CONNECTIONS];
    };

    /**
     * @brief Connects a connection's callback to a signal.
     *
     * @param connection The connection object containing the callback and context.
     * @param signal A pointer to the signal being connected.
     *
     * This function will call `connect(...)` on the signal using the callback and
     * context stored in the connection object.
     */
    template<typename... arguments>
    void connect(connection<arguments...>& connection, signal<arguments...>* signal) {
        signal->connect(connection.callback, connection.context);
    }

    /**
     * @brief Disconnects a connection.
     * @param connection The connection that will be disconnected.
     *
     * This function will call `disconnect()` on the connection provided which marks
     * it as inactive.
     *
     * Note: The connection is taken by reference so the disconnection affects
     *       the original connection object.
     */
    template<typename... arguments>
    void disconnect(connection<arguments...>& connection) {
        connection.disconnect();
    }
}

#endif // !CPP_CONNECTIONS_HEADER_GUARD
