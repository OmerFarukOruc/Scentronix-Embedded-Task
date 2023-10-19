#ifndef SerialPort_H_
#define SerialPort_H_

#include "stdint.h"

#include <stdio.h>

typedef void (*receiveCallback_t)(uint8_t* msg, uint16_t size);

class SerialPort
{
    public:
        SerialPort();
        receiveCallback_t receiveCallback = NULL;
        int serialPort = 0;

        virtual ~SerialPort();

        void registerReceiveCallback(receiveCallback_t callback);
        void sendMessage(char *msg, uint16_t size);

        void connect(const char *port, uint32_t baudrate);
        void receiveData(void);
        void disconnect();
};

#endif /* SerialPort_H_ */
