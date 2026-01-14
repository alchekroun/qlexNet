#pragma once

#include <vector>

namespace qlexnet
{
    template <typename T>
    struct MessageHeader
    {
        T id{};
        uint32_t size = 0;
    };

    template <typename T>
    struct Message
    {
        MessageHeader<T> header{};
        // TODO :   Standardize msg length and body
        //          Body should always be composed
        //          by the same data.
        //          All transmisted in binary.
        std::vector<uint8_t> body;

        size_t size() const
        {
            return body.size();
        }

        friend std::ostream &operator<<(std::ostream &os, const Message<T> &msg)
        {
            os << "[ID " << msg.header.id << "] Size : " << msg.header.size;
            return os;
        }

        template <typename DataType>
        friend Message<T> &operator<<(Message<T> &msg, const DataType &data)
        {
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");
            size_t offset = msg.body.size();
            msg.body.resize(offset + sizeof(DataType));
            std::memcpy(msg.body.data() + offset, &data, sizeof(DataType));
            msg.header.size = msg.size();
            return msg;
        }

        template <typename DataType>
        friend Message<T> &operator>>(Message<T> &msg, DataType &data)
        {
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");
            size_t i = msg.body.size() - sizeof(DataType);
            std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
            msg.body.resize(i);
            msg.header.size = msg.size();
            return msg;
        }
    };

    template <typename T>
    class Connection;

    template <typename T>
    struct OwnedMessage
    {
        std::shared_ptr<Connection<T>> remote = nullptr;
        Message<T> msg;

        // Again, a friendly string maker
        friend std::ostream &operator<<(std::ostream &os, const OwnedMessage<T> &msg)
        {
            os << msg.msg;
            return os;
        }
    };

    template <typename T>
    class MessageWriter
    {
    public:
        explicit MessageWriter(Message<T>& msg) : _msg(msg) {}

        template <typename DataType>
        void write(const DataType& data)
        {
            static_assert(std::is_trivially_copyable_v<DataType>,
                          "DataType must be trivially copyable");

            size_t offset = _msg.body.size();
            _msg.body.resize(offset + sizeof(DataType));
            std::memcpy(_msg.body.data() + offset, &data, sizeof(DataType));
            _msg.header.size = static_cast<uint32_t>(_msg.body.size());
        }

        void writeString(const std::string& s)
        {
            uint32_t len = static_cast<uint32_t>(s.size());
            write(len);

            size_t offset = _msg.body.size();
            _msg.body.resize(offset + len);
            std::memcpy(_msg.body.data() + offset, s.data(), len);
            _msg.header.size = static_cast<uint32_t>(_msg.body.size());
        }

    private:
        Message<T>& _msg;
    };
    template <typename T>
    class MessageReader
    {
    public:
        explicit MessageReader(const Message<T>& msg)
            : _msg(msg) {}

        template <typename DataType>
        void read(DataType& out)
        {
            static_assert(std::is_trivially_copyable_v<DataType>,
                          "DataType must be trivially copyable");

            if (_offset + sizeof(DataType) > _msg.body.size())
                throw std::runtime_error("MessageReader overflow");

            std::memcpy(&out, _msg.body.data() + _offset, sizeof(DataType));
            _offset += sizeof(DataType);
        }

        std::string readString()
        {
            uint32_t len;
            read(len);

            if (_offset + len > _msg.body.size())
                throw std::runtime_error("Invalid string length");

            std::string s(len, '\0');
            std::memcpy(s.data(), _msg.body.data() + _offset, len);
            _offset += len;
            return s;
        }

    private:
        const Message<T>& _msg;
        size_t _offset = 0;
    };

} // qlexnet