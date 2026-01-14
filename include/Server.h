#pragma once

#include "Message.h"
#include "XQueue.h"
#include "Connection.h"

#include <memory>

namespace qlexnet
{

    template <typename T>
    class ServerInterface
    {
    public:
        ServerInterface(uint16_t port_)
            : _asioAcceptor(_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_))
        {
        }

        virtual ~ServerInterface() { stop(); }

        bool start()
        {
            try
            {
                waitForClientConnection();
                _threadContext = std::thread([this]()
                                             { _asioContext.run(); });
            }
            catch (std::exception &e)
            {
                std::cerr << "[SERVER] Exception: " << e.what() << "\n";
                return false;
            }
            std::cout << "[SERVER] Started!\n";
            return true;
        }

        void stop()
        {
            _asioContext.stop();

            if (_threadContext.joinable())
                _threadContext.join();

            std::cout << "[SERVER] Stopped!\n";
        }

        void waitForClientConnection()
        {
            _asioAcceptor.async_accept(
                [this](std::error_code ec, asio::ip::tcp::socket socket)
                {
                    if (!ec)
                    {
                        std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;

                        std::shared_ptr<Connection<T>> newconn =
                            std::make_shared<Connection<T>>(Connection<T>::owner::server,
                                                            _asioContext, std::move(socket), _rxQueue);

                        if (onClientConnect(newconn))
                        {
                            _connections.push_back(std::move(newconn));
                            _connections.back()->connectToClient(nIDCounter++);

                            std::cout << "[" << _connections.back()->GetID() << "] Connection Approved" << std::endl;
                        }
                        else
                        {
                            std::cout << "[-----] Connection Denied" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
                    }
                    waitForClientConnection();
                });
        }

        void messageClient(std::shared_ptr<Connection<T>> client_, const Message<T> &msg_)
        {
            if (client_ && client_->isConnected())
            {
                client_->send(msg_);
            }
            else
            {
                onClientDisconnect(client_);
                client_.reset();
                _connections.erase(
                    std::remove(_connections.begin(), _connections.end(), client_), _connections.end());
            }
        }

        void messageClient(uint32_t id_, const Message<T>& msg_)
        {
            std::shared_ptr<Connection<T>> client;
            for (const auto& c : _connections) {
                if (c->GetID() == id_) {
                    client = c;
                }
            }
            if (client == nullptr) return;

            messageClient(client, msg_);
        }

        void messageAllClients(const Message<T> &msg_, std::shared_ptr<Connection<T>> pIgnoreClient_ = nullptr)
        {
            bool invalidClientExists = false;

            for (auto &client : _connections)
            {
                if (client && client->isConnected())
                {
                    if (client != pIgnoreClient_)
                        client->send(msg_);
                }
                else
                {
                    onClientDisconnect(client);
                    client.reset();

                    invalidClientExists = true;
                }
            }

            if (invalidClientExists)
                _connections.erase(
                    std::remove(_connections.begin(), _connections.end(), nullptr), _connections.end());
        }

        virtual void update(size_t maxMessages_ = -1, bool wait_ = false, std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
        {
            if (wait_)
                _rxQueue.wait_for(timeout);

            size_t msgCount = 0;
            while (msgCount < maxMessages_ && !_rxQueue.empty())
            {
                auto msg = _rxQueue.pop_front();

                onMessage(msg.remote, msg.msg);

                msgCount++;
            }
        }

    protected:
        virtual bool onClientConnect(std::shared_ptr<Connection<T>> client_) { return false; }
        virtual void onClientDisconnect(std::shared_ptr<Connection<T>> client_) {}
        virtual void onMessage(std::shared_ptr<Connection<T>> client_, Message<T> &msg_) {}

    protected:
        XQueue<OwnedMessage<T>> _rxQueue;
        std::vector<std::shared_ptr<Connection<T>>> _connections;
        asio::io_context _asioContext;
        std::thread _threadContext;

        asio::ip::tcp::acceptor _asioAcceptor; // Handles new incoming connection attempts...

        // Clients will be identified in the "wider system" via an ID
        uint32_t nIDCounter = 10000;
    };

} // qlexnet