#include "Server.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "Ambassador.h"



using namespace std::string_literals;
using networking::Message;
using networking::Server;
using networking::ServerImpl;
using networking::ServerImplDeleter;
using namespace ambassador;

/////////////////////////////////////////////////////////////////////////////
// code base from Nick Sumner github repo: https://github.com/nsumner/web-socket-networking"
/////////////////////////////////////////////////////////////////////////////

namespace networking {


/////////////////////////////////////////////////////////////////////////////
// Private Server API
/////////////////////////////////////////////////////////////////////////////


class Channel;


class ServerImpl {
public:

  ServerImpl(Server& server, unsigned short port, std::string httpMessage)
   : server{server},
     endpoint{boost::asio::ip::tcp::v4(), port},
     acceptor{ioContext, endpoint},
     httpMessage{std::move(httpMessage)},

     // Initialize the Ambassador object in the Server constructor
     writeQ{std::make_shared<msgQImpl>(SERVER_QUEUE, true, false)},
     readQ{std::make_shared<msgQImpl>(EVENT_LOOP_QUEUE, false, true)},
     loopAmbassador{writeQ, readQ}
  {
    listenForConnections();
  }

  void listenForConnections();
  void registerChannel(Channel& channel);
  void reportError(std::string_view message);

  // To do: place in another place
  int generateUniqueGameInstanceID() {
    return GameInstanceId++;
  }

  int generateUniquePlayerID() {
    return PlayerId++;
  }

  using ChannelMap =
    std::unordered_map<Connection, std::shared_ptr<Channel>, ConnectionHash>;

  Server& server;
  const boost::asio::ip::tcp::endpoint endpoint;
  boost::asio::io_context ioContext{};
  boost::asio::ip::tcp::acceptor acceptor;
  boost::beast::http::string_body::value_type httpMessage;

  ChannelMap channels;
  std::shared_ptr<Channel> activeChannel;
  std::deque<Message> incoming;

  // Ambassador Object
  std::shared_ptr<msgQImpl> writeQ;
  std::shared_ptr<msgQImpl> readQ;
  Ambassador loopAmbassador;

  std::unordered_map<Connection, int, ConnectionHash> socketToGameInstanceID; // Mapping from sockets to gameInstance IDs
  std::unordered_map<Connection, int, ConnectionHash> socketToPlayerID; // Mapping from sockets to player IDs

private:
  int GameInstanceId = 1;
  int PlayerId = 1;
};

/////////////////////////////////////////////////////////////////////////////
// Channels (connections private to the implementation)
/////////////////////////////////////////////////////////////////////////////


class Channel : public std::enable_shared_from_this<Channel> {
public:
  Channel(boost::asio::ip::tcp::socket socket, ServerImpl& serverImpl)
    : connection{reinterpret_cast<uintptr_t>(this)},
      serverImpl{serverImpl},
      websocket{std::move(socket)},
      readBuffer{serverImpl.incoming}
      { }

  void start(boost::beast::http::request<boost::beast::http::string_body>& request);
  void send(std::string outgoing);
  void disconnect();

  [[nodiscard]] Connection getConnection() const noexcept { return connection; }

private:
  void readMessage();
  void afterWrite(std::error_code errorCode, std::size_t size);

  bool disconnected = false;
  Connection connection;
  ServerImpl &serverImpl;

  boost::beast::flat_buffer streamBuf{};
  boost::beast::websocket::stream<boost::asio::ip::tcp::socket> websocket;

  std::deque<Message> &readBuffer;
  std::deque<std::string> writeBuffer;
};

}

using networking::Channel;


void
Channel::start(boost::beast::http::request<boost::beast::http::string_body>& request) {
  auto self = shared_from_this();
  websocket.async_accept(request,
    [this, self] (std::error_code errorCode) {
      if (!errorCode) {
        serverImpl.registerChannel(*this);
        self->readMessage();
      } else {
        serverImpl.server.disconnect(connection);
      }
    });
}


void
Channel::disconnect() {
  disconnected = true;
  boost::beast::error_code ec;
  websocket.close(boost::beast::websocket::close_reason{}, ec);
}


