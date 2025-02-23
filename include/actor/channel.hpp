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

namespace channel
{

/// The buffer size of the channel.
struct MessageBufferSize
{
    size_t value;
};

template <typename T>
class Channel;

/// Thrown when a channel does not belong to the controller that is being used.
class ControllerMismatchError: public std::runtime_error
{
  public:
    ControllerMismatchError():
        std::runtime_error("Channel does not belong to the controller")
    {
    }
};

/// Manages multiple channels.
///
/// This is specifically important when intending to multiplex multiple receiving channels
class [[nodiscard]] Controller
{
  private:
    std::mutex _mutex;
    std::condition_variable _condition;
    std::atomic<size_t> _channelCount = 0;
    std::atomic<bool> _terminating = false;

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

    [[nodiscard]] bool terminating() const noexcept
    {
        return _terminating;
    }

    void terminate() noexcept
    {
        _terminating = true;
        notify_all();
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
    Channel<T> channel(MessageBufferSize maxBufferSize, std::string name = {});

    /// Selects all channels with available values.
    ///
    /// If no value is available, the caller will be blocked until a value is available.
    ///
    /// @returns A vector of indices of the channels with available values.
    template <typename... Ts>
    std::vector<size_t> select(Channel<Ts>&... channels);

    /// Selects all channels with available values.
    ///
    /// If no value is available, the caller will be blocked until a value is available for the specified timeout.
    ///
    /// @returns A vector of indices of the channels with available values or an empty vector if the timeout was
    /// reached.
    template <typename... Ts>
    std::vector<size_t> select_for(std::chrono::milliseconds timeout, Channel<Ts>&... channels);

    /// Selects all values available from multiple channels.
    ///
    /// If no value is available, the caller will be blocked until a value is available.
    ///
    /// @retval true If one or more values are available.
    /// @retval false If no value is available.
    template <typename Callable, typename... Ts>
        requires(std::invocable<Callable, Channel<Ts>&> || ...)
    bool select(Callable&& callable, Channel<Ts>&... channels);

    /// Selects all values available from multiple channels.
    ///
    /// If no value is available, the caller will be blocked until a value is available for the specified timeout.
    ///
    /// @retval true If one or more values are available.
    /// @retval false If no value is available.
    template <typename Callable, typename... Ts>
        requires(std::invocable<Callable, Channel<Ts>&> || ...)
    bool select_for(std::chrono::milliseconds timeout, Callable&& callable, Channel<Ts>&... channels);

    // TODO
    // template <typename... Ts>
    // std::optional<std::variant<Ts...>> select_value_for(std::chrono::milliseconds timeout, Ts&&... channels);
};

/// Thread-safe channel for sending and receiving messages.
///
/// @code
/// auto channel = channel::Channel<int> { MessageBufferSize { 1 } };
/// std::thread { [&channel] { channel.send(42); } }.detach();
/// std::thread { [&channel] { std::cout << channel.receive().value() << std::endl; } }.detach();
/// @endcode
template <typename T>
class [[nodiscard]] Channel
{
    friend class Controller;

  public:
    using value_type = T;

    /// Constructs a channel with a maximum buffer size.
    ///
    /// @param maxBufferSize The maximum buffer size of the channel.
    /// @param controller The controller to use for the channel.
    /// @param name The name of the channel.
    ///
    /// @note If no controller is provided, a new (internally owned) controller will be created.
    explicit Channel(MessageBufferSize maxBufferSize = { 1 }, Controller* controller = nullptr, std::string name = {});

    Channel(Channel&&) = default;
    Channel(Channel const&) = delete;
    Channel& operator=(Channel&&) = default;
    Channel& operator=(Channel const&) = delete;
    ~Channel();

    /// Retrieves the controller associated with the channel.
    [[nodiscard]] Controller const& controller() const noexcept
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
    std::unique_ptr<Controller> _ownedController;
    Controller* _controller;
    MessageBufferSize _maxBufferSize;
    std::deque<T> _queue;
    std::atomic<bool> _terminating = false;
    std::string _name;
};

// ----------------------------------------------------------------------------

template <typename T>
Channel<T>::Channel(MessageBufferSize maxBufferSize, Controller* controller, std::string name):
    _ownedController { controller ? nullptr : std::make_unique<Controller>() },
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
inline Channel<T> Controller::channel(MessageBufferSize maxBufferSize, std::string name)
{
    return Channel<T> { maxBufferSize, this, std::move(name) };
}

template <typename... Ts>
std::vector<size_t> Controller::select(Channel<Ts>&... channels)
{
    return select_for(std::chrono::years { 10 }, channels...);
}

template <typename... Ts>
std::vector<size_t> Controller::select_for(std::chrono::milliseconds timeout, Channel<Ts>&... channels)
{
    // clang-format off
    (
        [&] {
            if (&channels.controller() != this)
                throw ControllerMismatchError {};
        }(),
        ...
    );
    // clang-format on

    auto result = std::vector<size_t> {};
    auto const tryFetch = [&]<typename T>(Channel<T>& channel, size_t index) {
        auto const pending = channel._queue.size();
        for (size_t i = 0; i < pending; ++i)
            result.push_back(index);
    };
    auto lock = std::unique_lock { _mutex };
    if (!terminating())
    {
        _condition.wait_for(lock, timeout, [&] {
            result.clear();
            size_t index = 0;
            (tryFetch(channels, index++), ...);
            return !result.empty() || !alive() || terminating();
        });
    }
    return result;
}

template <typename Callable, typename... Ts>
    requires(std::invocable<Callable, Channel<Ts>&> || ...)
bool Controller::select(Callable&& callable, Channel<Ts>&... channels)
{
    return select_for(std::chrono::years { 10 }, std::forward<Callable>(callable), channels...);
}

template <typename Callable, typename... Ts>
    requires(std::invocable<Callable, Channel<Ts>&> || ...)
bool Controller::select_for(std::chrono::milliseconds timeout, Callable&& callable, Channel<Ts>&... channels)
{
    auto const result = select_for(timeout, channels...);

    for (auto const index: result)
    {
        size_t i = 0;
        // clang-format off
        (
            [&] {
                if (i == index)
                    std::forward<Callable>(callable)(channels);
                ++i;
            }(),
            ...
        );
        // clang-format on
    }

    return !result.empty();
}

} // namespace channel
