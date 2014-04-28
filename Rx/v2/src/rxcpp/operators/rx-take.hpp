// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_OPERATORS_RX_TAKE_HPP)
#define RXCPP_OPERATORS_RX_TAKE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace operators {

namespace detail {

template<class T, class Observable, class Count>
struct take : public operator_base<T>
{
    typedef typename std::decay<Observable>::type source_type;
    typedef typename std::decay<Count>::type count_type;
    struct values
    {
        values(source_type s, count_type t)
            : source(std::move(s))
            , count(std::move(t))
        {
        }
        source_type source;
        count_type count;
    };
    values initial;

    take(source_type s, count_type t)
        : initial(std::move(s), std::move(t))
    {
    }

    struct mode
    {
        enum type {
            taking,
            clear,
            triggered,
            errored,
            stopped
        };
    };

    template<class Subscriber>
    void on_subscribe(const Subscriber& s) {

        typedef Subscriber output_type;
        struct state_type
            : public std::enable_shared_from_this<state_type>
            , public values
        {
            state_type(const values& i, const output_type& oarg)
                : values(i)
                , mode_value(mode::taking)
                , out(oarg)
            {
            }
            typename mode::type mode_value;
            output_type out;
        };
        // take a copy of the values for each subscription
        auto state = std::shared_ptr<state_type>(new state_type(initial, s));

        state->source.subscribe(
        // share subscription lifetime
            state->out,
        // on_next
            [state](T t) {
                if (state->mode_value < mode::triggered) {
                    if (--state->count > 0) {
                        state->out.on_next(t);
                    } else {
                        state->out.on_next(t);
                        state->mode_value = mode::clear;
                        state->out.unsubscribe();
                    }
                }
            },
        // on_error
            [state](std::exception_ptr e) {
                state->mode_value = mode::errored;
                state->out.on_error(e);
            },
        // on_completed
            [state]() {
                state->mode_value = mode::triggered;
                state->out.on_completed();
            }
        );
    }
};

template<class T>
class take_factory
{
    typedef typename std::decay<T>::type count_type;
    count_type count;
public:
    take_factory(count_type t) : count(std::move(t)) {}
    template<class Observable>
    auto operator()(Observable&& source)
        ->      observable<typename std::decay<Observable>::type::value_type,   take<typename std::decay<Observable>::type::value_type, Observable, count_type>> {
        return  observable<typename std::decay<Observable>::type::value_type,   take<typename std::decay<Observable>::type::value_type, Observable, count_type>>(
                                                                                take<typename std::decay<Observable>::type::value_type, Observable, count_type>(std::forward<Observable>(source), count));
    }
};

}

template<class T>
auto take(T&& t)
    ->      detail::take_factory<T> {
    return  detail::take_factory<T>(std::forward<T>(t));
}

}

}

#endif