void
Channel::send(std::string outgoing) {
  if (outgoing.empty()) {
    return;
  }
  writeBuffer.push_back(std::move(outgoing));

  if (1 < writeBuffer.size()) {
    // Note, multiple writes will be chained within asio via `continueSending`,
    // so that callback should be used instead of directly invoking async_write
    // again.
    return;
  }

  websocket.async_write(boost::asio::buffer(writeBuffer.front()),
    [this, self = shared_from_this()] (auto errorCode, std::size_t size) {
      afterWrite(errorCode, size);
    });
}


void
Channel::afterWrite(std::error_code errorCode, std::size_t /*size*/) {
  if (errorCode) {
    if (!disconnected) {
      serverImpl.server.disconnect(connection);
    }
    return;
  }

  writeBuffer.pop_front();

  // Continue asynchronously processing any further messages that have been
  // sent.
  if (!writeBuffer.empty()) {
    websocket.async_write(boost::asio::buffer(writeBuffer.front()),
      [this, self = shared_from_this()] (auto errorCode, std::size_t size) {
        afterWrite(errorCode, size);
      });
  }
}


void
Channel::readMessage() {
  auto self = shared_from_this();
  websocket.async_read(streamBuf,
    [this, self] (auto errorCode, std::size_t /*size*/) {
      if (!errorCode) {
        auto message = boost::beast::buffers_to_string(streamBuf.data());
        readBuffer.push_back({connection, std::move(message)});
        streamBuf.consume(streamBuf.size());
        this->readMessage();
      } else if (!disconnected) {
        serverImpl.server.disconnect(connection);
      }
    });
}


////////////////////////////////////////////////////////////////////////////////
// Basic HTTP Request Handling
////////////////////////////////////////////////////////////////////////////////

class RequestHandler;

class HTTPSession : public std::enable_shared_from_this<HTTPSession> {
public:
  explicit HTTPSession(ServerImpl& serverImpl)
    : serverImpl{serverImpl},
      socket{serverImpl.ioContext}
      { }

  void start();
  void handleRequest(Ambassador & loopAmbassador);

  boost::asio::ip::tcp::socket & getSocket() { return socket; }

private:
  ServerImpl &serverImpl;
  boost::asio::ip::tcp::socket socket;
  boost::beast::flat_buffer streamBuf{};
  boost::beast::http::request<boost::beast::http::string_body> request;
  static std::unordered_map<std::string, std::shared_ptr<RequestHandler>> requestHandlers;
};

class RequestHandler {
public:
    virtual void handle(boost::beast::http::request<boost::beast::http::string_body>& request, Ambassador& loopAmbassador, ServerImpl& serverImpl) = 0;
};

class CreateGameRequestHandler : public RequestHandler {
public:
  void handle(boost::beast::http::request<boost::beast::http::string_body>& request, Ambassador& loopAmbassador, ServerImpl& serverImpl) {
    std::string input;
    std::cout << "Please enter a game name: " << '\n';
    std::cin >> input;
    std::cout<< "Game name: " << input << '\n';

    // Generate GameInstanceIDs
    int gameInstanceId = serverImpl.generateUniqueGameInstanceID();
    serverImpl.socketToGameInstanceID[serverImpl.activeChannel->getConnection()] = gameInstanceId;

    // store data in a Response type
    Response newRes;
    newRes.setType(msgType::GAME_INIT);
    newRes.setAttr("val", input);
    newRes.setAttr("gameInstanceId", std::to_string(gameInstanceId));
    loopAmbassador.sendMsg(newRes);
    std::cout << "create game..." << '\n';
  }
};

class JoinGameRequestHandler : public RequestHandler{
public:
  void handle(boost::beast::http::request<boost::beast::http::string_body>& request, Ambassador& loopAmbassador, ServerImpl& serverImpl){
    std::string input;
    std::cout << "Please enter your player Id and game Id: ";
    std::cin >> input;
    std::cout << "User input: " << input << '\n'; // TODO: convert user input into a struct/json

    // Generate playerIds
    int playerId = serverImpl.generateUniquePlayerID();
    serverImpl.socketToPlayerID[serverImpl.activeChannel->getConnection()] = playerId;

    Response newRes;
    newRes.setType(msgType::PLAYER_JOIN);
    newRes.setAttr("val", input);
    newRes.setAttr("playerId", std::to_string(playerId));
    loopAmbassador.sendMsg(newRes);
    std::cout << "join game..." << '\n';
  }
};

