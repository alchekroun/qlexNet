#pragma once

#include "Message.h"
#include "XQueue.h"

#include <asio.hpp>
#include <asio/ts/buffer.hpp>

namespace qlexnet
{
    template <typename T>
    class Connection : public std::enable_shared_from_this<Connection<T>>
    {
    public:
        enum class owner
        {
            server,
            client
        };

    public:
        Connection(owner parent_, asio::io_context &asioContext_, asio::ip::tcp::socket socket_, XQueue<OwnedMessage<T>> &rxQueue_)
            : _asioContext(asioContext_), _socket(std::move(socket_)), _rxQueue(rxQueue_)
        {
            _ownerType = parent_;
        }

        virtual ~Connection() {}

        uint32_t GetID() const { return _id; }

    public:
        void connectToClient(uint32_t id_ = 0)
        {
            if (_ownerType == owner::server)
            {
                if (_socket.is_open())
                {
                    _id = id_;
                    readHeader();
                }
            }
        }

        void connectToServer(const asio::ip::tcp::resolver::results_type &endpoints_)
        {
            if (_ownerType == owner::client)
            {
                asio::async_connect(_socket, endpoints_,
                                    [this](std::error_code ec_, asio::ip::tcp::endpoint endpoint_)
                                    {
                                        if (!ec_)
                                        {
                                            readHeader();
                                        }
                                    });
            }
        }

        void disconnect()
        {
            if (isConnected())
            {
                asio::post(_asioContext, [this]()
                           { _socket.close(); });
            }
        }

        bool isConnected() const { return _socket.is_open(); }

        void startListening() {}

    public:
        void send(const Message<T> &msg_)
        {
            asio::post(_asioContext,
                       [this, msg_]()
                       {
                           bool bWritingMessage = !_txQueue.empty();
                           _txQueue.push_back(msg_);
                           if (!bWritingMessage)
                           {
                               writeHeader();
                           }
                       });
        }

    private:
        void writeHeader()
        {
            asio::async_write(_socket, asio::buffer(&_txQueue.front().header, sizeof(MessageHeader<T>)),
                              [this](std::error_code ec_, std::size_t length_)
                              {
                                  if (!ec_)
                                  {
                                      if (_txQueue.front().body.size() > 0)
                                      {
                                          writeBody();
                                      }
                                      else
                                      {
                                          _txQueue.pop_front();

                                          if (!_txQueue.empty())
                                          {
                                              writeHeader();
                                          }
                                      }
                                  }
                                  else
                                  {
                                      std::cout << "[" << _id << "] Write Header Fail.\n";
                                      _socket.close();
                                  }
                              });
        }

        void writeBody()
        {
            asio::async_write(_socket, asio::buffer(_txQueue.front().body.data(), _txQueue.front().body.size()),
                              [this](std::error_code ec_, std::size_t length_)
                              {
                                  if (!ec_)
                                  {
                                      _txQueue.pop_front();

                                      if (!_txQueue.empty())
                                      {
                                          writeHeader();
                                      }
                                  }
                                  else
                                  {
                                      std::cout << "[" << _id << "] Write Body Fail.\n";
                                      _socket.close();
                                  }
                              });
        }

        void readHeader()
        {
            asio::async_read(_socket, asio::buffer(&_msgRxTmp.header, sizeof(MessageHeader<T>)),
                             [this](std::error_code ec_, std::size_t length_)
                             {
                                 if (!ec_)
                                 {
                                     if (_msgRxTmp.header.size > 0)
                                     {
                                         _msgRxTmp.body.resize(_msgRxTmp.header.size);
                                         readBody();
                                     }
                                     else
                                     {
                                         addToIncomingMessageQueue();
                                     }
                                 }
                                 else
                                 {
                                     std::cout << "[" << _id << "] Read Header Fail. " << ec_ << " \n";
                                     _socket.close();
                                 }
                             });
        }

        void readBody()
        {
            asio::async_read(_socket, asio::buffer(_msgRxTmp.body.data(), _msgRxTmp.body.size()),
                             [this](std::error_code ec_, std::size_t length_)
                             {
                                 if (!ec_)
                                 {
                                     addToIncomingMessageQueue();
                                 }
                                 else
                                 {
                                     std::cout << "[" << _id << "] Read Body Fail.\n";
                                     _socket.close();
                                 }
                             });
        }

        void addToIncomingMessageQueue()
        {
            if (_ownerType == owner::server)
                _rxQueue.push_back({this->shared_from_this(), _msgRxTmp});
            else
                _rxQueue.push_back({nullptr, _msgRxTmp});

            readHeader();
        }

    protected:
        asio::ip::tcp::socket _socket;
        asio::io_context &_asioContext;
        XQueue<Message<T>> _txQueue;
        XQueue<OwnedMessage<T>> &_rxQueue;
        Message<T> _msgRxTmp;
        owner _ownerType = owner::server;
        uint32_t _id = 0;
    };
} // qlexnet