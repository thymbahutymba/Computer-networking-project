# Computer Networking project
Messaging application with client-server approach, possibility to send messages to registered users that could be both online or offline.

## Build
Simple run `make` to build sources

## Usage
When execute binary both server and client port could be your favorite port, remember that port < 1024 require root permission.
### Start server
```./msg_server <server port>```
### Start clients
```./msg_client <local client ip> <local client port> <server ip> <server port>```
### Using tmux
With some simple change to file `test.sh` and `Makefile` you could execute server and four instances of clients with simple run `make test`.