class ConfigureGameRequestHandler : public RequestHandler{
public:
  void handle(boost::beast::http::request<boost::beast::http::string_body>& request, Ambassador& loopAmbassador, ServerImpl& serverImpl){
    std::string input;
    std::cout << "Please enter your game instance id and configurations: ";
    std::cin >> input;
    std::cout<< "User input: " << input << '\n';

    Response newRes;
    newRes.setType(msgType::CONFIG_RES);
    newRes.setAttr("val", input);
    loopAmbassador.sendMsg(newRes);
    std::cout << "configure game..." << '\n';
  }
};

// Initialize the static member
std::unordered_map<std::string, std::shared_ptr<RequestHandler>> HTTPSession::requestHandlers = {
    {"/create-game", std::make_shared<CreateGameRequestHandler>()},
    {"/join-game", std::make_shared<JoinGameRequestHandler>()},
    {"/configure-game", std::make_shared<ConfigureGameRequestHandler>()}
};


void
HTTPSession::start() {
  boost::beast::http::async_read(socket, streamBuf, request,
    [this, session = this->shared_from_this()]
    (std::error_code ec, std::size_t /*bytes*/) {
      if (ec) {
        serverImpl.reportError("Error reading from HTTP stream.");

      } else if (boost::beast::websocket::is_upgrade(request)) {
        auto channel = std::make_shared<Channel>(std::move(socket), serverImpl);
        channel->start(request);

      } else {
        session->handleRequest(serverImpl.loopAmbassador);
      }
    });
}


