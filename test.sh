#!/bin/sh

/bin/tmux new-session ./msg_server 8070 \; \
	splitw -h ./msg_client 192.168.1.199 8075 192.168.1.199 8070 \; \
	select-pane -t 0 \; \
	splitw -v ./msg_client 192.168.1.199 8076 192.168.1.199 8070 \; \
	select-pane -t 2 \; \
	splitw -v ./msg_client 192.168.1.199 8077 192.168.1.199 8070 \; \
	attach
