#include "SerialPort.h"

#include <fcntl.h>   // open
#include <unistd.h> //getpid
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>

SerialPort::SerialPort()
{
    // don't forget to run connect()
}

void SerialPort::connect(const char *port, uint32_t baudrate)
{
    printf("Open port: %s\n", port);

    serialPort = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serialPort == -1)
    {
        perror("FAILED ");
        exit(1);
    }

    fcntl(serialPort, F_SETOWN, getpid());
    fcntl(serialPort, F_SETFL, (FNDELAY)); // FASYNC Flag removed: https://stackoverflow.com/questions/68531708/c-application-quits-without-error-on-serial-data-receive

    struct termios tty;

    tcgetattr(serialPort, &tty);

    cfsetispeed(&tty, (speed_t) baudrate);
    cfsetospeed(&tty, (speed_t) baudrate);

    // taken from: https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp

    // control modes
    tty.c_cflag &= ~PARENB;         // Clear parity bit
    tty.c_cflag &= ~CSTOPB;         // Use 1 stop bit
    tty.c_cflag &= ~CSIZE;          // Clear all bits that set the data size
    tty.c_cflag |= CS8;             // Use 8 bit
    tty.c_cflag &= ~CRTSCTS;        // Disable RTS/CTS hardware flow control
    tty.c_cflag |= CREAD | CLOCAL;  // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    // local modes
    tty.c_lflag &= ~ICANON;         // Disable canonical mode
    tty.c_lflag &= ~ECHO;           // Disable echo
    tty.c_lflag &= ~ECHOE;          // Disable erasure
    tty.c_lflag &= ~ECHONL;         // Disable new-line echo
    tty.c_lflag &= ~ISIG;           // Disable interpretation of INTR, QUIT and SUSP

    // input modes
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off software flow control
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    // output modes
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    tcsetattr(serialPort, TCSANOW, &tty);
}

SerialPort::~SerialPort()
{
}

void SerialPort::disconnect()
{
    close(serialPort);
}

void SerialPort::registerReceiveCallback(receiveCallback_t callback)
{
    receiveCallback = callback;
}

void SerialPort::sendMessage(char *msg, uint16_t size)
{
    int nbytes = 0;
    int total= 0;
    int tries = 0;

    while (total < size && tries < 100)
    {
        nbytes = write(serialPort, &msg[total], size - total);
        if (nbytes >= 0)
        {
            total += nbytes;
        }
        else
        {
            tries++;
        }
    }
}

void SerialPort::receiveData(void)
{
    uint8_t buffer[1024];
    int nbytes, total = 0;
    uint8_t *bufptr = buffer;
    bool received = false;

    nbytes = read(serialPort, bufptr, 1024 - total);

    while (nbytes > 0)
    {
        received = true;

        bufptr += nbytes;
        total += nbytes;

        nbytes = 0;

        for (int i = 0; i < 10 && nbytes <= 0; ++i)
        {
            nbytes = read(serialPort, bufptr, 1024 - total);
        }
    }

    buffer[total] = '\0';
    if (received && receiveCallback)
    {
        receiveCallback(buffer, total);
    }
}


