/////////////////////////////////////////////////////////////////////////////
//                         Single Threaded Networking
//
// This file is distributed under the MIT License. See the LICENSE file
// for details.
/////////////////////////////////////////////////////////////////////////////
// code base from Nick Sumner github repo: https://github.com/nsumner/web-socket-networking"


#include "Client.h"


#include <deque>
#include <sstream>

using networking::Client;

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace networking {


class Client::ClientImpl {
public:
    ClientImpl(std::string_view address, std::string_view port)
        : hostAddress{address.data(), address.size()} {
        boost::asio::ip::tcp::resolver resolver{ioService};
        connect(resolver.resolve(address, port));
    }

    void disconnect();

    void reportError(std::string_view message) const;

    void update() { ioService.poll(); }

    void send(std::string message);

    std::ostringstream& getIncomingStream() { return incomingMessage; }

    bool isClosed() const { return closed; }

private:

    void connect(boost::asio::ip::tcp::resolver::iterator endpoint);

    void handshake();

    void readMessage();

    bool closed = false;
    boost::asio::io_service ioService{};
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> websocket{ioService};
    std::string hostAddress;
    boost::beast::multi_buffer readBuffer;
    std::ostringstream incomingMessage;
    std::deque<std::string> writeBuffer;
};


}


void
Client::ClientImpl::disconnect() {
    closed = true;
    websocket.async_close(boost::beast::websocket::close_code::normal,
        [] (auto errorCode) {
            // Swallow errors while closing.
        });
}


void
Client::ClientImpl::connect(boost::asio::ip::tcp::resolver::iterator endpoint) {
    boost::asio::async_connect(websocket.next_layer(), endpoint,
        [this] (auto errorCode, auto) {
            if (!errorCode) {
                this->handshake();
            } else {
                reportError("Unable to connect.");
            }
        });
}


void
Client::ClientImpl::handshake() {
    websocket.async_handshake(hostAddress, "/",
        [this] (auto errorCode) {
            if (!errorCode) {
                this->readMessage();
            } else {
                reportError("Unable to handshake.");
            }
        });
}


void
Client::ClientImpl::readMessage() {
    websocket.async_read(readBuffer,
        [this] (auto errorCode, std::size_t size) {
            if (!errorCode) {
                if (size > 0) {
                    auto message = boost::beast::buffers_to_string(readBuffer.data());
                    incomingMessage.write(message.c_str(), message.size());
                    readBuffer.consume(readBuffer.size());
                    this->readMessage();
                }
            } else {
                reportError("Unable to read.");
                this->disconnect();
            }
        });
}


void
Client::ClientImpl::send(std::string message) {
    writeBuffer.emplace_back(std::move(message));
    websocket.async_write(boost::asio::buffer(writeBuffer.back()),
        [this] (auto errorCode, std::size_t /*size*/) {
            if (!errorCode) {
                writeBuffer.pop_front();
            } else {
                reportError("Unable to write.");
                disconnect();
            }
        });
}

/////////////////////////////////////////////////////////////////////////////
// Core Client
/////////////////////////////////////////////////////////////////////////////


Client::Client(std::string_view address, std::string_view port)
    : impl{std::make_unique<ClientImpl>(address, port)}
        { }


Client::~Client() = default;


void
Client::ClientImpl::reportError(std::string_view /*message*/) const {
    // Swallow errors by default.
    // This can still provide a useful entrypoint for debugging.
}


void
Client::update() {
    impl->update();
}


std::string
Client::receive() {
    auto& stream = impl->getIncomingStream();
    auto result = stream.str();
    stream.str(std::string{});
    stream.clear();
    return result;
}


void
Client::send(std::string message) {
    if (message.empty()) {
        return;
    }
    impl->send(std::move(message));
}


bool
Client::isDisconnected() const noexcept {
    return impl->isClosed();
}
