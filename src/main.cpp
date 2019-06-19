
#include <algorithm>
#include <boost/algorithm/hex.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <iostream>
#include <iterator>

using boost::uuids::detail::md5;

#include "Arduino.h"

WiFiClass WiFi;
ESPClass ESP;

#include "painlessMesh.h"
#include "painlessMeshConnection.h"

painlessmesh::logger::LogClass Log;

#define OTA_PART_SIZE 2048

/*
Reading files:

std::ifstream input( "C:\\Final.gif", std::ios::binary );

// copies all data into buffer
std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});


Add support for --help, --port (default is 5555), --server, --ip (if neither
given then --server, else as required.

MD5

std::string toString(const md5::digest_type &digest)
{
    const auto charDigest = reinterpret_cast<const char *>(&digest);
    std::string result;
    boost::algorithm::hex(charDigest, charDigest + sizeof(md5::digest_type),
std::back_inserter(result)); return result;
}

int main ()
{
    std::string s;

    while(std::getline(std::cin, s)) {
        md5 hash;
        md5::digest_type digest;

        hash.process_bytes(s.data(), s.size());
        hash.get_digest(digest);

        std::cout << "md5(" << s << ") = " << toString(digest) << '\n';
    }

    return 0;
}

FirmwareVersion parseFileName(string name) {
    import std.string : split;
    import std.path : baseName, extension;
    FirmwareVersion fw;
    auto ext = name.extension;
    if (ext != ".bin")
        return fw;
    auto bName = name.baseName(ext);
    auto sp = bName.split("_");
    if (sp.length < 2 || sp.length > 3 || sp[0] != "firmware")
        return fw;
    else {
        fw.hardware = sp[1];
        if (sp.length == 3)
            fw.nodeType = sp[2];
    }
    return fw;
}
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

// Will be used to obtain a seed for the random number engine
static std::random_device rd;
static std::mt19937 gen(rd());

uint32_t runif(uint32_t from, uint32_t to) {
  std::uniform_int_distribution<uint32_t> distribution(from, to);
  return distribution(gen);
}

std::vector<std::string> split(const std::string& s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

std::string toString(const md5::digest_type& digest) {
  const auto charDigest = reinterpret_cast<const char*>(&digest);
  std::string result;
  boost::algorithm::hex(charDigest, charDigest + sizeof(md5::digest_type),
                        std::back_inserter(result));
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

struct FileStat {
  bool newFile;
  std::string file;
  std::string md5;
  std::string hw;
  std::string role;
};

FileStat addFile(std::shared_ptr<std::map<std::string, std::string>> files,
                 boost::filesystem::path p, int dur) {
  // Check for time since change
  auto ftime =
      boost::posix_time::from_time_t(boost::filesystem::last_write_time(p));
  auto now = boost::posix_time::second_clock::universal_time();

  FileStat stat;
  stat.newFile = false;
  // We take some uncertainty in account when this was last run
  if ((now - ftime).total_milliseconds() < 2 * dur) {
    stat.file = p.filename().string();
    auto stem = p.stem().string();
    if (p.extension().string() != ".bin") return stat;
    auto fv = split(stem, '_');
    if (fv[0] != "firmware" || fv.size() < 2) return stat;
    stat.hw = fv[1];
    if (fv.size() == 3) stat.role = fv[2];
    std::ifstream input(p.string(), std::ios::binary);
    // copies all data into buffer
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input),
                                      {});
    md5 hash;
    md5::digest_type digest;
    hash.process_bytes(buffer.data(), buffer.size());
    hash.get_digest(digest);
    stat.md5 = toString(digest);
    if (files->count(stat.md5)) return stat;
    stat.newFile = true;
    auto data64 = painlessmesh::base64::encode(buffer.data(), buffer.size());
    files->operator[](stat.md5) = data64;
    return stat;
  }
  return stat;
}

int main(int ac, char* av[]) {
  try {
    size_t port = 5555;
    std::string ip = "";
    size_t logLevel = 0;
    size_t nodeId = runif(0, std::numeric_limits<uint32_t>::max());
    std::string otaDir;

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
        "client,c", po::value<std::string>(&ip),
        "Connect to another node as a client. You need to provide the ip "
        "address of the node.")(
        "log,l", po::value<size_t>(&logLevel)->implicit_value(0),
        "Log events to the command line. The given level will determine what "
        "is logged. 0 will log everything, 1 will log everything except "
        "messages received, 2 will log only messages received.")(
        "ota-dir,d", po::value<std::string>(&otaDir),
        "Watch given folder for new firmware files.");

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }

    Scheduler scheduler;
    boost::asio::io_service io_service;
    painlessMesh mesh;
    Log.setLogLevel(ERROR | COMMUNICATION | CONNECTION);
    mesh.init(&scheduler, nodeId, port);
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

    if (vm.count("ota-dir")) {
      using namespace painlessmesh;
      using namespace painlessmesh::plugin;
      // We probably want to temporary store the file
      // md5 -> data
      auto files = std::make_shared<std::map<std::string, std::string>>();
      // Setup task that monitors the folder for changes
      auto task = mesh.addTask(
          scheduler, TASK_SECOND, TASK_FOREVER,
          [files, &mesh, &scheduler, otaDir]() {
            // TODO: Scan for change
            boost::filesystem::path p(otaDir);
            boost::filesystem::directory_iterator end_itr;
            for (boost::filesystem::directory_iterator itr(p); itr != end_itr;
                 ++itr) {
              if (!boost::filesystem::is_regular_file(itr->path())) {
                continue;
              }
              auto stat = addFile(files, itr->path(), TASK_SECOND);
              if (stat.newFile) {
                // When change, announce it, load it into files
                ota::Announce announce;
                announce.md5 = stat.md5;
                announce.role = stat.role;
                announce.hardware = stat.hw;
                announce.noPart =
                    1 + files->operator[](stat.md5).length() / OTA_PART_SIZE;
                announce.from = mesh.nodeId;

                auto announceTask =
                    mesh.addTask(scheduler, TASK_MINUTE, 60,
                                 [&mesh, &scheduler, announce]() {
                                   std::cout << "Announcing " <<  std::endl;
                                   std::cout << announce.md5 << std::endl;
                                   mesh.sendPackage(&announce);
                                 });
                // after anounce, remove file from memory
                announceTask->setOnDisable(
                    [files, md5 = stat.md5]() { files->erase(md5); });
              }
            }
          });
      // Setup reply to data requests
      mesh.onPackage(11, [files, &mesh](protocol::Variant variant) {
        auto pkg = variant.to<ota::DataRequest>();
        // cut up the data and send it
        if (files->count(pkg.md5)) {
          std::cout << "Sending data " <<  std::endl;
          auto reply =
              ota::Data::replyTo(pkg,
                                 files->operator[](pkg.md5).substr(
                                     OTA_PART_SIZE * pkg.partNo, OTA_PART_SIZE),
                                 pkg.partNo + 1);
          mesh.sendPackage(&reply);
        } else {
          Log(ERROR, "File not found");
        }
        return true;
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
