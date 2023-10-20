#include "Common.h"
#include "library/SerialPort/SerialPort.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip> 
#include <string.h>

class Junior;
Junior* currentJuniorInstance = nullptr;
void juniorGlobalReceiveCallback(uint8_t* msg, uint16_t size);

class Junior 
{
public:
    Junior(uint8_t address, uint8_t deviceType, const std::string& rxPort, const std::string& txPort);
    ~Junior();

    void run();
    void onReceiveCommand(uint8_t* data, uint16_t size);

private:
    uint8_t address;
    uint8_t deviceType;
    State state;
    SerialPort serialPort;
    
    void sendStateToSenior(State state);
    void processCommand(const Command& cmd);
};

void juniorGlobalReceiveCallback(uint8_t* msg, uint16_t size) 
{
    if (currentJuniorInstance) 
    {
        currentJuniorInstance->onReceiveCommand(msg, size);
    }
}

Junior::Junior(uint8_t address, uint8_t deviceType, const std::string& rxPort, const std::string& txPort) : address(address), deviceType(deviceType), state(STATE_NOT_INITIALIZED) 
{
    currentJuniorInstance = this;
    serialPort.connect(rxPort.c_str(), 9600);
    serialPort.registerReceiveCallback(&juniorGlobalReceiveCallback);
    std::cout << "Junior instance created with address: " << static_cast<int>(address) << " and deviceType: " << static_cast<int>(deviceType) << std::endl;
}

Junior::~Junior() 
{
    currentJuniorInstance = nullptr;
    std::cout << "Junior instance with address: " << static_cast<int>(address) << " is being destroyed." << std::endl;
}

void Junior::run() 
{
    std::cout << "Junior instance with address: " << static_cast<int>(address) << " is running." << std::endl;

    while (true) 
    {
        serialPort.receiveData();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void Junior::onReceiveCommand(uint8_t* data, uint16_t size) 
{
    std::cout << "-------------------------------------------------------------------------------" << std::endl;
    std::cout << "Junior with address: " << static_cast<int>(address) << " received a command." << std::endl;
    std::cout << "Received data (" << size << " bytes): ";
    for (uint16_t i = 0; i < size; ++i) 
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;

    // Assuming each command is 2 bytes long, iterate through the commands in the received data.
    for (uint16_t i = 0; i < size; i += 2) 
    {
        if (i + 1 < size) // Make sure there's a pair of bytes to read
        {
            uint8_t cmdType = data[i];
            uint8_t cmdAddress = data[i + 1];

            // If the command is a CONFIGURE_COMMAND and is intended for this Junior
            if (cmdType == CONFIGURE_COMMAND && cmdAddress == this->address) 
            {
                std::cout << "-------------------------------------------------------------------------------" << std::endl;
                std::cout << "Found CONFIGURE_COMMAND for this Junior with address: " << static_cast<int>(this->address) << std::endl;
                // Create a Command object and process it
                Command cmd;
                cmd.type = static_cast<CommandType>(cmdType);
                cmd.address = cmdAddress;
                processCommand(cmd);
            }
        } 
    }
}

void Junior::processCommand(const Command& cmd) 
{
    std::cout << "Processing command of type: " << static_cast<int>(cmd.type) << std::endl;

    switch (cmd.type) 
    {
        case CONFIGURE_COMMAND:
            std::cout << "Initializing peripherals for address: " << static_cast<int>(this->address) << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Sleep to simulate initializing periphals and stuff etc.
            state = STATE_IDLE;
            std::cout << "[STATE_IDLE] Peripherals are initialized. Changing state to STATE_IDLE." << std::endl;
            sendStateToSenior(state);
            break;

        default:
            // Log an error for unknown command types. Optional: Inform Senior about the unknown command.
            std::cerr << "Received unknown command type: " << static_cast<int>(cmd.type) << std::endl;
            break;
    }  
}

void Junior::sendStateToSenior(State state)
{
    std::vector<uint8_t> stateData = { static_cast<uint8_t>(state) };
    Response resp(RESPONSE_ACKNOWLEDGEMENT, this->address, stateData);
    std::vector<uint8_t> serializedResp = serializeResponse(resp);
    std::cout << "Sending message: ";

    for (size_t i = 0; i < serializedResp.size(); ++i) 
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(serializedResp[i]);
        if (i < (serializedResp.size() - 1)) 
        {
            std::cout << " ";
        }
    }
    std::cout << std::dec << std::endl;

    serialPort.sendMessage((char*)serializedResp.data(), serializedResp.size());
    std::cout << "Sent state update to Senior." << std::endl;
    std::cout << "-------------------------------------------------------------------------------" << std::endl;

}

int main(int argc, char *argv[]) 
{
    if (argc < 5) 
    {
        std::cerr << "Usage: " << argv[0] << " <address> <deviceType> <RX port> <TX port>" << std::endl;
        return 1;
    }

    uint8_t address = std::stoi(argv[1]);
    uint8_t deviceType = std::stoi(argv[2]);
    const std::string rxPort = argv[3];
    const std::string txPort = argv[4];

    Junior junior(address, deviceType, rxPort, txPort);
    junior.run();

    return 0;
}
