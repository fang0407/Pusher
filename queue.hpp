#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <list>
#include <mutex>
#include <functional>
#include <condition_variable>

namespace gcm {
namespace container {

template<typename T, typename Container = std::list<T>, typename Mutex=std::mutex, typename Cond=std::condition_variable>
class Queue {
public:
    using ValueType = T;
    using ContainerType = Container;
    using PredicateFn = std::function<bool (void)>;
public:
    Queue() = default;
    Queue(const Queue&) = delete;
    Queue operator=(const Queue&) = delete;
    Queue(const Queue&&) = delete;
    Queue operator=(const Queue&&) = delete;

public:
    template<typename TREF>
    void sync_emplace(TREF&& data)
    {
        std::unique_lock<Mutex> lck(mu_);
        queue_.emplace_back(std::forward<TREF>(data));
        cv_.notify_one();
    }

    void sync_emplace(Container&& queue)
    {
        std::unique_lock<Mutex> lck(mu_);
        std::copy(
            std::make_move_iterator(queue.begin()),
            std::make_move_iterator(queue.end()),
            std::back_inserter(queue_));
        cv_.notify_all();
    }

    void sync_pop(T& data)
    {
        std::unique_lock<Mutex> lck(mu_);
        cv_.wait(lck, [this]{ return !queue_.empty(); });
        data = std::move(queue_.front());
        queue_.pop_front();
    }

    void sync_pop(T& data, PredicateFn cb)
    {
        std::unique_lock<Mutex> lck(mu_);
        cv_.wait(lck, [this, cb]{ return cb(); });
        if (!queue_.empty()) {
            data = std::move(queue_.front());
            queue_.pop_front();
        }
    }

    bool sync_pop_for(T& data, int milliseconds)
    {
        std::unique_lock<Mutex> lck(mu_);
        auto wait_timeout = std::chrono::milliseconds(milliseconds);
        bool ret = cv_.wait_for(lck, wait_timeout, [this]{ return !queue_.empty(); });
        if (ret) {
            data = std::move(queue_.front());
            queue_.pop_front();
        }
        return ret;
    }

    bool sync_pop_for(T& data, int milliseconds, PredicateFn cb)
    {
        std::unique_lock<Mutex> lck(mu_);
        auto wait_timeout = std::chrono::milliseconds(milliseconds);
        bool ret = cv_.wait_for(lck, wait_timeout, [this, cb]{ return cb(); });
        if (ret) {
            if (!queue_.empty()) {
                data = std::move(queue_.front());
                queue_.pop_front();
            }
        }
        return ret;
    }

    void sync_pop(Container& queue)
    {
        std::unique_lock<Mutex> lck(mu_);
        cv_.wait(lck, [this]{ return !queue_.empty(); });
        std::swap(queue_, queue);
    }

    void sync_pop(Container& queue, PredicateFn cb)
    {
        std::unique_lock<Mutex> lck(mu_);
        cv_.wait(lck, [this, cb]{ return cb(); });
        std::swap(queue_, queue);
    }

    bool sync_pop_for(Container& queue, int milliseconds)
    {
        std::unique_lock<Mutex> lck(mu_);
        auto wait_timeout = std::chrono::milliseconds(milliseconds);
        bool ret = cv_.wait_for(lck, wait_timeout, [this]{ return !queue_.empty(); });
        if (ret)
            std::swap(queue_, queue);
        return ret;
    }

    bool sync_pop_for(Container& queue, int milliseconds, PredicateFn cb)
    {
        std::unique_lock<Mutex> lck(mu_);
        auto wait_timeout = std::chrono::milliseconds(milliseconds);
        bool ret = cv_.wait_for(lck, wait_timeout, [this, cb]{ return cb(); });
        if (ret)
            std::swap(queue_, queue);
        return ret;
    }

    template<typename TREF>
    void emplace(TREF&& data)
    {
        std::unique_lock<Mutex> lck(mu_);
        queue_.emplace_back(std::forward<TREF>(data));
    }

    void pop(T& data)
    {
        std::unique_lock<Mutex> lck(mu_);
        data = std::move(queue_.front());
        queue_.pop_front();
    }

    void clear()
    {
        std::lock_guard<Mutex> lck(mu_);
        return queue_.clear();
    }

    size_t size()
    {
        std::lock_guard<Mutex> lck(mu_);
        return queue_.size();
    }

    bool empty()
    {
        std::lock_guard<Mutex> lck(mu_);
        return queue_.empty();
    }

    void notify()
    {
        cv_.notify_all();
    }

    Container* queue()
    {
        return &queue_;
    }

    Mutex* mutex()
    {
        return &mu_;
    }

private:
    mutable Mutex mu_;
    Cond cv_;

    Container queue_;
};

}
}


#endif // QUEUE_HPP
