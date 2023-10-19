#ifndef COMMON_H_
#define COMMON_H_

#include <cstdint>
#include <vector>
#include <stdexcept>

enum State 
{
    STATE_NOT_INITIALIZED,
    STATE_IDLE,
    STATE_BUSY,
    WAITING_FOR_CONFIG,
    CONFIGURING,
    IN_PRODUCTION,
    ALL_IDLE
};

enum CommandType 
{
    CONFIGURE_COMMAND,
    MOTOR_RUN_TO_POSITION_COMMAND,
    PUMP_COMMAND
};

enum ResponseType 
{
    RESPONSE_ACKNOWLEDGEMENT,
    RESPONSE_ERROR
};

struct Command 
{
    CommandType type;
    uint8_t address;
    std::vector<uint8_t> data;
    Command() : type(CommandType(0)), address(0) {}

    Command(CommandType type, uint8_t address, const std::vector<uint8_t>& data) : type(type), address(address), data(data) {}
};

struct Response 
{
    ResponseType type;
    uint8_t address;
    std::vector<uint8_t> data;
    Response() : type(ResponseType(0)), address(0) {}

    Response(ResponseType type, uint8_t address, const std::vector<uint8_t>& data) : type(type), address(address), data(data) {}
};

std::vector<uint8_t> serializeCommand(const Command& cmd) 
{
    std::vector<uint8_t> serializedData;
    serializedData.push_back(static_cast<uint8_t>(cmd.type));
    serializedData.push_back(cmd.address);
    serializedData.insert(serializedData.end(), cmd.data.begin(), cmd.data.end());
    return serializedData;
}

Command deserializeCommand(const std::vector<uint8_t>& data) 
{
    if (data.size() < 2) 
    {
        throw std::runtime_error("Invalid command data: Not enough bytes");
    }

    CommandType type = static_cast<CommandType>(data[0]);
    uint8_t address = data[1];
    std::vector<uint8_t> commandData;

    if (data.size() > 2) 
    {
        commandData.assign(data.begin() + 2, data.end());
    }

    return Command(type, address, commandData);
}

Response deserializeResponse(const std::vector<uint8_t>& data) 
{
    if (data.size() < 2) 
    {
        throw std::runtime_error("Invalid response data: Not enough bytes");
    }

    ResponseType type = static_cast<ResponseType>(data[0]);
    uint8_t address = data[1];
    std::vector<uint8_t> responseData;

    if (data.size() > 2) 
    {
        responseData.assign(data.begin() + 2, data.end());
    }

    return Response(type, address, responseData);
}

std::vector<uint8_t> serializeResponse(const Response& response) 
{
    std::vector<uint8_t> serialized;
    // First byte could be the response type
    serialized.push_back(static_cast<uint8_t>(response.type));
    // Second byte is the address
    serialized.push_back(response.address);
    // Append the data
    serialized.insert(serialized.end(), response.data.begin(), response.data.end());
    return serialized;
}

#endif // COMMON_H_
