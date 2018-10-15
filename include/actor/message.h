// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <any>
#include <iostream>
#include <cstddef>
#include <vector>

namespace actor {

class Message {
  public:
	template <typename T>
	Message(const T& val) : value_{val}
	{
	}

	template <typename T>
	Message(T&& val) : value_{std::move(val)}
	{
	}

	Message() = default;
	Message(const Message&) = delete;
	Message(Message&&) = default;
	Message& operator=(const Message&) = delete;
	Message& operator=(Message&&) = default;

	template <typename T>
	bool is() const noexcept
	{
		return typeid(T) == value_.type();
	}

	template <typename T>
	auto get()
	{
		return std::any_cast<T>(value_);
	}

	template <typename T>
	const auto get() const
	{
		return std::any_cast<T>(value_);
	}

    /**
     * Tests if underlying value is of type @p T and invokes @p f if so.
     */
	template <typename T, typename U>
	Message& match(U f)
	{
		if (is<T>())
        {
            matched_ = true;
			f(get<T>());
        }

		return *this;
	}

    /**
     * Invokes given handler function @p f when no prior match<>() function matched the underlying
     * type.
     */
    template <typename U>
    void otherwise(U f)
    {
        if (!matched_)
            f();
    }

    /**
     * Expects given type @p T and invokes handler function @p f on underlying value.
     *
     * @throw std::bad_any_cast as get<T>() throws because of std::any_cast<T>().
     */
    template <typename T, typename U>
    void expect(U f)
    {
        f(get<T>());
    }

	// TODO: add Visitor pattern
	// message.visit(Vistor{ .... });

  private:
	std::any value_;
    bool matched_ = false;
};

}  // namespace actor
// vim:ts=4:sw=4
