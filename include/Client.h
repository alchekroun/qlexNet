#pragma once

#include <ostream>

#include "Message.h"
#include "XQueue.h"
#include "Connection.h"

namespace qlexnet
{
    template <typename T>
    class ClientInterface
    {
    public:
        ClientInterface() {}
        virtual ~ClientInterface() { disconnect(); }

    public:
        bool connect(const std::string &host_, const uint16_t port_)
        {
            try
            {
                asio::ip::tcp::resolver resolver(_context);
                asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host_, std::to_string(port_));

                // Create connection
                _connection = std::make_unique<Connection<T>>(Connection<T>::owner::client, _context, asio::ip::tcp::socket(_context), _rxQueue);

                // Tell the connection object to connect to server
                _connection->connectToServer(endpoints);

                // Start Context Thread
                thrContext = std::thread([this]()
                                         { _context.run(); });
            }
            catch (std::exception &e)
            {
                std::cerr << "Client Exception: " << e.what() << "\n";
                return false;
            }
            return true;
        }

        void disconnect()
        {
            if (isConnected())
            {
                _connection->disconnect();
            }

            _context.stop();
            if (thrContext.joinable())
            {
                thrContext.join();
            }

            _connection.release();
        }

        // Check if client is actually connected to a server
        bool isConnected()
        {
            if (_connection)
            {
                return _connection->isConnected();
            }
            return false;
        }

    public:
        // Send message to server
        void send(const Message<T> &msg_)
        {
            if (isConnected())
            {
                _connection->send(msg_);
            }
        }

        // Retrieve queue of messages from server
        XQueue<OwnedMessage<T>> &incoming()
        {
            return _rxQueue;
        }

    protected:
        // asio context handles the data transfer...
        asio::io_context _context;
        // ...but needs a thread of its own to execute its work commands
        std::thread thrContext;
        // The client has a single instance of a "connection" object, which handles data transfer
        std::unique_ptr<Connection<T>> _connection;

    private:
        XQueue<OwnedMessage<T>> _rxQueue;
    };
} // qlexnet