void
HTTPSession::handleRequest(Ambassador & loopAmbassador) {
  auto send = [this, session = this->shared_from_this()] (auto&& response) {
    using Response = typename std::decay<decltype(response)>::type;
    auto sharedResponse =
      std::make_shared<Response>(std::forward<decltype(response)>(response));

    boost::beast::http::async_write(socket, *sharedResponse,
      [this, session, sharedResponse] (std::error_code ec, std::size_t /*bytes*/) {
        if (ec) {
          session->serverImpl.reportError("Error writing to HTTP stream");
          socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
        } else if (sharedResponse->need_eof()) {
          // This signifies a deliberate close
          boost::system::error_code ec;
          socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
          if (ec) {
            session->serverImpl.reportError("Error closing HTTP stream");
          }
        } else {
          session->start();
        }
      });
  };

  auto const badRequest =
    [&request = this->request] (std::string_view why) {
    boost::beast::http::response<boost::beast::http::string_body> result {
      boost::beast::http::status::bad_request,
      request.version()
    };
    result.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    result.set(boost::beast::http::field::content_type, "text/html");
    result.keep_alive(request.keep_alive());
    result.body() = why;
    result.prepare_payload();
    return result;
  };


  if (auto method = request.method();
      method != boost::beast::http::verb::get
      && method != boost::beast::http::verb::head
      && method != boost::beast::http::verb::post) {
    send(badRequest("Unknown HTTP-method"));
  }
  else{
      auto shouldServeIndex = [] (auto target) { // checking legal requests
      std::string const index = "/index.html"s;
      constexpr auto npos = std::string_view::npos;
      // Adding more supported targets  [modified by yca452, ava32]
      return target == "/"
        || target == "/create-game"
        || target == "/configure-game"
        || target == "/input-game-configuration"
        || target == "/finalize-creating-game"
        || target == "/join-game"
        || (index.size() <= target.size()
          && target.compare(target.size() - index.size(), npos, index) == 0);
    };
    if (!shouldServeIndex(request.target())) {
        send(badRequest("Illegal request-target"));
    }

    boost::beast::http::string_body::value_type body = serverImpl.httpMessage;

    auto addResponseMetaData =
      [bodySize = body.size(), &request = this->request] (auto& response) {
      response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
      response.set(boost::beast::http::field::content_type, "text/html");
      response.content_length(bodySize);
      response.keep_alive(request.keep_alive());
    };

    if (request.method() == boost::beast::http::verb::head) {
      // Respond to HEAD
      boost::beast::http::response<boost::beast::http::empty_body> result {
        boost::beast::http::status::ok,
        request.version()
      };
      addResponseMetaData(result);
      send(std::move(result));

    }
    else if (request.method() == boost::beast::http::verb::get) {
      if (request.target() == "/") {
        boost::beast::http::response<boost::beast::http::string_body> result {
          std::piecewise_construct,
          std::make_tuple(std::move(body)),
          std::make_tuple(boost::beast::http::status::ok, request.version())
        };
        addResponseMetaData(result);
        send(std::move(result));
      }

      // Look for other endpoints in the map requestHandlers
      else{
        auto endpoint = requestHandlers.find(std::string(request.target()));
        if (endpoint != requestHandlers.end()) {
          endpoint->second->handle(request, loopAmbassador, serverImpl);
        } else {
            send(badRequest("Not Found 404"));
        }
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// Hidden Server implementation
/////////////////////////////////////////////////////////////////////////////


void
ServerImpl::listenForConnections() {
  auto session =
    std::make_shared<HTTPSession>(*this);

  acceptor.async_accept(session->getSocket(),
    [this, session] (auto errorCode) {
      if (!errorCode) {
        session->start();
      } else {
        reportError("Fatal error while accepting");
      }
      this->listenForConnections();
    });
}

/////////////////////////////////////////////////////////////////////////////
  // Handling mapping sockets to gameInstanceIDs and playerIdDs  [by ava32]
  /////////////////////////////////////////////////////////////////////////////

void
ServerImpl::registerChannel(Channel& channel) {
  auto connection = channel.getConnection();
  channels[connection] = channel.shared_from_this();
  server.connectionHandler->handleConnect(connection);
  activeChannel = channel.shared_from_this();
}


void
ServerImpl::reportError(std::string_view /*message*/) {
  // Swallow errors....
}

void
ServerImplDeleter::operator()(ServerImpl* serverImpl) {
  // NOTE: This is a custom deleter used to help hide the impl class. Thus
  // it must use a raw delete.
  // NOLINTNEXTLINE (cppcoreguidelines-owning-memory)
  delete serverImpl;
}


/////////////////////////////////////////////////////////////////////////////
// Core Server
/////////////////////////////////////////////////////////////////////////////

void
Server::update() {
  impl->ioContext.poll();
}


std::deque<Message>
Server::receive() {
  std::deque<Message> oldIncoming;
  std::swap(oldIncoming, impl->incoming);
  return oldIncoming;
}


void
Server::send(const std::deque<Message>& messages) {
  for (const auto& message : messages) {
    auto found = impl->channels.find(message.connection);
    if (impl->channels.end() != found) {
      found->second->send(message.text);
    }
  }
}

// std::shared_ptr<ambassador::Ambassador>
ambassador::Ambassador*
Server::getAmbassador() {
    ambassador::Ambassador* ptr = &(impl->loopAmbassador);
    return ptr;
}

/////////////////////////////////////////////////////////////////////////////
  // Handling removing the mappings for the disconnected socket  [by ava32]
  /////////////////////////////////////////////////////////////////////////////
void
Server::disconnect(Connection connection) {
  auto found = impl->channels.find(connection);
  if (impl->channels.end() != found) {
    connectionHandler->handleDisconnect(connection);

    // Remove the mappings for the disconnected socket
    auto gameInstanceIDIt = impl->socketToGameInstanceID.find(connection);
    if (gameInstanceIDIt != impl->socketToGameInstanceID.end()) {
      impl->socketToGameInstanceID.erase(gameInstanceIDIt);
    }

    auto playerIDIt = impl->socketToPlayerID.find(connection);
    if (playerIDIt != impl->socketToPlayerID.end()) {
      impl->socketToPlayerID.erase(playerIDIt);
    }

    found->second->disconnect();
    impl->channels.erase(found);
  }
}


std::unique_ptr<ServerImpl,ServerImplDeleter>
Server::buildImpl(Server& server,
                  unsigned short port,
                  std::string httpMessage) {
  // NOTE: We are using a custom deleter here so that the impl class can be
  // hidden within the source file rather than exposed in the header. Using
  // a custom deleter means that we need to use a raw `new` rather than using
  // `std::make_unique`.
  auto* impl = new ServerImpl(server, port, std::move(httpMessage));
  return std::unique_ptr<ServerImpl,ServerImplDeleter>(impl);
}



