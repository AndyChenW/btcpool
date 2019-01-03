#!/bin/bash
#
# init run folders for btcpool
#
# @copyright btc.com
# @author Kevin Pan
# @since 2016-08
#

# No longer restrict the installation folder
# cd /work/btcpool/build

sourceDir=$(cd "$(dirname "$0")"; pwd)"/../src"

# blkmaker
if [ ! -d "run_blkmaker" ]; then
  mkdir "run_blkmaker" && cd "run_blkmaker"
  mkdir "log_blkmaker"
  cp "$sourceDir"/build/blkmaker .
  cp "$sourceDir"/blkmaker/blkmaker.cfg .
  nohup ./blkmaker -c blkmaker.cfg > blkmaker.log &
  cd ..
fi

# gbtmaker
if [ ! -d "run_gbtmaker" ]; then
  mkdir "run_gbtmaker" && cd "run_gbtmaker"
  mkdir "log_gbtmaker"
  cp "$sourceDir"/build/gbtmaker .
  cp "$sourceDir"/gbtmaker/gbtmaker.cfg .
  nohup ./gbtmaker -c gbtmaker.cfg > gbtmaker.log &
  cd ..
fi

# gwmaker
if [ ! -d "run_gwmaker" ]; then
  mkdir "run_gwmaker" && cd "run_gwmaker"
  mkdir "log_gwmaker"
  cp "$sourceDir"/build/gwmaker .
  cp "$sourceDir"/gwmaker/gwmaker.cfg .
  nohup ./gwmaker -c gwmaker.cfg > gwmaker.log &
  cd ..
fi

# jobmaker
if [ ! -d "run_jobmaker" ]; then
  mkdir "run_jobmaker" && cd "run_jobmaker"
  mkdir "log_jobmaker"
  cp "$sourceDir"/build/jobmaker .
  cp "$sourceDir"/jobmaker/jobmaker.cfg .
  nohup ./jobmaker -c jobmaker.cfg > jobmaker.log &
  cd ..
fi

# sharelogger
if [ ! -d "run_sharelogger" ]; then
  mkdir "run_sharelogger" && cd "run_sharelogger"
  mkdir "log_sharelogger"
  ln -s ../sharelogger .
  cp "$sourceDir"/sharelogger/sharelogger.cfg .
  nohup ./sharelogger -c sharelogger.cfg > sharelogger.log &
  cd ..
fi

# slparser
if [ ! -d "run_slparser" ]; then
  mkdir "run_slparser" && cd "run_slparser"
  mkdir "log_slparser"
  cp "$sourceDir"/build/slparser .
  cp "$sourceDir"/slparser/slparser.cfg .
  nohup ./slparser -c slparser.cfg > slparser.log &
  cd ..
fi

# sserver
if [ ! -d "run_sserver" ]; then
  mkdir "run_sserver" && cd "run_sserver"
  mkdir "log_sserver"
  cp "$sourceDir"/build/sserver .
  cp "$sourceDir"/sserver/sserver.cfg .
  nohup ./sserver -c sserver.cfg > sserver.log &
  cd ..
fi

# statshttpd
if [ ! -d "run_statshttpd" ]; then
  mkdir "run_statshttpd" && cd "run_statshttpd"
  mkdir "log_statshttpd"
  cp "$sourceDir"/build/statshttpd .
  cp "$sourceDir"/statshttpd/statshttpd.cfg .
  nohup ./statshttpd -c statshttpd.cfg > statshttpd.log &
  cd ..
fi

# poolwatcher
if [ ! -d "run_poolwatcher" ]; then
  mkdir "run_poolwatcher" && cd "run_poolwatcher"
  mkdir "log_poolwatcher"
  cp "$sourceDir"/build/poolwatcher .
  cp "$sourceDir"/poolwatcher/poolwatcher.cfg .
  nohup ./poolwatcher -c poolwatcher.cfg > poolwatcher.log &
  cd ..
fi

# simulator
if [ ! -d "run_simulator" ]; then
  mkdir "run_simulator" && cd "run_simulator"
  mkdir "log_simulator"
  cp "$sourceDir"/build/simulator .
  cp "$sourceDir"/simulator/simulator.cfg .
  nohup ./simulator -c simulator.cfg > simulator.log &
  cd ..
fi

# nmcauxmaker
if [ ! -d "run_nmcauxmaker" ]; then
  mkdir "run_nmcauxmaker" && cd "run_nmcauxmaker"
  mkdir "log_nmcauxmaker"
  cp "$sourceDir"/build/nmcauxmaker .
  cp "$sourceDir"/nmcauxmaker/nmcauxmaker.cfg .
  nohup ./nmcauxmaker -c nmcauxmaker.cfg > nmcauxmaker.log &
  cd ..
fi

