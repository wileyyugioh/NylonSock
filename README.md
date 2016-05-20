#NylonSock

[![Build Status](https://travis-ci.org/wileyyugioh/NylonSock.svg)](https://travis-ci.org/wileyyugioh/NylonSock)


A C++ interface for the C UNIX Sockets Library

The function names should remain unchanged resulting in easy understanding and porting.

#Documentation (Incomplete!)

Creating a server is easy, just do

```
Server<CLIENTSOCK> serv {PORTNUM}

serv.onConnect([](CLIENTSOCK& sock)
{
	sock.emit(“Hallo”, {“ok”})
});

serv.start();
```

Where CLIENTSOCK is any class inherited by NylonSock::ClientSocket.

```
    class CLIENTSOCK : public NylonSock::ClientSocket<CLIENTSOCK>
    {
        public:
            CLIENTSOCK(NylonSock::Socket sock) : ClientSocket(sock, this)
            {
            }
    }
```

What the code above does is sends the message “ok” under the event name "Hallo" when a client connects.

Clients are inherited by the NylonSock::Client class and pass in itself as a template.

```
    class CUSTCLIENT : public NylonSock::Client<CUSTCLIENT>
    {
        public:
            CUSTCLIENT(std::string ip, int port) : Client(ip, port)
            {
            }
    }

    CUSTCLIENT client{"MYIP", PORTNUM};
    client.on("hallo", [](SockData data, InClient& ps)
        {
            std::cout << data.getRaw() << std::endl;
        });

    client.start();
```

When a client receives the hallo event from a server, it will print out “ok.”

#The Gritty

The Client class and the ClientSocket class have the same functions.

##*.on(EventName, Func);

The on function takes in a string EventName for the event to act upon and a function Func, which is void and takes in a SockData for the first parameter and for the second a reference to the class that is calling it, be it a ClientSocket, a Client, or anything inherited from it.

##*.emit(EventName, Data);

The emit function takes in a string EventName and a SockData class for the second parameter. The function, when called, will send the data encapsulated in the SockData class to any clients, and call their 'on' function with a matching EventName.

##*.start()

Only for Client class and Server Class. Starts the server's updating functions.

##*.stop()

Only for Client class and Server Class. Stops the updating function. Automatically called upon destruction.

##Client Class

Uses CRTP

On destruction, stop is automatically called.

###Functions

void on

void emit

bool getDestroy():

    When a server disconnects from a client, it destroy's the client's socket. This can be used to check if the client's socket is destroyed.

void start

void stop

bool status():

    Returns the status of the client. If true, then start has been called. If false, then the client has stopped.

##Server Class

Takes in a ClientSocket as a template.

Please use CRTP

###Functions

onConnect:

    takes in as a parameter a function that takes in as an argument a reference to the ClientSocket that is passed into the template. 

void start

void stop

bool status

UsrSock& getUsrSock(unsigned int pos):

    All ClientSockets are stored in a vector in the Server class. You can access a particular client if you want by the pos, and this function will return a reference to it.

##ClientSocket class

void on

void emit

bool getDestroy

#EVERYTHING PASSED THIS IS LOWER LEVEL

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

The library also possesses its own functions with the same naming convention as the C UNIX ones. It is very similar, however, it throws an exception class, NylonSock::Error upon failure. 

The Error class inherits from std::runtime_exception.

The library also favors the use of std::string instead of const char*.

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

A function select, is given, which takes in a FD_Set and a timeval struct and returns a vector of fd_set. A class TimeVal which converts milliseconds to a timeval struct is given. An example would be TOBECONTINUED


Todo:
Documentation

Windows support
