//
// Copyright (c) 2017 Jakub Tyrcha (jakub tyrcha at gmail com)
//
// Modified beast/example/http-server-small/http_server_small.cpp to work with coroutines
//

#include <boost/asio.hpp>
#include <chrono>
#include <beast/http.hpp>
#include <beast/websocket.hpp>
#include <beast/core.hpp>
#include <beast/http.hpp>
#include <iostream>
#include "coroutine_utils.h"
#include "boost_future.h"

namespace asio = boost::asio;
using namespace std::chrono;
namespace asio_ip = asio::ip;
using asio_tcp = asio::ip::tcp;
namespace http = beast::http;

namespace my_program_state
{
    std::size_t
    request_count()
    {
        static std::size_t count = 0;
        return ++count;
    }

    std::time_t
    now()
    {
        return std::time(0);
    }
}

class http_connection
{
public:
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::dynamic_body> request_;

    // The response message.
    http::response<http::dynamic_body> response_;

    void process_request()
    {
        response_.version = 11;
        response_.set(http::field::connection, "close");

        switch(request_.method())
        {
            case http::verb::get:
                response_.result(http::status::ok);
                response_.set(http::field::server, "Beast");
                create_response();
                break;

            default:
                // We return responses indicating an error if
                // we do not recognize the request method.
                response_.result(http::status::bad_request);
                response_.set(http::field::content_type, "text/plain");
                beast::ostream(response_.body)
                        << "Invalid request-method '"
                        << request_.method_string().to_string()
                        << "'";
                break;
        }
    }

    void create_response()
    {
        if(request_.target() == "/count")
        {
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body)
                    << "<html>\n"
                    <<  "<head><title>Request count</title></head>\n"
                    <<  "<body>\n"
                    <<  "<h1>Request count</h1>\n"
                    <<  "<p>There have been "
                    <<  my_program_state::request_count()
                    <<  " requests so far.</p>\n"
                    <<  "</body>\n"
                    <<  "</html>\n";
        }
        else if(request_.target() == "/time")
        {
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body)
                    <<  "<html>\n"
                    <<  "<head><title>Current time</title></head>\n"
                    <<  "<body>\n"
                    <<  "<h1>Current time</h1>\n"
                    <<  "<p>The current time is "
                    <<  my_program_state::now()
                    <<  " seconds since the epoch.</p>\n"
                    <<  "</body>\n"
                    <<  "</html>\n";
        }
        else
        {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body) << "File not found\r\n";
        }
    }
};

boost::future<void> run_session(asio_tcp::socket socket)
{
    http_connection connection;
    asio::basic_waitable_timer<std::chrono::steady_clock> deadline{
            socket.get_io_service(), 60s};

    deadline.async_wait(
            [&socket](beast::error_code ec)
            {
                if(!ec)
                {
                    socket.close(ec);
                }
            });

    boost::system::error_code ec;
    ec = co_await http::async_read(socket, connection.buffer_, connection.request_, use_boost_future);

    if(!ec)
    {
        connection.process_request();

        connection.response_.set(http::field::content_length, connection.response_.body.size());

        ec = co_await http::async_write(socket, connection.response_, use_boost_future);

        deadline.cancel();
        socket.shutdown(asio_tcp::socket::shutdown_send, ec);
    }
}

boost::future<void> run_tcp_server_coro(asio::io_service & ios) {
    auto address = asio_ip::address::from_string("0.0.0.0");
    unsigned short port = 80;
    asio_tcp::acceptor acceptor{ios, {address, port}};
    asio_tcp::socket socket{ios};

    while(true) {
        auto ec = co_await acceptor.async_accept(socket, use_boost_future);

        if(!ec)
        {
            run_session(std::move(socket));
        }
        else
        {
            std::cout << ec.message() << std::endl;
        }
    }
};


int main() {
    try
    {
        boost::asio::io_service ios{1};

        run_tcp_server_coro(ios);

        auto work = boost::asio::io_service::work(ios);
        ios.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}