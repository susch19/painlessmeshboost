
#include "Arduino.h"

WiFiClass WiFi;
ESPClass ESP;

#include "painlessMesh.h"
#include "painlessMeshConnection.h"

painlessmesh::logger::LogClass Log;

/*
Reading files:

std::ifstream input( "C:\\Final.gif", std::ios::binary );

// copies all data into buffer
std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});


Add support for --help, --port (default is 5555), --server, --ip (if neither
given then --server, else as required.
*/

#undef F
#include <boost/program_options.hpp>
#define F(string_literal) string_literal
namespace po = boost::program_options;

#include <iostream>
#include <iterator>

int main(int ac, char* av[]) {
  try {
    size_t port = 5555;
    std::string ip = "";

    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "Produce this help message")(
        "port,p", po::value<size_t>(&port), "The mesh port (default is 5555)")(
        "server,s",
        "Listen to incoming node connections. This is the default, unless "
        "--client "
        "is specified. Specify both if you want to both listen for incoming "
        "connections and try to connect to a specific node.")(
        "client,c", po::value<std::string>(&ip),
        "Connect to another node as a client. You need to provide the ip "
        "address of the node.")("log,l", "Log events to the command line");

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }

    /*if (vm.count("compression")) {
        cout << "Compression level was set to "
             << vm["compression"].as<double>() << ".\n";
    } else {
        cout << "Compression level was not set.\n";
    }*/
    boost::asio::io_service io_service;
    painlessMesh mesh;
    Log.setLogLevel(ERROR);
    mesh.init(1, port);
    std::shared_ptr<AsyncServer> pServer;
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

    if (vm.count("log")) {
      mesh.onReceive([&mesh](uint32_t nodeId, std::string& msg) {
        std::cout << "{\"event\":\"receive\", \"time\":" << mesh.getNodeTime()
                  << ",\"nodeId\":" << nodeId << ",\"msg\":\"" << msg << "\"}"
                  << std::endl;
      });
      mesh.onNewConnection([&mesh](uint32_t nodeId) {
        std::cout << "{\"event\":\"connect\", \"time\":" << mesh.getNodeTime()
                  << ",\"nodeId\":" << nodeId
                  << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                  << std::endl;
      });

      mesh.onDroppedConnection([&mesh](uint32_t nodeId) {
        std::cout << "{\"event\":\"disconnect\", \"time\":"
                  << mesh.getNodeTime() << ",\"nodeId\":" << nodeId
                  << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                  << std::endl;
      });

      mesh.onChangedConnections([&mesh]() {
        std::cout << "{\"event\":\"change\", \"time\":" << mesh.getNodeTime()
                  << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                  << std::endl;
      });
      mesh.onNodeTimeAdjusted([&mesh](int32_t offset) {
        std::cout << "{\"event\":\"offset\", \"time\":" << mesh.getNodeTime()
                  << ",\"offset\":" << offset << "}" << std::endl;
      });
      mesh.onNodeDelayReceived([&mesh](uint32_t nodeId, int32_t delay) {
        std::cout << "{\"event\":\"delay\", \"time\":" << mesh.getNodeTime()
                  << ",\"nodeId\":" << nodeId << ",\"delay\":" << delay << "}"
                  << std::endl;
      });
    }

    while (true) {
      usleep(1000);  // Tweak this for acceptable cpu usage
      mesh.update();
      io_service.poll();
    }
  } catch (std::exception& e) {
    std::cerr << "error: " << e.what() << std::endl;
    ;
    return 1;
  } catch (...) {
    std::cerr << "Exception of unknown type!" << std::endl;
    ;
  }

  return 0;
}
