#include <server.h>


Task printLocalTime;

void Server::setup()
{
    mesh.setDebugMsgTypes(ERROR | COMMUNICATION | CONNECTION); // set before init() so that you can see startup messages
    mesh.init(Server::MESH_PREFIX, Server::MESH_PASSWORD, Server::MESH_PORT, 0, 6);

    mesh.setRoot(true);
    mesh.setContainsRoot(true);

    mesh.onReceive(bind(&Device::receivedCallback, this, std::placeholders::_1, std::placeholders::_2));
    mesh.onNewConnection(bind(&Server::newConnectionCallback, this, std::placeholders::_1));

}

void Server::newConnectionCallback(uint32_t nodeId){
    sendSingle(Device::serv, "Get", "Time", {});
}
