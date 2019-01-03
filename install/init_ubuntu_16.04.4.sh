apt-get update

apt-get install -y git build-essential autotools-dev libtool autoconf automake pkg-config cmake openssl libssl-dev libcurl4-openssl-dev libconfig++-dev libboost-all-dev libgmp-dev libmysqlclient-dev zookeeper zookeeper-bin zookeeperd libzookeeper-mt-dev libzmq3-dev libgoogle-glog-dev libhiredis-dev zlib1g zlib1g-dev libprotobuf-dev protobuf-compiler

CPUS=`lscpu | grep '^CPU(s):' | awk '{print $2}'`

# zmq-v4.1.5
mkdir -p /work/source && cd /work/source
wget https://github.com/zeromq/zeromq4-1/releases/download/v4.1.5/zeromq-4.1.5.tar.gz
tar zxvf zeromq-4.1.5.tar.gz
cd zeromq-4.1.5
./autogen.sh && ./configure && make -j $CPUS
make check && make install && ldconfig

# glog-v0.3.4
mkdir -p /work/source && cd /work/source
wget https://github.com/google/glog/archive/v0.3.4.tar.gz
tar zxvf v0.3.4.tar.gz
cd glog-0.3.4
./configure && make -j $CPUS && make install

# zookeeper
sudo mkdir -p /work/zookeeper  
sudo mkdir /work/zookeeper/version-2
sudo touch /work/zookeeper/myid
sudo echo 1 > /work/zookeeper/myid
sudo chown -R zookeeper:zookeeper /work/zookeeper
sudo service zookeeper restart

# kafka
mkdir -p /work/source && cd /work/source
wget http://ftp.jaist.ac.jp/pub/apache/kafka/2.1.0/kafka_2.11-2.1.0.tgz
tar zxvf kafka_2.11-2.1.0.tgz
sed -i "s/broker.id=0/broker.id=1/g" kafka_2.11-2.1.0/config/server.properties
sed -i "s/\#listeners=PLAINTEXT\:\/\/\:9092/listeners=PLAINTEXT\:\/\/127.0.0.1:9092/g" kafka_2.11-2.1.0/config/server.properties
nohup kafka_2.11-2.1.0/bin/kafka-server-start.sh kafka_2.11-2.1.0/config/server.properties 2>&1 &

# librdkafka-v0.9.1
mkdir -p /work/source && cd /work/source
wget https://github.com/edenhill/librdkafka/archive/0.9.1.tar.gz
tar zxvf 0.9.1.tar.gz
cd librdkafka-0.9.1
./configure && make -j $CPUS && make install

# libevent-2.0.22-stable
mkdir -p /work/source && cd /work/source
wget https://github.com/libevent/libevent/releases/download/release-2.0.22-stable/libevent-2.0.22-stable.tar.gz
tar zxvf libevent-2.0.22-stable.tar.gz
cd libevent-2.0.22-stable
./configure && make -j $CPUS && 
make install

# bitcoin
mkdir -p /work && cd /work
wget -O bitcoin-0.16.0.tar.gz https://github.com/bitcoin/bitcoin/archive/v0.16.0.tar.gz
tar zxf bitcoin-0.16.0.tar.gz

# btcpool
mkdir -p /work && cd /work
git clone https://github.com/btccom/btcpool.git
cd /work/btcpool
mkdir build && cd build
cmake -DJOBS=$CPUS -DCHAIN_TYPE=BTC -DCHAIN_SRC_ROOT=/work/bitcoin-0.16.0 ..
make -j $CPUS


