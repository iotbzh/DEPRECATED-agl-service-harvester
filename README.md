# README

## Installation

```bash
git clone --recurse-submodules https://github.com/iotbzh/agl-service-harvester.git
cd agl-service-harvester.git
mkdir build && cd build
cmake .. && make
```

## Usage

Typical example to write in a TimeSeries DB from source project directory:

```bash
$ cd build/package/ && /opt/AGL/bin/afb-daemon --workdir=. --name=afbd-harvester --ldpaths=lib --roothttp=. --tracereq=common --token=1 -vvv
[...]
$ afb-client-demo ws://localhost:1234/api?token=1
harvester auth
ON-REPLY 1:harvester/write: {"jtype":"afb-reply","request":{"status":"success", "uuid":"03dc89fb-88b4-4204-ba9b-13dded3c38ab"}}

harvester write {"metric": [{"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 0},"timestamp": 1526048390725229811}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 1},"timestamp": 1526048590732571963}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 2},"timestamp": 1526048790741053301}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 3},"timestamp": 1526049390746492374}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 4},"timestamp": 1526049590753850373}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 5},"timestamp": 1526049790760449841}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 6},"timestamp": 1526049990768334799}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 7},"timestamp": 1526052290774058754}, {"name": "engine_speed","metadata": {"source": "my_source","identity": "claneys"},"values": {"value": 8},"timestamp": 1526055590781866679}]}

ON-REPLY 2:harvester/write: {"jtype":"afb-reply","request":{"status":"success","info":"Request has been successfully writen"}}
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
influx
Connected to http://localhost:8086 version 1.5.1
InfluxDB shell version: 1.5.1
> use agl-garner
Using database agl-garner
> drop measurement engine_speed
```
