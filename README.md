Distributed Fileserver in C
===========================
This is a distributed fileserver in C utilizing TCP. The project consists of a server and client, which allows the client to store and retrieve files spread across multiple servers. 

The servers can handle multiple requests from multiple clients. 

Setup
-----
A makefile has been provided to aid in compilation: 

```
$ make 
```
Next, create four directories: 

```
$ mkdir ./DFS1/ ./DFS2/ ./DFS3/ ./DFS4/ 
```


Run
-----

**1\.** Start the server and set the server's default directory and port:

```
./server /DFS1 10001 & 
./server /DFS2 10002 & 
./server /DFS3 10003 & 
./server /DFS4 10004 &
```

The second argument in starting the servers should correspond to theserver's port number specified in client configuration file `./dfc.conf`. 

**2\.** Start the client: 

```
$ ./client 
```

Upon sending a request, the server will create subdirectories for the user specified in `dfc.conf`. 

