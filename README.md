# README

## Installation

```bash
git clone --recurse-submodules https://github.com/iotbzh/agl-service-harvester.git
cd agl-service-harvester.git
mkdir build && cd build
cmake .. && make
```

### InfluxDB installation

```bash
# openSUSE installation instructions
# For other distro, see docs: https://portal.influxdata.com/downloads
wget https://dl.influxdata.com/influxdb/releases/influxdb-1.5.1.x86_64.rpm
sudo zypper in ./influxdb-1.5.1.x86_64.rpm
# ignore warnings and force install

sudo systemctl daemon-reload
sudo systemctl start influxdb
sudo systemctl status influxdb
```

You may also install useful tool named chronograf (Time-Series Data Visualization)
```bash
wget https://dl.influxdata.com/chronograf/releases/chronograf-1.4.4.2.x86_64.rpm
sudo zypper in shadow
sudo zypper in ./chronograf-1.4.4.2.x86_64.rpm
# ignore warnings and force install
```

## Usage

Typical example to write in a TimeSeries DB from source project directory:

```bash
$ cd build/ && /opt/AGL/bin/afb-daemon --workdir=./package --name=afbd-harvester --ldpaths=lib --roothttp=. --tracereq=common --token=1 -vvv
[...]
$ afb-client-demo ws://localhost:1234/api?token=1
harvester auth
ON-REPLY 1:harvester/auth: {"jtype":"afb-reply","request":{"status":"success", "uuid":"03dc89fb-88b4-4204-ba9b-13dded3c38ab"}}

harvester write {"metric": [{"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 0},"timestamp": 1526048390725229811}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 1},"timestamp": 1526048590732571963}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 2},"timestamp": 1526048790741053301}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 3},"timestamp": 1526049390746492374}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 4},"timestamp": 1526049590753850373}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 5},"timestamp": 1526049790760449841}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 6},"timestamp": 1526049990768334799}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 7},"timestamp": 1526052290774058754}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 8},"timestamp": 1526055590781866679}]}

ON-REPLY 2:harvester/write: {"jtype":"afb-reply","request":{"status":"success","info":"Request has been successfully written"}}
```

```bash
afb-client-demo -H ws://localhost:1234/api?token=1 harvester write '{"metric": [{"name": "engine_speed", "metadata": {"source": "my_source","identity": "claneys"}, "values": {"value": 10}, "timestamp": 1526065590781866679}]}'
ON-REPLY 1:harvester/write: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "info":"Request has been successfully written",
    "uuid":"24d61c19-469f-4fad-a85b-55f4bac59e8e"
  }
}
afb-client-demo -H ws://localhost:1234/api?token=1 harvester write '{"metric": [{"name": "engine_speed","metadata": {source": "my_source","identity": "claneys"},"values": {"value": 10},"timestamp": 1526065590781866679}]}'
```

## InfluxDB check

```bash
sudo systemctl start influxdb
```

Check a measurement:

```bash
$ influx
Connected to http://localhost:8086 version 1.5.1
InfluxDB shell version: 1.5.1
> use agl-garner
Using database agl-garner
> select value_f from engine_speed
name: engine_speed
time                value_f
----                -------
1526643129561864049 0
1526643129561873378 1
1526643129561877058 2
1526643129561879981 3
1526643129561882627 4
1526643129561885153 5
1526643129561888066 6
1526643129561891514 7
1526643129561893920 8
> exit
```

Drop measurement:

```bash
influx -database 'agl-garner' -execute 'drop measurement engine_speed'
```
