# Scentronix Embedded Task
 "Scentronix Embedded Task" orchestrates slave devices in production environments, enabling real-time monitoring, dynamic configuration, and efficient state management via serial communication.
 
Tested on Ubuntu 23.10

Runs as expected.

How to run project:

1. Run ./start.sh

2. Note Output:
   
Creating virtual Rx/Tx byte streams..

/dev/pts/7 (TX)

/dev/pts/8 (RX)

3. Run Senior executable:
   
./Senior /dev/pts/8 /dev/pts/7

4. Run multiple instances of Junior.cpp:

./Junior 1 1 /dev/pts/7 /dev/pts/8 

Run for each slave addresses (i.e: 1,2,13,14,15)

Arguments: Address , Device Type, TX PORT, RX PORT



