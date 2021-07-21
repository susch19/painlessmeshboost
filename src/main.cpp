// #define LINUX_ENVIRONMENT
#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>// #define LINUX_ENVIRONMENT
#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include "Arduino.h"
#include <cstdint>
#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <csignal>

#undef F
#include <boost/algorithm/hex.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
// #include <boost/date_time.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <chat_client.hpp>
#include <iterator>

#define F(string_literal) string_literal
using String=std::string;

#include "painlessMeshConnection.h"
#include "painlessMesh.h"
#include "serializer.hpp"
#include "package.hpp"

WiFiClass WiFi;
ESPClass ESP;
class LittleFS LittleFS;
class Update Update;

// #include "plugin/performance.hpp"

painlessmesh::logger::LogClass Log;



namespace po = boost::program_options;

#include <cstdlib>
#include <limits>
#include <random>

using boost::asio::ip::tcp;
#define OTA_PART_SIZE 3072
#include "piota.hpp"
using namespace boost::endian;

namespace ssl = boost::asio::ssl;
typedef ssl::stream<tcp::socket> ssl_socket;

template <class T>
// bool contains(T &v, T::value_type const value) {
bool contains(T& v, std::string const value) {
    return std::find(v.begin(), v.end(), value) != v.end();
}




using boost::asio::ip::tcp;

void handleNonMeshMessage(painlessMesh &mesh, std::string str, chat_client &c_client){

    std::string m,c;
    int offset = 2;
    SerializeHelper::deserialize(&m, str, offset);
    SerializeHelper::deserialize(&c, str, offset);



    if(m == "Get" && c == "Mesh" ){
        std::string resultMessage;
        const std::string updateStr = "Update";
        const std::string meshStr = "Mesh";
        uint32_t id = 1;
        offset=0;
        SmarthomeHeader header = {.version = 1, .type = packageType::Normal};
        SerializeHelper::serialize(&id, resultMessage, offset);
        SerializeHelper::serialize(&header, resultMessage, offset);
        SerializeHelper::serialize(&updateStr, resultMessage, offset);
        SerializeHelper::serialize(&meshStr, resultMessage, offset);
        auto meshTree = mesh.asNodeTree().toString();
        uint8_t paramLength = 1;
        SerializeHelper::serialize(&paramLength, resultMessage, offset);

        SerializeHelper::serialize(&meshTree, resultMessage, offset);

        c_client.write(resultMessage);
    }
}

