#pragma once

namespace qlexnet
{
    template <typename T>
    class XQueue
    {
    public:
        tsqueue() = default;
        tsqueue(const tsqueue<T> &) = delete;
        virtual ~tsqueue() { clear(); }

        public:
        
    };
} // qlexnet