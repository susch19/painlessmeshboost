
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
#include <boost/date_time.hpp>
#include <boost/program_options.hpp>
#define F(string_literal) string_literal
namespace po = boost::program_options;

#include <iostream>
#include <iterator>
#include <limits>
#include <random>

std::string timeToString() {
  boost::posix_time::ptime timeLocal =
      boost::posix_time::second_clock::local_time();
  return to_iso_extended_string(timeLocal);
}

static std::random_device
    rd;  // Will be used to obtain a seed for the random number engine
static std::mt19937 gen(rd());

uint32_t runif(uint32_t from, uint32_t to) {
  std::uniform_int_distribution<uint32_t> distribution(from, to);
  return distribution(gen);
}

int main(int ac, char* av[]) {
  try {
    size_t port = 5555;
    std::string ip = "";
    size_t logLevel = 0;
    size_t nodeId = runif(0, std::numeric_limits<uint32_t>::max());

    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "Produce this help message")("nodeid,n", po::value<size_t>(&nodeId), "Set nodeID, otherwise set to a random value")(
        "port,p", po::value<size_t>(&port), "The mesh port (default is 5555)")(
        "server,s",
        "Listen to incoming node connections. This is the default, unless "
        "--client "
        "is specified. Specify both if you want to both listen for incoming "
        "connections and try to connect to a specific node.")(
        "client,c", po::value<std::string>(&ip),
        "Connect to another node as a client. You need to provide the ip "
        "address of the node.")(
        "log,l", po::value<size_t>(&logLevel)->implicit_value(0),
        "Log events to the command line. The given level will determine what "
        "is logged. 0 will log everything, 1 will log everything except "
        "messages received, 2 will log only messages received.");

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }

    boost::asio::io_service io_service;
    painlessMesh mesh;
    Log.setLogLevel(ERROR);
    mesh.init(nodeId, port);
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
      if (logLevel == 0 || logLevel == 2) {
        mesh.onReceive([&mesh](uint32_t nodeId, std::string& msg) {
          std::cout << "{\"event\":\"receive\",\"nodeTime\":"
                    << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                    << "\""
                    << ",\"nodeId\":" << nodeId << ",\"msg\":\"" << msg << "\"}"
                    << std::endl;
        });
      }
      if (logLevel < 2) {
        mesh.onNewConnection([&mesh](uint32_t nodeId) {
          std::cout << "{\"event\":\"connect\",\"nodeTime\":"
                    << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                    << "\""
                    << ",\"nodeId\":" << nodeId
                    << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                    << std::endl;
        });

        mesh.onDroppedConnection([&mesh](uint32_t nodeId) {
          std::cout << "{\"event\":\"disconnect\",\"nodeTime\":"
                    << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                    << "\""
                    << ",\"nodeId\":" << nodeId
                    << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                    << std::endl;
        });

        mesh.onChangedConnections([&mesh]() {
          std::cout << "{\"event\":\"change\",\"nodeTime\":"
                    << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                    << "\""
                    << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                    << std::endl;
        });
        mesh.onNodeTimeAdjusted([&mesh](int32_t offset) {
          std::cout << "{\"event\":\"offset\",\"nodeTime\":"
                    << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                    << "\""
                    << ",\"offset\":" << offset << "}" << std::endl;
        });
        mesh.onNodeDelayReceived([&mesh](uint32_t nodeId, int32_t delay) {
          std::cout << "{\"event\":\"delay\",\"nodeTime\":"
                    << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                    << "\""
                    << ",\"nodeId\":" << nodeId << ",\"delay\":" << delay << "}"
                    << std::endl;
        });
      }
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
