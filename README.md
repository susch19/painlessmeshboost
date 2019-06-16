# PainlessMeshBoost

This is a boost based version of [painlessMesh](gitlab.com/painlessMesh/painlessMesh). For more details on how to set up painlessMesh to log to this listener see the usage instructions below and the [wiki](https://gitlab.com/BlackEdder/painlessMesh/wikis/bridge-between-mesh-and-another-network). `painlessMeshBoost` is a replacement of the old `painlessMeshListener`, which was harder to maintain and harder to compile on low memory/arm devices such as the raspberrypi. `painlessMeshBoost` shares most of its code with `painlessMesh`, which ensures compatibility between the two.

## Install 

```
git clone --recurse-submodules https://gitlab.com/painlessMesh/painlessmeshboost.git
cd painlessmeshboost
cmake .
make
sudo make install
```

`painlessMeshBoost` depends on a number of boost libraries. On ubuntu (or other debian derivatives) these can be installed with:

```
sudo apt-get install libboost-system-dev libboost-thread-dev libboost-regex-dev libboost-p rogram-options-dev libboost-chrono-dev libboost-date-time-dev libboost-atomic-dev
```

## Usage

`painlessMeshBoost` can be run as both a server or a client node. By default it runs as a server node. If you use `./painlessMeshBoost --log` it will log all events (including messages received) to the console. To expand the nodes behaviour you can change the source code in `src/main.cpp`. The painlessMesh implementation is the same as in the original code and can therefore be changed in a similar way. 

### Server

This is the default mode when you run painlessMeshBoost

```
./painlessMeshBoost
```

In this mode it will listen on a port (5555 by default, use `-p <port>` to change this). This mode requires you to include a [bridge](https://gitlab.com/BlackEdder/painlessMesh/blob/master/examples/bridge/bridge.ino) node in your network. 

### Client

Running it as a client requires you to setup a WiFi connection to the mesh network. Then you can run `painlessMeshBoost` to connect to the node you are connected to. You need the IP address of this node (this is typically the IP of the gateway of the WiFi connection to the mesh).

```
./painlessMeshBoost -c <ip>
```


