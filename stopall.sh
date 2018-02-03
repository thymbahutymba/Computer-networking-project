#!/bin/sh

for i in $(pidof msg_client msg_server);
do
	kill $i;
done
