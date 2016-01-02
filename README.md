#NylonSock

[![Build Status](https://travis-ci.org/wileyyugioh/NylonSock.svg)](https://travis-ci.org/wileyyugioh/NylonSock)


A C++ interface for the C UNIX Sockets Library

The function names should remain unchanged resulting in easy understanding and porting.

#Documentation (Incomplete!)

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
