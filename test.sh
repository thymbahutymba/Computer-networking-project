#!/bin/sh

/bin/tmux new-session ./msg_server 8070 \; \
	splitw -h ./msg_client 192.168.1.199 8075 192.168.1.199 8070 \; \
	splitw -v ./msg_client 192.168.1.199 8076 192.168.1.199 8070 \; \
	attach
