#ifndef __WIFI_HEADER__
#define __WIFI_HEADER__




#include <string>

#include "common/wpa_ctrl.h"
#include "utils/common.h"
#include <dirent.h>



typedef enum {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
} wl_status_t;



class FakeIPAddress{
  std::string ip;

  public:
  FakeIPAddress(std::string ip):ip(ip){}

  std::string toString(){
    return ip;
  }

};


class WiFiClass {
 struct wpa_ctrl *ctrl_conn;
 int hostapd_cli_quit = 0;
 int hostapd_cli_attached = 0;
 const char *ctrl_iface_dir = "/var/run/wpa_supplicant";
 char *ctrl_ifname = NULL;
 int ping_interval = 5;
 public:
  void disconnect() {}
  auto status() {
    return WL_CONNECTED;
  }

  void connect(){

  }

  void search(){
    ctrl_conn = wpa_ctrl_open(ctrl_iface_dir);
    if (!ctrl_conn){
        printf("Could not get ctrl interface!\n");
    }
  }

  FakeIPAddress localIP(){
    return FakeIPAddress("192.168.49.24");
  }

};
#endif