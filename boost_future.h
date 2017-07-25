//
// Created by System Administrator on 23/07/2017.
//

#ifndef STREAMS_BACKEND_ASIO_FUTURE_H
#define STREAMS_BACKEND_ASIO_FUTURE_H

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#include <boost/thread.hpp>
#include <memory>

class use_boost_future_t {};

/// @brief A special value, similiar to std::nothrow.
constexpr use_boost_future_t use_boost_future;

//
//
//namespace detail {
//
///// @brief Completion handler to adapt a boost::promise as a completion
/////        handler.
//    template <typename T>
//    class boost_promise_handler;
//
///// @brief Completion handler to adapt a void boost::promise as a completion
/////        handler.
//    template <>
//    class boost_promise_handler<void>
//    {
//    public:
//        /// @brief Construct from use_unique_future special value.
//        explicit boost_promise_handler(use_boost_future_t)
//                : promise_(std::make_shared<boost::promise<void> >())
//        {}
//
//        void operator()(const boost::system::error_code& error)
//        {
//            // On error, convert the error code into an exception and set it on
//            // the promise.
//            if (error)
//                promise_->set_exception(
//                        std::make_exception_ptr(boost::system::system_error(error)));
//                // Otherwise, set the value.
//            else
//                promise_->set_value();
//        }
//
////private:
//        std::shared_ptr<boost::promise<void> > promise_;
//    };
//
//// Ensure any exceptions thrown from the handler are propagated back to the
//// caller via the future.
//    template <typename Function, typename T>
//    void asio_handler_invoke(
//            Function function,
//            boost_promise_handler<T>* handler)
//    {
//        // Guarantee the promise lives for the duration of the function call.
//        std::shared_ptr<boost::promise<T> > promise(handler->promise_);
//        try
//        {
//            function();
//        }
//        catch (...)
//        {
//            promise->set_exception(std::current_exception());
//        }
//    }
//
//} // namespace detail
//
//namespace boost {
//    namespace asio {
//
///// @brief Handler type specialization for use_unique_future.
//        template <typename ReturnType>
//        struct handler_type<
//                use_boost_future_t,
//                ReturnType(boost::system::error_code)>
//        {
//            typedef ::detail::boost_promise_handler<void> type;
//        };
//
///// @brief Handler traits specialization for unique_promise_handler.
//        template <typename T>
//        class async_result< ::detail::boost_promise_handler<T> >
//        {
//        public:
//            // The initiating function will return a boost::unique_future.
//            typedef boost::future<T> type;
//
//            // Constructor creates a new promise for the async operation, and obtains the
//            // corresponding future.
//            explicit async_result(::detail::boost_promise_handler<T>& handler)
//            {
//                value_ = handler.promise_->get_future();
//            }
//
//            // Obtain the future to be returned from the initiating function.
//            type get() { return std::move(value_); }
//
//        private:
//            type value_;
//        };
//
//    } // namespace asio
//} // namespace boost



namespace detail {

/// @brief Completion handler to adapt a boost::promise as a completion
///        handler.
    template <typename T>
    class boost_promise_handler;

/// @brief Completion handler to adapt a void boost::promise as a completion
///        handler.
    template <>
    class boost_promise_handler<boost::system::error_code>
    {
    public:
        /// @brief Construct from use_unique_future special value.
        explicit boost_promise_handler(use_boost_future_t)
                : promise_(std::make_shared<boost::promise<boost::system::error_code>>())
        {}

        void operator()(const boost::system::error_code& error)
        {
            promise_->set_value(error);
        }

//private:
        std::shared_ptr<boost::promise<boost::system::error_code> > promise_;
    };

// Ensure any exceptions thrown from the handler are propagated back to the
// caller via the future.
    template <typename Function, typename T>
    void asio_handler_invoke(
            Function function,
            boost_promise_handler<T>* handler)
    {
        // Guarantee the promise lives for the duration of the function call.
        std::shared_ptr<boost::promise<T> > promise(handler->promise_);
        try
        {
            function();
        }
        catch (...)
        {
            promise->set_exception(std::current_exception());
        }
    }

} // namespace detail

namespace boost {
    namespace asio {

        template <typename ReturnType>
        struct handler_type<
                use_boost_future_t,
                ReturnType(boost::system::error_code)>
        {
            typedef ::detail::boost_promise_handler<boost::system::error_code> type;
        };

        template <typename T>
        class async_result< ::detail::boost_promise_handler<T> >
        {
        public:
            // The initiating function will return a boost::unique_future.
            typedef boost::future<T> type;

            // Constructor creates a new promise for the async operation, and obtains the
            // corresponding future.
            explicit async_result(::detail::boost_promise_handler<T>& handler)
            {
                value_ = handler.promise_->get_future();
            }

            // Obtain the future to be returned from the initiating function.
            type get() { return std::move(value_); }

        private:
            type value_;
        };

    } // namespace asio
} // namespace boost

#endif //STREAMS_BACKEND_ASIO_FUTURE_H