int main(int ac, char* av[]) {
    using namespace painlessmesh;
    try {




        size_t port = 5555;
        std::string interfaceName = "";
        std::vector<std::string> logLevel;
        size_t nodeId = 1;
        std::string otaDir;
        double performance = 2.0;
        std::string serverIp = "192.168.49.28";
        std::string serverPort = "8801";

        po::options_description desc("Allowed options");
        desc.add_options()("help,h", "Produce this help message")(
                    "nodeid,n", po::value<size_t>(&nodeId),
                    "Set nodeID, otherwise set to a random value")(
                    "port,p", po::value<size_t>(&port), "The mesh port (default is 5555)")(
                    "server,s",
                    "Listen to incoming node connections. This is the default, unless "
                    "--client "
                    "is specified. Specify both if you want to both listen for incoming "
                    "connections and try to connect to a specific node.")(
                    "client,c", po::value<std::string>(&interfaceName),
                    "Connect to another node as a client. You need to provide the interface "
                    "name of the connection.")(
                    "log,l", po::value<std::vector<std::string>>(&logLevel),
                    "Only log given events to the console. By default all events are "
                    "logged, this allows you to filter which ones to log. Events currently "
                    "logged are: receive, connect, disconnect, change, offset and delay. "
                    "This option can be specified multiple times to log multiple types of "
                    "events.")("ota-dir,d", po::value<std::string>(&otaDir),
                               "Watch given folder for new firmware files.")(
                    "serverip,S", po::value<std::string>(&serverIp),
                    "AppBroker Server IP (default is 192.168.49.28)")(
                    "serverport,P", po::value<std::string>(&serverPort),
                    "AppBroker Server Port (default is 8801)");

        po::variables_map vm;
        po::store(po::parse_command_line(ac, av, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        int fd;
        struct ifreq ifr;

        fd = socket(AF_INET, SOCK_DGRAM, 0);

        /* I want to get an IPv4 IP address */
        ifr.ifr_addr.sa_family = AF_INET;

        /* I want IP address attached to specified interface */
        strncpy(ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ-1);

        ioctl(fd, SIOCGIFADDR, &ifr);

        close(fd);

         /* display result */
        std::string ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

        std::cout << ip << std::endl;

        std::string newIp;

        while(ip.back() != '.'){
            ip.pop_back();
        }
        ip.push_back('1');

        Scheduler scheduler;

        boost::asio::io_service socket_io_service;
        chat_client c_client(socket_io_service, serverIp, serverPort);

        boost::thread t([&socket_io_service]{
            for(;;)
                socket_io_service.poll();
        }
        );

        boost::asio::io_service io_service;
        painlessMesh mesh;
        Log.setLogLevel(ERROR);
        //mesh.init(&scheduler, nodeId, port);
        mesh.init(&scheduler, nodeId);
        std::shared_ptr<AsyncServer> pServer;

        c_client.setHandleMessageReceiveHandler([&mesh, &c_client](const mesh_message& mm) {
            uint8_t type;
            int offset = 0;
            auto data = mm.body().substr(4); //skip first 4 bytes to skip nodeid
            SerializeHelper::deserialize(&type, data, offset);

            if(type == 255)
            {
                handleNonMeshMessage(mesh, mm.body().substr(5), c_client);
                return;
            }
            auto nodeId = (*(uint32_t*)mm.body().data());
            if(type == 8)
                mesh.sendBroadcast(mm.body().substr(5));
            else if(type == 9)
                mesh.sendSingle(nodeId, mm.body().substr(5));
        });

        if (vm.count("server") || !vm.count("client")) {
            pServer = std::make_shared<AsyncServer>(io_service, port);
            painlessmesh::tcp::initServer<MeshConnection, painlessMesh>(*pServer,
                                                                        mesh);
        }

        std::shared_ptr<AsyncClient> pClient;
        if (vm.count("client")) {
            pClient = std::make_shared<AsyncClient>(io_service);
            painlessmesh::tcp::connect<MeshConnection, painlessMesh>(
                        (*pClient), boost::asio::ip::address::from_string(ip), port, mesh);
        }

        if (logLevel.size() == 0 || contains(logLevel, "receive")) {
            mesh.onReceive([&mesh, &c_client](uint32_t nodeId, std::string& msg) {
                /*    std::cout << "{\"event\":\"receive\",\"nodeTime\":"
                      << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                      << "\""
                      << ",\"nodeId\":" << nodeId << ",\"msg\":\"" << msg
                      << "\"}" << std::endl;*/
                // if (_nodeId == nodeId

                boost::algorithm::hex(msg, std::ostream_iterator<char> {std::cout, ""});
                std::cout << std::endl;
                msg.insert(0, 4, '\0');
                int offset = 0;
                SerializeHelper::serialize(&nodeId, msg, offset);
                c_client.write(msg);
                // mesh.sendSingle()
                // TODO
            });
        }
        if (logLevel.size() == 0 || contains(logLevel, "connect")) {
            mesh.onNewConnection([&mesh, &c_client](uint32_t nodeId) {
                /* std::cout << "{\"event\":\"connect\",\"nodeTime\":"
                  << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                  << "\""
                  << ",\"nodeId\":" << nodeId
                  << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                  << std::endl;*/
                /*std::stringstream ss;
                ss<<"{\"id\":"<<nodeId
                 <<",\"m\":\"Update\""
                <<",\"c\":\"OnNewConnection\""
                <<",\"p\":[" << mesh.asNodeTree().toString() << "]}";
                c_client.write(ss.str());*/ //TODO: Rework?
            });
        }

        if (logLevel.size() == 0 || contains(logLevel, "disconnect")) {
            mesh.onDroppedConnection([&mesh, &c_client](uint32_t nodeId) {
                /*       std::cout << "{\"event\":\"disconnect\",\"nodeTime\":"
                         << mesh.getNodeTime() << ",\"time\":\"" <<
           timeToString()
                         << "\""
                         << ",\"nodeId\":" << nodeId
                         << ", \"layout\":" << mesh.asNodeTree().toString() <<
           "}"
                         << std::endl;
          std::stringstream ss;
          ss<<"{\"id\":"<<nodeId
            <<",\"m\":\"OnDroppedConnection\""
            <<",\"c\":\"OnDroppedConnection\""
            <<",\"p\":[" << mesh.asNodeTree().toString() << "]}";
          c_client.write(ss.str());*/
            });
        }

        if (logLevel.size() == 0 || contains(logLevel, "change")) {
            mesh.onChangedConnections([&mesh, &c_client]() {
                /*    std::cout << "{\"event\":\"change\",\"nodeTime\":" <<
           mesh.getNodeTime()
                      << ",\"time\":\"" << timeToString() << "\""
                      << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                      << std::endl;*/

            });
            /*std::stringstream ss;
            ss<<"{\"id\":"<<nodeId
             <<",\"m\":\"Update\""
            <<",\"c\":\"OnChangedConnections\""
            <<",\"p\":[" << mesh.asNodeTree().toString() << "]}";
            c_client.write(ss.str());*/ //TODO: Rework?
        }

        if (logLevel.size() == 0 || contains(logLevel, "offset")) {
            mesh.onNodeTimeAdjusted([&mesh](int32_t offset) {
                /* std::cout << "{\"event\":\"offset\",\"nodeTime\":" <<
           mesh.getNodeTime()
                   << ",\"time\":\"" << timeToString() << "\""
                   << ",\"offset\":" << offset << "}" << std::endl;*/
            });
        }

        if (logLevel.size() == 0 || contains(logLevel, "delay")) {
            mesh.onNodeDelayReceived([&mesh](uint32_t nodeId, int32_t delay) {
                /*     std::cout << "{\"event\":\"delay\",\"nodeTime\":" <<
           mesh.getNodeTime()
                       << ",\"time\":\"" << timeToString() << "\""
                       << ",\"nodeId\":" << nodeId << ",\"delay\":" << delay <<
           "}"
                       << std::endl;*/
            });
        }

        //        if (vm.count("ota-dir")) {

        //            using namespace painlessmesh::plugin;
        //            // We probably want to temporary store the file
        //            // md5 -> data
        //            auto files = std::make_shared<std::map<std::string, std::string>>();
        //            // Setup task that monitors the folder for changes
        //            auto task =
        //                    mesh.addTask(TASK_SECOND*10, TASK_FOREVER, [files, &mesh, otaDir]() {
        //                // TODO: Scan for change
        //                boost::filesystem::path p(otaDir);
        //                boost::filesystem::directory_iterator end_itr;
        //                for (boost::filesystem::directory_iterator itr(p); itr != end_itr;
        //                     ++itr) {
        //                    if (!boost::filesystem::is_regular_file(itr->path())) {
        //                        continue;
        //                    }
        //                    auto stat = addFile(files, itr->path(), TASK_MINUTE*5);
        //                    if (stat.newFile) {
        //                        std::cout << "Role:"<<stat.role <<" HW:"<< stat.hw << std::endl;
        //                        // When change, announce it, load it into files
        //                        ota::Announce announce;
        //                        announce.md5 = stat.md5;
        //                        announce.role = stat.role;
        //                        announce.hardware = stat.hw;
        //                        announce.noPart =
        //                                ceil(((float)files->operator[](stat.md5).length()) /
        //                                OTA_PART_SIZE);
        //                        announce.from = mesh.getNodeId();

        //                        auto announceTask = mesh.addTask(
        //                                    TASK_MINUTE, 60,
        //                                    [&mesh, announce]() { mesh.sendPackage(&announce); });
        //                        // after anounce, remove file from memory
        //                        announceTask->setOnDisable(
        //                                    [files, md5 = stat.md5]() { files->erase(md5); });
        //                    }
        //                }
        //            });
        //            // Setup reply to data requests
        //            mesh.onPackage(11, [files, &mesh](protocol::Variant variant) {
        //                auto pkg = variant.to<ota::DataRequest>();
        //                // cut up the data and send it
        //                if (files->count(pkg.md5)) {
        //                    auto reply =
        //                            ota::Data::replyTo(pkg,
        //                                               files->operator[](pkg.md5).substr(
        //                                OTA_PART_SIZE * pkg.partNo, OTA_PART_SIZE),
        //                            pkg.partNo);
        //                    mesh.sendPackage(&reply);
        //                } else {
        //                    // Log(ERROR, "File not found");
        //                }
        //                return true;
        //            });
        //        }
        if (vm.count("ota-dir")) {
            // using namespace painlessmesh::plugin;
            // // We probably want to temporary store the file
            // // md5 -> data
            // auto files = std::make_shared<std::map<std::string, std::string>>();
            // // Setup task that monitors the folder for changes
            // auto task =
            //         mesh.addTask(TASK_SECOND, TASK_FOREVER, [files, &mesh, otaDir]() {
            //     // TODO: Scan for change
            //     boost::filesystem::path p(otaDir);
            //     boost::filesystem::directory_iterator end_itr;
            //     for (boost::filesystem::directory_iterator itr(p); itr != end_itr;
            //          ++itr) {
            //         if (!boost::filesystem::is_regular_file(itr->path())) {
            //             continue;
            //         }
            //         auto stat = addFile(files, itr->path(), TASK_SECOND);
            //         if (stat.newFile) {
            //             // When change, announce it, load it into files
            //             ota::Announce announce;
            //             announce.md5 = stat.md5;
            //             announce.role = stat.role;
            //             announce.hardware = stat.hw;
            //             announce.noPart =
            //                     ceil(((float)files->operator[](stat.md5).length()) /
            //                     stat.packageSize);
            //             announce.from = mesh.getNodeId();
            //             announce.packageSize = stat.packageSize;

            //             auto announceTask = mesh.addTask(
            //                         TASK_MINUTE, 60,
            //                         [&mesh, announce]() { mesh.sendPackage(&announce); });
            //             // after anounce, remove file from memory
            //             announceTask->setOnDisable(
            //                         [files, md5 = stat.md5]() { files->erase(md5); });
            //         }
            //     }
            // });
            // // Setup reply to data requests
            // mesh.onPackage(11, [files, &mesh](protocol::Variant variant) {
            //     auto pkg = variant.to<ota::DataRequest>();
            //     // cut up the data and send it
            //     if (files->count(pkg.md5)) {
            //         auto reply =
            //                 ota::Data::replyTo(pkg,
            //                                    files->operator[](pkg.md5).substr(
            //                     pkg.packageSize * pkg.partNo, pkg.packageSize),
            //                 pkg.partNo);
            //         mesh.sendPackage(&reply);
            //     } else {
            //         Log(ERROR, "File not found");
            //     }
            //     return true;
            // });
        }

        while (true) {
            usleep(1000);  // Tweak this for acceptable cpu usage
            mesh.update();
            io_service.poll();
        }
    } catch (std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Exception of unknown type!" << std::endl;
    }

    return 0;
}
