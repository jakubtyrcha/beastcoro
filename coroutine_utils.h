//
// Created by System Administrator on 22/07/2017.
//

#ifndef STREAMS_BACKEND_COROUTINES_H
#define STREAMS_BACKEND_COROUTINES_H

#include <experimental/coroutine>
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#include <boost/thread.hpp>

template <typename... Args>
struct std::experimental::coroutine_traits<boost::future<void>, Args...> {
    struct promise_type {
        boost::promise<void> p;
        auto get_return_object() { return p.get_future(); }
        auto initial_suspend() { return std::experimental::suspend_never{}; }
        auto final_suspend() { return std::experimental::suspend_never{}; }
        void set_exception(std::exception_ptr e) { p.set_exception(std::move(e)); }
        void return_void() { p.set_value(); }
        void unhandled_exception() { std::terminate(); }
    };
};

template <typename R, typename... Args>
struct std::experimental::coroutine_traits<boost::future<R>, Args...> {
    struct promise_type {
        boost::promise<R> p;
        auto get_return_object() { return p.get_future(); }
        auto initial_suspend() { return std::experimental::suspend_never{}; }
        auto final_suspend() { return std::experimental::suspend_never{}; }
        void set_exception(std::exception_ptr e) { p.set_exception(std::move(e)); }
        template <typename U> void return_value(U &&u) {
            p.set_value(std::forward<U>(u));
        }
        void unhandled_exception() { std::terminate(); }
    };
};

auto operator co_await(boost::future<void> &&f) {
    struct Awaiter {
        boost::future<void> &&input;
        boost::future<void> output;
        bool await_ready() { return false; }
        auto await_resume() { return output.get(); }
        void await_suspend(std::experimental::coroutine_handle<> coro) {
            input.then([this, coro](auto result_future) {
                this->output = std::move(result_future);
                // workaround for compiler bug? complains about const type captured (coro)
                static_cast<std::experimental::coroutine_handle<>>(coro).resume();
            });
        }
    };
    return Awaiter{static_cast<boost::future<void>&&>(f)};
}

template <typename R> auto operator co_await(boost::future<R> &&f) {
    struct Awaiter {
        boost::future<R> &&input;
        boost::future<R> output;
        bool await_ready() { return false; }
        auto await_resume() { return output.get(); }
        void await_suspend(std::experimental::coroutine_handle<> coro) {
            input.then([this, coro](auto result_future) {
                this->output = std::move(result_future);
                // workaround for compiler bug? complains about const type captured (coro)
                static_cast<std::experimental::coroutine_handle<>>(coro).resume();
            });
        }
    };
    return Awaiter{static_cast<boost::future<R>&&>(f)};
}

template<typename T>
struct generator {
    struct promise_type;
    using handle = std::experimental::coroutine_handle<promise_type>;
    struct promise_type {
        T current_value;
        static auto get_return_object_on_allocation_failure() { return generator{nullptr}; }
        auto get_return_object() { return generator{handle::from_promise(*this)}; }
        auto initial_suspend() { return std::experimental::suspend_always{}; }
        auto final_suspend() { return std::experimental::suspend_always{}; }
        void unhandled_exception() { std::terminate(); }
        void return_void() {}
        auto yield_value(T value) {
            current_value = value;
            return std::experimental::suspend_always{};
        }
    };

    // end iterator should never call any methods!
    struct iterator {
        using gen_type = generator<T>;

        iterator(generator<T> * gen) : generator_(gen) {
            if(generator_)
                (*generator_)();
        }

        iterator& operator ++() {
            (*generator_)();
            return *this;
        }

        T operator *() {
            return generator_->get();
        }

        bool operator != (const iterator & rhs) {
            return static_cast<bool>(*generator_);
        }
    protected:
        generator<T> * generator_;
    };

    bool move_next() { return coro ? (coro.resume(), !coro.done()) : false; }
    int get() { return coro.promise().current_value; }
    generator(generator const&) = delete;
    generator(generator && rhs) : coro(rhs.coro) { rhs.coro = nullptr; }
    ~generator() { if (coro) coro.destroy(); }

    bool operator () () {
        return move_next();
    }

    explicit operator bool () const {
        return !coro.done();
    }

    iterator begin() {
        return iterator(this);
    }

    iterator end() {
        return iterator(nullptr);
    }

private:
    generator(handle h) : coro(h) {}
    handle coro;
};

#endif //STREAMS_BACKEND_COROUTINES_H
