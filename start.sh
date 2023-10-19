#!/bin/bash

trap "kill 0" EXIT

echo "Creating virtual Rx/Tx byte streams.."

socat -d -d pty,raw,echo=0 pty,raw,echo=0 2>&1 | grep --line-buffered "N PTY is" | grep --line-buffered -o '[^ ]*$' > socat.output &

sleep 2

cat socat.output

TX=$(head -n 1 socat.output)
RX=$(tail -n 1 socat.output)

echo "
Senior should send to $TX, listen on $RX
Junior should listen on $TX, send to $RX"

#./Junior 13 1 $RX $TX
#./Senior $TX $RX

wait
