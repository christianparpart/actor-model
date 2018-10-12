// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

namespace actor {

template <typename Message>
template <typename T>
Actor<Message>::Actor(const T& handler) : Actor<Message>(T{handler})
{
}

template <typename Message>
template <typename T>
Actor<Message>::Actor(T&& handler)
	: handler_{std::move(handler)}, killing_{false}, inbox_{}, thread_{std::bind(&Actor<Message>::main, this)}
{
}

template <typename Message>
Actor<Message>::~Actor()
{
	killing_.store(true);
	condition_.notify_one();
	thread_.join();
}

template <typename Message>
void Actor<Message>::main()
{
	handler_(std::bind(&Actor<Message>::receive, this));
}

template <typename Message>
std::optional<Message> Actor<Message>::receive()
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

template <typename Message>
void Actor<Message>::send(const Message& message)
{
	std::unique_lock lock{lock_};
	inbox_.emplace_back(message);
	condition_.notify_one();
}

template <typename Message>
void Actor<Message>::send(Message&& message)
{
	std::unique_lock lock{lock_};
	inbox_.emplace_back(std::move(message));
	condition_.notify_one();
}

template <typename Message>
Actor<Message>& Actor<Message>::operator<<(const Message& m)
{
	send(m);
	return *this;
}

template <typename Message>
Actor<Message>& Actor<Message>::operator<<(Message&& m)
{
	send(std::move(m));
	return *this;
}

}  // namespace actor

// vim:ts=4:sw=4:et
