// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace comms
{

/// The buffer size of the channel.
struct ChannelBufferSize
{
    size_t value;
};

template <typename T>
class Channel;

/// Manages multiple channels.
class [[nodiscard]] ChannelController
{
  private:
    std::mutex _mutex;
    std::condition_variable _condition;
    std::atomic<size_t> _channelCount = 0;

    template <typename T>
    friend class Channel;

  public:
    void lock()
    {
        _mutex.lock();
    }

    void unlock()
    {
        _mutex.unlock();
    }

    void notify_one()
    {
        _condition.notify_one();
    }

    void notify_all()
    {
        _condition.notify_all();
    }

    [[nodiscard]] bool alive() const noexcept
    {
        return _channelCount.load() > 0;
    }

    template <typename Predicate>
    [[nodiscard]] std::unique_lock<std::mutex> lock_wait(Predicate&& pred)
    {
        auto lock = std::unique_lock { _mutex };
        _condition.wait(lock, std::forward<Predicate>(pred));
        return lock;
    }

    template <typename Predicate>
    void wait(Predicate&& pred)
    {
        _condition.wait(*this, std::forward<Predicate>(pred));
    }

    template <typename T>
    Channel<T> channel(ChannelBufferSize maxBufferSize, std::string name = {});

    template <typename... Ts>
    std::vector<size_t> select(Channel<Ts>&... channels);

    /// Selects all values available from multiple channels.
    ///
    /// If no value is available, the caller will be blocked until a value is available.
    /// If one or more values are available, they will all be returned>
    template <typename Callable, typename... Ts>
        requires(std::invocable<Callable, Channel<Ts>&> || ...)
    bool select(Callable&& callable, Channel<Ts>&... channels);

    template <typename... Ts>
    std::optional<std::variant<Ts...>> select_value_for(std::chrono::milliseconds timeout, Ts&&... channels);
};

/// Thread-safe channel for sending and receiving messages.
///
/// @code
/// auto channel = Channel<int> { ChannelBufferSize { 1 } };
/// std::thread { [&channel] { channel.send(42); } }.detach();
/// std::thread { [&channel] { std::cout << channel.receive().value() << std::endl; } }.detach();
/// @endcode
template <typename T>
class [[nodiscard]] Channel
{
    friend class ChannelController;

  public:
    using value_type = T;

    explicit Channel(ChannelBufferSize maxBufferSize = { 1 },
                     ChannelController* controller = nullptr,
                     std::string name = {});

    Channel(Channel&&) = default;
    Channel(Channel const&) = delete;
    Channel& operator=(Channel&&) = default;
    Channel& operator=(Channel const&) = delete;
    ~Channel();

    [[nodiscard]] ChannelController const& controller() const noexcept
    {
        return *_controller;
    }

    /// Retrieves the channel name, useful for introspection/debugging purposes.
    [[nodiscard]] std::string const& name() const noexcept;

    /// Returns the maximum buffer size of the channel.
    [[nodiscard]] size_t capacity() const noexcept;

    /// Returns true if the channel is empty, false otherwise.
    [[nodiscard]] bool empty() const noexcept;

    /// Returns the current buffer size of the channel.
    [[nodiscard]] size_t size() const noexcept;

    /// Sends a message to the channel.
    ///
    /// If the channel is full, the caller will be blocked until the message can be sent.
    template <typename U>
        requires std::convertible_to<U, T>
    void send(U&& value);

    /// Receives a message from the channel.
    ///
    /// If the channel is empty, the caller will be blocked until a message is available.
    /// If the channel is closed, std::nullopt will be returned.
    [[nodiscard]] std::optional<T> receive();

    /// Tries to receive a value without blocking, returning std::nullopt if no value is available.
    [[nodiscard]] std::optional<T> try_receive();

    /// Closes the channel.
    void close() noexcept;

  private:
    std::unique_ptr<ChannelController> _ownedController;
    ChannelController* _controller;
    ChannelBufferSize _maxBufferSize;
    std::deque<T> _queue;
    std::atomic<bool> _terminating = false;
    std::string _name;
};

// ----------------------------------------------------------------------------

template <typename T>
Channel<T>::Channel(ChannelBufferSize maxBufferSize, ChannelController* controller, std::string name):
    _ownedController { controller ? nullptr : std::make_unique<ChannelController>() },
    _controller { controller ? controller : _ownedController.get() },
    _maxBufferSize { maxBufferSize },
    _name { std::move(name) }
{
    ++_controller->_channelCount;
}

template <typename T>
Channel<T>::~Channel()
{
    close();
}

template <typename T>
template <typename U>
    requires std::convertible_to<U, T>
void Channel<T>::send(U&& value)
{
    auto _ = _controller->lock_wait([this]() { return _queue.size() < _maxBufferSize.value || _terminating.load(); });
    _queue.emplace_back(std::forward<U>(value));
    _controller->notify_one();
}

template <typename T>
std::optional<T> Channel<T>::receive()
{
    auto _ = _controller->lock_wait([this]() { return !_queue.empty() || _terminating.load(); });

    if (_queue.empty())
        return std::nullopt;

    auto value = std::move(_queue.front());
    _queue.pop_front();
    _controller->notify_one();
    return value;
}

template <typename T>
std::optional<T> Channel<T>::try_receive()
{
    auto lock = std::unique_lock { *_controller };
    if (_queue.empty())
        return std::nullopt;

    auto value = std::move(_queue.front());
    _queue.pop_front();
    _controller->notify_one();
    return value;
}

template <typename T>
inline bool Channel<T>::empty() const noexcept
{
    auto _ = std::unique_lock { *_controller };
    return _queue.empty();
}

template <typename T>
inline size_t Channel<T>::size() const noexcept
{
    auto _ = std::unique_lock { *_controller };
    return _queue.size();
}

template <typename T>
inline std::string const& Channel<T>::name() const noexcept
{
    return _name;
}

template <typename T>
inline size_t Channel<T>::capacity() const noexcept
{
    auto _ = std::unique_lock { *_controller };
    return _maxBufferSize.value;
}

template <typename T>
inline void Channel<T>::close() noexcept
{
    auto const wasClosedBefore = _terminating.exchange(true);
    if (wasClosedBefore)
        return;

    --_controller->_channelCount;
    _controller->notify_all();
}

// ----------------------------------------------------------------------------

template <typename T>
inline Channel<T> ChannelController::channel(ChannelBufferSize maxBufferSize, std::string name)
{
    return Channel<T> { maxBufferSize, this, std::move(name) };
}

template <typename... Ts>
std::vector<size_t> ChannelController::select(Channel<Ts>&... channels)
{
    auto result = std::vector<size_t> {};
    auto const tryFetch = [&]<typename T>(Channel<T>& channel, size_t index) {
        auto const pending = channel._queue.size();
        for (size_t i = 0; i < pending; ++i)
            result.push_back(index);
    };
    auto lock = std::unique_lock { _mutex };
    _condition.wait(lock, [&] {
        result.clear();
        size_t index = 0;
        (tryFetch(channels, index++), ...);
        return !result.empty() || !alive();
    });
    return result;
}

template <typename Callable, typename... Ts>
    requires(std::invocable<Callable, Channel<Ts>&> || ...)
bool ChannelController::select(Callable&& callable, Channel<Ts>&... channels)
{
    auto const result = select(channels...);

    for (auto const index: result)
    {
        size_t i = 0;
        // clang-format off
        (
            [&] {
                if (i == index)
                    std::forward<Callable>(callable).template operator()<Ts>(channels);
                ++i;
            }(),
            ...
        );
        // clang-format on
    }

    return !result.empty();
}

} // namespace comms
