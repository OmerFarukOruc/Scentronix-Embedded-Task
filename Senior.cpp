#include "Common.h"
#include "library/SerialPort/SerialPort.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <fstream>
#include <iomanip> 
#include <json/json.h>

class Senior;
Senior* currentSeniorInstance = nullptr;
void globalReceiveCallback(uint8_t* msg, uint16_t size);

class Senior 
{
public:
    Senior(const std::string& rxPort, const std::string& txPort);
    ~Senior();
    
    void run();
    bool allJuniorsIdle();
    void onReceiveResponse(uint8_t* data, uint16_t size);
    void readConfig();
    void configureDevices(const Json::Value& root, const std::string& configType);

private:
    State state;
    SerialPort serialPort;
    std::map<uint8_t, State> juniors;
};

void globalReceiveCallback(uint8_t* msg, uint16_t size) 
{
    if (currentSeniorInstance) 
    {
        currentSeniorInstance->onReceiveResponse(msg, size);
    }
}

Senior::Senior(const std::string& rxPort, const std::string& txPort) : state(WAITING_FOR_CONFIG), serialPort() 
{
    currentSeniorInstance = this;
    serialPort.connect(rxPort.c_str(), 9600);
    serialPort.registerReceiveCallback(&globalReceiveCallback);

    std::cout << "Senior instance created." << std::endl;
}

Senior::~Senior() 
{
    currentSeniorInstance = nullptr;
    std::cout << "Senior instance is being destroyed." << std::endl;
}

void Senior::run() 
{
    std::cout << "Senior instance is running." << std::endl;
    bool configRead = false;
    
    while (true) 
    {
        // Polling the serial port for incoming responses.
        serialPort.receiveData();

        switch (state) 
        {
            case WAITING_FOR_CONFIG:
                if (!configRead) 
                {
                    readConfig();
                    configRead = true;
                    state = CONFIGURING;
                }
                break;

            case CONFIGURING:
                
                if (allJuniorsIdle()) 
                {
                    state = ALL_IDLE;
                    std::cout << "All juniors are idle." << std::endl;
                }
                break;

            case ALL_IDLE:
                state = IN_PRODUCTION; // DESIRED STATE
                std::cout << "PRODUCTION!" << std::endl;
                break;

            default:
                std::cerr << "Senior is in an unknown state!" << std::endl;
                break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool Senior::allJuniorsIdle() 
{
    std::cout << "Checking if all juniors are idle..." << std::endl;
    bool allIdle = true; // Assume all are idle initially

    for (std::map<uint8_t, State>::iterator it = juniors.begin(); it != juniors.end(); ++it)
    {
        uint8_t address = it->first; // The key, which is the junior's address
        State state = it->second;    // The value, which is the junior's state

        std::cout << "Junior address: " << static_cast<int>(address) << ", State: " << state << std::endl;

        if (state != STATE_IDLE) 
        {
            std::cout << "Junior with address " << static_cast<int>(address) << " is not idle. State: " << state << ". Reconfiguring..." << std::endl;
            allIdle = false; // At least one junior is not idle

            // Re-configure the non-idle junior
            Command cmd;
            cmd.type = CONFIGURE_COMMAND;
            cmd.address = address;

            std::vector<uint8_t> serializedCmd = serializeCommand(cmd);

            std::cout << "Sending reconfiguration message: ";
            for (size_t j = 0; j < serializedCmd.size(); ++j) 
            {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(serializedCmd[j]);
                if (j < (serializedCmd.size() - 1)) 
                {
                    std::cout << " ";
                }
            }
            std::cout << std::dec << std::endl;

            serialPort.sendMessage((char*) serializedCmd.data(), serializedCmd.size());
        }
    }

    return allIdle; // Will return true if all juniors are idle, false otherwise
}

void Senior::onReceiveResponse(uint8_t* data, uint16_t size) 
{
    std::cout << "Senior received a response." << std::endl;

    std::vector<uint8_t> vecData(data, data + size);
    Response resp = deserializeResponse(vecData);

    juniors[resp.address] = STATE_IDLE; // To make sure 'resp.state' contains the current state of the junior.

    std::cout << "Response Type: " << static_cast<int>(resp.type) << ", Address: " << static_cast<int>(resp.address) << ", Data: ";

    for (size_t i = 0; i < resp.data.size(); ++i) 
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(resp.data[i]);
        if (i < (resp.data.size() - 1)) 
        {
            std::cout << " ";
        }
    }
    std::cout << std::dec << std::endl;
}

void Senior::readConfig() 
{
    std::ifstream pumpsConfigStream("devices/pumps.jsonc");
    if (!pumpsConfigStream.is_open()) 
    {
        std::cerr << "Unable to open pumps.jsonc" << std::endl;
        return;
    }

    std::cout << "-------------------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "pumps.jsonc opened." << std::endl;
    Json::Value pumpsRoot;
    pumpsConfigStream >> pumpsRoot;
    configureDevices(pumpsRoot, "pumps");
    pumpsConfigStream.close();

    std::ifstream motorsConfigStream("devices/motors.jsonc");
    if (!motorsConfigStream.is_open()) 
    {
        std::cerr << "Unable to open motors.jsonc" << std::endl;
        return;
    }

    std::cout << "-------------------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "motors.jsonc opened." << std::endl;
    Json::Value motorsRoot;
    motorsConfigStream >> motorsRoot;
    configureDevices(motorsRoot, "motors");
    motorsConfigStream.close();

    std::cout << "--------------------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "Configuration loaded. Waiting for all juniors to become idle." << std::endl;
}

void Senior::configureDevices(const Json::Value& root, const std::string& configType) 
{
    const Json::Value devices = root["devices"];
    if (!devices.isArray()) {
        std::cerr << "Error: devices is not an array in " << configType << " configuration." << std::endl;
        return;
    }

    std::cout << "Configuring " << devices.size() << " devices from " << configType << " configuration." << std::endl;

    for (int i = 0; i < devices.size(); i++) 
    {
        const Json::Value& device = devices[i];
        if (!device.isObject()) 
        {
            std::cerr << "Error: device at index " << i << " is not an object in " << configType << " configuration." << std::endl;
            continue;
        }

        uint8_t address = device.get("address", Json::Value::null).asUInt();
        uint8_t deviceType = device.get("deviceType", Json::Value::null).asUInt();

        std::cout << "Configuring device with address: " << static_cast<int>(address) << " from " << configType << " configuration." << std::endl;

        juniors[address] = STATE_NOT_INITIALIZED;

        Command cmd;
        cmd.type = CONFIGURE_COMMAND;
        cmd.address = address;

        std::vector<uint8_t> serializedCmd = serializeCommand(cmd);

        std::cout << "Sending message: ";
        for (size_t j = 0; j < serializedCmd.size(); ++j) 
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(serializedCmd[j]);
            if (j < (serializedCmd.size() - 1)) 
            {
                std::cout << " ";
            }
        }
        std::cout << std::dec << std::endl;

        serialPort.sendMessage((char*) serializedCmd.data(), serializedCmd.size());

        std::cout << "[ConfigureCommand] Sent configuration to device with address: " << static_cast<int>(address) << " and device type: " << static_cast<int>(deviceType) << " from " << configType << " configuration." << std::endl;
    }

}

int main(int argc, char *argv[]) 
{
    if (argc < 3) 
    {
        std::cerr << "Usage: " << argv[0] << " <RX port> <TX port>" << std::endl;
        return 1;
    }

    const std::string rxPort = argv[1];
    const std::string txPort = argv[2];

    Senior senior(rxPort, txPort);
    senior.run();

    return 0;
}
