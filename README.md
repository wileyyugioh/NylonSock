# NylonSock

[![Build Status](https://travis-ci.org/wileyyugioh/NylonSock.svg)](https://travis-ci.org/wileyyugioh/NylonSock)


A C++ interface for the C UNIX Sockets Library

The function names should remain unchanged resulting in easy understanding and porting.

# Building

Use CMake

Tested on OSX and Windows, but should work on Linux as well

# Including

```
#include <NylonSock.hpp>
```

# Documentation

Creating a server is easy, just do

```
Server<CLIENTSOCK> serv {PORTNUM}

serv.onConnect([](CLIENTSOCK& sock)
{
    sock.emit(“hallo”, {“ok”})
});

serv.start();
```

Where CLIENTSOCK is any class inherited by NylonSock::ClientSocket.

```
class CLIENTSOCK : public NylonSock::ClientSocket<CLIENTSOCK>
{
    public:
        CLIENTSOCK(NylonSock::Socket&& sock) : ClientSocket(std::move(sock) ) {}
}
```

What the code above does is sends the message “ok” under the event name "Hallo" when a client connects.

Clients use the same format as servers.

```
class CLIENTSOCK : public NylonSock::ClientSocket<CLIENTSOCK>
{
    public:
        CLIENTSOCK(NylonSock::Socket&& sock) : ClientSocket(std::move(sock) ) {}
}

NylonSock::Client<CLIENTSOCK> client{"MYIP", PORTNUM};
client.on("hallo", [](SockData data, CLIENTSOCK& ps)
    {
        std::cout << data.getRaw() << std::endl;
    });

client.start();
```

When a client receives the hallo event from a server, it will print out “ok.”

# More Documentation!

Here's a quick cheatsheet of the various functionality baked in

Server

```
// initialize the server (but don't start it)
Server<CLIENTSOCK> server {PORTNUM}

// this is called whenever a socket connects
server.onConnect([&server](CLIENTSOCK& socket)
{
    // this is sent to every client connected to the server
    server.emit("message", "test");

    // this is sent to one client
    socket.emit("message", "specific test");

    // this is called when the socket receives an event
    socket.on("secret message", [](SockData data, CLIENTSOCK& socket)
    {
        std::string message = data;
        std::cout << message << std::endl;
    });

    // on has a second flavor as well
    socket.on("no message", [](CLIENTSOCK& socket)
    {
        std::cout << "mo problems" << std::endl;
    });

    // this is called when a socket disconnects
    socket.on("disconnect", [](CLIENTSOCK& socket)
    {
        std::cout << "Goodbye" << std::endl;
    });
});

// spins up a thread running a polling loop
server.start();
```

Client
```
// initialize the client (but don't start it)
NylonSock::Client<CLIENTSOCK> client{MYIP, PORTNUM};

// send to the server (not all clients)
client.emit("message", "server test");

// this is called when the event is received
client.on("secret message", [](SockData data, CLIENTSOCK& socket)
{
    std::string message = data;
    std::cout << message << std::endl;
});

// client.on comes in a a second flavor as well
client.on("cool event", [](CLIENTSOCK& socket)
{
    std::cout << "Rad" << std::endl;
});

// spins up a thread running a polling loop
client.start();
```

# Running the Test Server / Client

```
./TestServer


In another shell...
./TestClient localhost
or
./TestClient XXX.XXX.XXX.XXX (Local IP Address of the TestServer Computer)
```

# The Gritty

The Client class and the ClientSocket class have the same functions.

## \*.on(EventName, Func);

The on function takes in a string EventName for the event to act upon and a function Func, which is void and takes in a SockData for the first parameter and for the second a reference to the class that is calling it, be it a ClientSocket, a Client, or anything inherited from it.

An example function to pass to \*.on is this:

```
void myfunc(SockData data, MYCUSTOMCLIENTSOCKETCLASS& mccsc)
{
    mccsc.data = data;
}
```

You can even pass in lambdas that capture!

```
int value;
client.on("printValue", [value](SockData data, MYCUSTOMCLIENTSOCKETCLASS& mccsc)
{
    std::cout << value << std::endl;
});
```

Please note that the function is void, and the first parameter is a SockData, and the second is the ClientSocket or inherited class that you passed to the server or class in the form of a template.

It should be noted that there is a special event called "disconnect", which is called when a socket is disconnected.

```
client.on("disconnect", []()
{
    std::cout << "Goodbye" << std::endl;
});
```

## \*.emit(EventName, Data);

The emit function takes in a string EventName and a SockData class for the second parameter. The function, when called, will send the data encapsulated in the SockData class to any clients, and call their 'on' function with a matching EventName.

```
client.emit("greetings", {"Hello World!"}); // this is sent to the server
server.emit("this is sent", {"to all clients!"}); // this is sent to every client
```

## \*.start()

Only for Client class and Server Class. Starts the socket's main thread to receive data.

## \*.stop()

Only for Client class and Server Class. Stops the socket's main thread. Automatically called upon destruction.

## Client Class

Takes in as a template a ClientSocket class or a class inherited from ClientSocket.

On destruction, stop is automatically called.

Constructor:

**Client(const std::string& ip, int port)**

**Client(const std::string& ip, const std::string& port)**

```
class CustomClient : public NylonSock::ClientSocket<CustomClient>
{
public:
    CustomClient(NylonSock::Socket&& sock) : ClientSocket(std::move(sock) ) {}
};

NylonSock::Client<CustomClient> client {IP_ADDRESS, PORT_NUM};
```

### Functions

**void on(std::string msgstr, NylonSock::SockFunc func)**

**void emit(std::string msgstr, NylonSock::SockData data)**

**bool getDestroy():**

When a client disconnects from a server, its socket is destroyed. This can be used to check if the client's socket is destroyed.

**void start()**

**void stop()**

**bool status():**

Returns True if the client's main thread has been started.

**T& get():**

Returns a reference to the ClientSocket or inherited class you passed in.

## Server Class

Takes in a ClientSocket as a template.

```
class CustomClient : public NylonSock::ClientSocket<CustomClient>
{
public:
    CustomClient(NylonSock::Socket&& sock) : ClientSocket(std::move(sock) ) {};
};

NylonSock::Server<CustomClient> server {PORT_NUM};
```

### Functions

**onConnect(std::function<void (ClientSocket&)>):**

Is called upon a new client connecting to the server.

Generally, this means the code should be used something like this:

```
server.onConnect([](TestClientSocket& sock)
{
    sock.on("greeting", [](SockData data, TestClientSock& sock)
    {
        std::cout << data.getRaw() << std::endl;
    });

    sock.on("disconnect", [](TestClientSock& sock)
    {
        std::cout << "Goodbye" << std::endl;
    });

});
```

**void emit(std::string event_name, SockData data):**

Sends data under event_name to ALL clients

**void start()**

**void stop()**

**bool status():**

Returns True if the server's main thread has been started.

## ClientSocket class

Constructor:

Takes in a NylonSock::Socket&& as an argument.

The template takes in a ClientSocket class or any inherited class.

Please use CRTP.

```
class CustomClient : public NylonSock::ClientSocket<CustomClient>
{
public:
    CustomClient(NylonSock::Socket&& sock) : ClientSocket(std::move(sock)) {}
};
```

### Functions

**void on(std::string msgstr, NylonSock::SockFunc func)**

**void emit(std::string msgstr, NylonSock::SockData data)**

**bool getDestroy()**

## SockData class

Constructor:

It uses templates to take in pretty much any primitive value and convert it to a string using stringstreams.

```
SockData {0};
SockData {1.1};
SockData {"Hello World!"};
```

Getting Values:

SockData uses a template operator to cast values back into their respective types. This also uses stringstreams for conversion.

```
SockData sockdata{1.56};

int i = sockdata; // 1
double d = sockdata; // 1.56
float f = sockdata; // 1.56
```

# EVERYTHING BEYOND THIS IS LOWER LEVEL

The class this library is built upon is the Sockets class. It takes in the typical things getaddrinfo takes in: node, service, and an address of an addrinfo hint structure.

```
addrinfo hints = {0};
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;

NylonSock::Socket sock{nullptr, "3490", &hints}
```

The class will safely close any socket and free the addrinfo upon destruction. In order to access information an addrinfo will normally contain, just use the -> operator or use the get method.

```
sock->ai_family is the same as sock.get()->ai_family
```

To get the size of the addrinfo the class has, use the size method

```
sock.size()
```

In order to get the port number, use the port method

```
sock.port()
```

The library also possesses its own functions with the same naming convention as the C UNIX ones. The functions are very similar. However, they throw an exception class, NylonSock::Error upon failure instead of returning -1.

The Error class inherits from std::runtime_exception.

The library also favors the use of std::string instead of const char\*.

The library also has its own class encapsulating fd_set called FD_Set. It has the methods set, isset and clr which take in a reference to a Socket class.

```
FD_Set f_set;
f_set.set(sock)
assert(f_set.isset(sock) )
f_set.clr(sock)
```

It also has the methods zero, and getMax. zero clears the fd_set and getMax returns the highest port currently in fd_set.

```
f_set.zero()
f_set.getMax()
```

A function select, is given, which takes in a FD_Set and a timeval struct and returns a vector of FD_Set. A class TimeVal which converts milliseconds to a timeval struct is given. An example would be

```
auto sel = select(f_set, TimeVal{1000});

//read
sel[0]

//write
sel[1]

//except
sel[2]
```
