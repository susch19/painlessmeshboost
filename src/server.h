#ifndef SERVER_H
#define SERVER_H

#include <Device.h>

class Server : public Device
{
public:
    Server() : Device("server", "V1") {}
    virtual ~Server() {}
    void setup() override;


    void newConnectionCallback(uint32_t nodeId);

private:
    void OnMsgReceived(uint32_t from, const String &messageType, const String &command, const String &parameter)override {}
    std::vector<MessageParameter> AdditionalWhoAmIResponseParams() override {return std::vector<MessageParameter>();}
};

#endif // SERVER_H
