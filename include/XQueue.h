#pragma once

#include <mutex>
#include <deque>

namespace qlexnet
{
    template <typename T>
    class XQueue
    {
    public:
        XQueue() = default;
        XQueue(const XQueue<T> &) = delete;
        virtual ~XQueue() { clear(); }

    public:
        const T &front()
        {
            std::scoped_lock<std::mutex> lock(muxQueue);
            return deqQueue.front();
        }

        const T &back()
        {
            std::scoped_lock<std::mutex> lock(muxQueue);
            return deqQueue.back();
        }

        T pop_front()
        {
            std::scoped_lock<std::mutex> lock(muxQueue);
            auto t = std::move(deqQueue.front());
            deqQueue.pop_front();
            return t;
        }

        T pop_back()
        {
            std::scoped_lock<std::mutex> lock(muxQueue);
            auto t = std::move(deqQueue.back());
            deqQueue.pop_back();
            return t;
        }

        void push_back(const T &item)
        {
            std::scoped_lock<std::mutex> lock(muxQueue);
            deqQueue.emplace_back(std::move(item));

            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.notify_one();
        }

        void push_front(const T &item)
        {
            std::scoped_lock<std::mutex> lock(muxQueue);
            deqQueue.emplace_front(std::move(item));

            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.notify_one();
        }

        bool empty()
        {
            std::scoped_lock<std::mutex> lock(muxQueue);
            return deqQueue.empty();
        }

        size_t count()
        {
            std::scoped_lock<std::mutex> lock(muxQueue);
            return deqQueue.size();
        }

        void clear()
        {
            std::scoped_lock<std::mutex> lock(muxQueue);
            deqQueue.clear();
        }

        void wait()
        {
            while (empty())
            {
                std::unique_lock<std::mutex> ul(muxBlocking);
                cvBlocking.wait(ul);
            }
        }

        void wait_for(std::chrono::milliseconds timeout)
        {
            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.wait_for(ul, timeout, [this](){
                return !empty();
            });
        }

    protected:
        std::mutex muxQueue;
        std::deque<T> deqQueue;
        std::condition_variable cvBlocking;
        std::mutex muxBlocking;
    };
} // qlexnet