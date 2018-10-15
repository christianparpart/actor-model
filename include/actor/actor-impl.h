// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

namespace actor {

template <typename T>
inline Actor::Actor(const T& handler) : Actor(T{handler})
{
}

template <typename T>
inline Actor::Actor(T&& handler)
	: handler_{std::move(handler)}, killing_{false}, inbox_{}, thread_{std::bind(&Actor::main, this)}
{
}

inline Actor::~Actor()
{
	killing_.store(true);
	condition_.notify_one();
	thread_.join();
}

inline void Actor::main()
{
	handler_(Receiver{*this});
}

inline std::optional<Message> Actor::receive()
{
	std::unique_lock lock{lock_};
	condition_.wait(lock, [this]() { return !inbox_.empty() || killing_.load(); });
	if (!inbox_.empty())
	{
		Message m = std::move(inbox_.front());
		inbox_.pop_front();
		return std::move(m);
	}
	return std::nullopt;
}

inline void Actor::send(Message&& message)
{
	std::unique_lock lock{lock_};
	inbox_.emplace_back(std::move(message));
	condition_.notify_one();
}

inline Actor& Actor::operator<<(Message&& m)
{
	send(std::move(m));
	return *this;
}

inline std::optional<Message> Receiver::receive()
{
    return actor_.receive();
}

inline Receiver::iterator& Receiver::iterator::operator++()
{
	if (std::optional<Message> m = actor.receive())
		value = std::move(*m);
	else
		eos = true;

	return *this;
}

inline Receiver::iterator Receiver::begin()
{
    if (auto mesg = actor_.receive())
        return iterator{actor_, std::move(*mesg), false};
    else
        return end();
}

inline Receiver::iterator Receiver::end()
{
	return iterator{actor_, {}, true};
}

}  // namespace actor

// vim:ts=4:sw=4:et
