/*
 The MIT License (MIT)

 Copyright (c) [2016] [BTC.COM]

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */
#include "BlockMakerEth.h"
#include "EthConsensus.h"

#include "utilities_js.hpp"

#include <thread>
#include <boost/thread.hpp>

////////////////////////////////////////////////BlockMakerEth////////////////////////////////////////////////////////////////
BlockMakerEth::BlockMakerEth(shared_ptr<BlockMakerDefinition> def, const char *kafkaBrokers, const MysqlConnectInfo &poolDB) 
  : BlockMaker(def, kafkaBrokers, poolDB)
{
  useSubmitBlockDetail_ = checkRpcSubmitBlockDetail();
  if (useSubmitBlockDetail_) {
    LOG(INFO) << "use RPC eth_submitBlockDetail";
  }
  else if (checkRpcSubmitBlock()) {
    LOG(INFO) << "use RPC eth_submitBlock, it has limited functionality and the block hash will not be recorded.";
  }
  else {
    LOG(FATAL) << "Ethereum nodes doesn't support both eth_submitBlockDetail and eth_submitBlock, cannot submit block!";
  }
}

void BlockMakerEth::processSolvedShare(rd_kafka_message_t *rkmessage)
{
  const char *message = (const char *)rkmessage->payload;
  JsonNode r;
  if (!JsonNode::parse(message, message + rkmessage->len, r))
  {
    LOG(ERROR) << "decode common event failure";
    return;
  }

  if (r.type() != Utilities::JS::type::Obj ||
      r["nonce"].type() != Utilities::JS::type::Str ||
      r["header"].type() != Utilities::JS::type::Str ||
      r["mix"].type() != Utilities::JS::type::Str ||
      r["height"].type() != Utilities::JS::type::Int ||
      r["networkDiff"].type() != Utilities::JS::type::Int ||
      r["userId"].type() != Utilities::JS::type::Int ||
      r["workerId"].type() != Utilities::JS::type::Int ||
      r["workerFullName"].type() != Utilities::JS::type::Str ||
      r["chain"].type() != Utilities::JS::type::Str)
  {
    LOG(ERROR) << "eth solved share format wrong";
    return;
  }

  StratumWorker worker;
  worker.userId_ = r["userId"].int32();
  worker.workerHashId_ = r["workerId"].int64();
  worker.fullName_ = r["workerFullName"].str();

  submitBlockNonBlocking(r["nonce"].str(), r["header"].str(), r["mix"].str(), def()->nodes,
                         r["height"].uint32(), r["chain"].str(), r["networkDiff"].uint64(),
                         worker);
}

bool BlockMakerEth::submitBlock(const string &nonce, const string &header, const string &mix,
                                const string &rpcUrl, const string &rpcUserPass,
                                string &/*errMsg*/, string &/*blockHash*/) {
  string request = Strings::Format("{\"jsonrpc\": \"2.0\", \"method\": \"eth_submitWork\", \"params\": [\"%s\",\"%s\",\"%s\"], \"id\": 5}\n",
                                   HexAddPrefix(nonce).c_str(),
                                   HexAddPrefix(header).c_str(),
                                   HexAddPrefix(mix).c_str());

  string response;
  bool ok = blockchainNodeRpcCall(rpcUrl.c_str(), rpcUserPass.c_str(), request.c_str(), response);
  DLOG(INFO) << "eth_submitWork request: " << request;
  DLOG(INFO) << "eth_submitWork response: " << response;
  if (!ok) {
    LOG(WARNING) << "Call RPC eth_submitWork failed, node url: " << rpcUrl;
    return false;
  }

  JsonNode r;
  if (!JsonNode::parse(response.c_str(), response.c_str() + response.size(), r)) {
    LOG(WARNING) << "decode response failure, node url: " << rpcUrl << ", response: " << response;
    return false;
  }

  if (r.type() != Utilities::JS::type::Obj || r["result"].type() != Utilities::JS::type::Bool) {
    LOG(WARNING) << "node doesn't support eth_submitWork, node url: " << rpcUrl << ", response: " << response;
    return false;
  }

  return r["result"].boolean();
}

/**
  * RPC eth_submitWorkDetail:
  *     A new RPC to submit POW work to Ethereum node. It has the same functionality as `eth_submitWork`
  *     but returns more details about the submitted block.
  *     It defined by the BTCPool project and implemented in the Parity Ethereum node as a patch.
  * 
  * Params (same as `eth_submitWork`):
  *     [
  *         (string) nonce,
  *         (string) pow_hash,
  *         (string) mix_hash
  *     ]
  * 
  * Result:
  *     [
  *         (bool)           success,
  *         (string or null) error_msg,
  *         (string or null) block_hash,
  *         ... // more fields may added in the future
  *     ]
  */
bool BlockMakerEth::submitBlockDetail(const string &nonce, const string &header, const string &mix,
                                      const string &rpcUrl, const string &rpcUserPass,
                                      string &errMsg, string &blockHash) {
  
  string request = Strings::Format("{\"jsonrpc\": \"2.0\", \"method\": \"eth_submitWorkDetail\", \"params\": [\"%s\",\"%s\",\"%s\"], \"id\": 5}\n",
                                   HexAddPrefix(nonce).c_str(),
                                   HexAddPrefix(header).c_str(),
                                   HexAddPrefix(mix).c_str());

  string response;
  bool ok = blockchainNodeRpcCall(rpcUrl.c_str(), rpcUserPass.c_str(), request.c_str(), response);
  DLOG(INFO) << "eth_submitWorkDetail request: " << request;
  DLOG(INFO) << "eth_submitWorkDetail response: " << response;
  if (!ok) {
    LOG(WARNING) << "Call RPC eth_submitWorkDetail failed, node url: " << rpcUrl;
    return false;
  }

  JsonNode r;
  if (!JsonNode::parse(response.c_str(), response.c_str() + response.size(), r)) {
    LOG(WARNING) << "decode response failure, node url: " << rpcUrl << ", response: " << response;
    return false;
  }

  if (r.type() != Utilities::JS::type::Obj || r["result"].type() != Utilities::JS::type::Array) {
    LOG(WARNING) << "node doesn't support eth_submitWorkDetail, node url: " << rpcUrl << ", response: " << response;
    return false;
  }

  auto result = r["result"].children();

  bool success = false;
  if (result->size() >= 1 && result->at(0).type() == Utilities::JS::type::Bool) {
    success = result->at(0).boolean();
  }

  if (result->size() >= 2 && result->at(1).type() == Utilities::JS::type::Str) {
    errMsg = result->at(1).str();
  }

  if (result->size() >= 3 && result->at(2).type() == Utilities::JS::type::Str) {
    blockHash = result->at(2).str();
  }

  return success;
}

bool BlockMakerEth::checkRpcSubmitBlock() {
  string request = Strings::Format("{\"jsonrpc\": \"2.0\", \"method\": \"eth_submitWork\", \"params\": [\"%s\",\"%s\",\"%s\"], \"id\": 5}\n",
                                   "0x0000000000000000",
                                   "0x0000000000000000000000000000000000000000000000000000000000000000",
                                   "0x0000000000000000000000000000000000000000000000000000000000000000");

  for (const auto &itr : def()->nodes) {
    string response;
    bool ok = blockchainNodeRpcCall(itr.rpcAddr_.c_str(), itr.rpcUserPwd_.c_str(), request.c_str(), response);
    if (!ok) {
      LOG(WARNING) << "Call RPC eth_submitWork failed, node url: " << itr.rpcAddr_;
      return false;
    }

    JsonNode r;
    if (!JsonNode::parse(response.c_str(), response.c_str() + response.size(), r)) {
      LOG(WARNING) << "decode response failure, node url: " << itr.rpcAddr_ << ", response: " << response;
      return false;
    }

    if (r.type() != Utilities::JS::type::Obj || r["result"].type() != Utilities::JS::type::Bool) {
      LOG(WARNING) << "node doesn't support eth_submitWork, node url: " << itr.rpcAddr_ << ", response: " << response;
      return false;
    }
  }

  return true;
}

bool BlockMakerEth::checkRpcSubmitBlockDetail() {
  string request = Strings::Format("{\"jsonrpc\": \"2.0\", \"method\": \"eth_submitWorkDetail\", \"params\": [\"%s\",\"%s\",\"%s\"], \"id\": 5}\n",
                                   "0x0000000000000000",
                                   "0x0000000000000000000000000000000000000000000000000000000000000000",
                                   "0x0000000000000000000000000000000000000000000000000000000000000000");

  if (def()->nodes.empty()) {
    LOG(FATAL) << "Node list is empty, cannot submit block!";
    return false;
  }

  for (const auto &itr : def()->nodes) {
    string response;
    bool ok = blockchainNodeRpcCall(itr.rpcAddr_.c_str(), itr.rpcUserPwd_.c_str(), request.c_str(), response);
    if (!ok) {
      LOG(WARNING) << "Call RPC eth_submitWorkDetail failed, node url: " << itr.rpcAddr_;
      return false;
    }

    JsonNode r;
    if (!JsonNode::parse(response.c_str(), response.c_str() + response.size(), r)) {
      LOG(WARNING) << "decode response failure, node url: " << itr.rpcAddr_ << ", response: " << response;
      return false;
    }

    if (r.type() != Utilities::JS::type::Obj ||
      r["result"].type() != Utilities::JS::type::Array ||
      r["result"].children()->size() < 3 ||
      r["result"].children()->at(0).type() != Utilities::JS::type::Bool)
    {
      LOG(WARNING) << "node doesn't support eth_submitWorkDetail, node url: " << itr.rpcAddr_ << ", response: " << response;
      return false;
    }
  }

  return true;
}

void BlockMakerEth::submitBlockNonBlocking(const string &nonce, const string &header, const string &mix, const vector<NodeDefinition> &nodes,
                                           const uint32_t height, const string &chain, const uint64_t networkDiff, const StratumWorker &worker) {
  std::vector<std::shared_ptr<std::thread>> threadPool;
  std::atomic<bool> syncSubmitSuccess(false);

  // run threads
  for (size_t i=0; i<nodes.size(); i++) {
    auto t = std::make_shared<std::thread>(
      std::bind(&BlockMakerEth::_submitBlockThread, this,
                nonce, header, mix, nodes[i],
                height, chain, networkDiff, worker,
                &syncSubmitSuccess));
    threadPool.push_back(t);
  }

  // waiting for threads to end
  for (auto &t : threadPool) {
    t->join();
  }
}

void BlockMakerEth::_submitBlockThread(const string &nonce, const string &header, const string &mix, const NodeDefinition &node,
                                       const uint32_t height, const string &chain, const uint64_t networkDiff, const StratumWorker &worker,
                                       std::atomic<bool> *syncSubmitSuccess) {
  string blockHash;

  auto submitBlockOnce = [&]() {
    // use eth_submitWorkDetail or eth_submitWork
    auto submitFunc = useSubmitBlockDetail_ ? BlockMakerEth::submitBlockDetail : BlockMakerEth::submitBlock;
    auto submitRpcName = useSubmitBlockDetail_ ? "eth_submitWorkDetail" : "eth_submitWork";

    string errMsg;
    bool success = submitFunc(nonce, header, mix, node.rpcAddr_, node.rpcUserPwd_, errMsg, blockHash);
    if (success) {
      LOG(INFO) << submitRpcName << " success, chain: " << chain << ", height: " << height
                << ", hash: " << blockHash << ", hash_no_nonce: " << header
                << ", networkDiff: " << networkDiff << ", worker: " << worker.fullName_;

      //add thread for classify block_states.
      boost::thread thread(std::bind(&BlockMakerEth::isUnclesThread, this, height, nonce, blockHash));

      return true;
    }

    LOG(WARNING) << submitRpcName << " failed, chain: " << chain << ", height: " << height << ", hash_no_nonce: " << header
                 << ", err_msg: " << errMsg;
    return false;
  };

  int retryTime = 5;
  while (retryTime > 0) {
    if (*syncSubmitSuccess) {
      LOG(INFO) << "_submitBlockThread(" << node.rpcAddr_ << "): " << "other thread submit success, skip";
      return;
    }
    if (submitBlockOnce()) {
      *syncSubmitSuccess = true;
      break;
    }
    sleep(6 - retryTime); // first sleep 1s, second sleep 2s, ...
    retryTime--;
  }

  // Still writing to the database even if submitting failed
  saveBlockToDB(nonce, header, blockHash, height, chain, networkDiff, worker);
}

void BlockMakerEth::saveBlockToDB(const string &nonce, const string &header, const string &blockHash, const uint32_t height,
                                  const string &chain, const uint64_t networkDiff, const StratumWorker &worker) {
  const string nowStr = date("%F %T");
  string sql;
  sql = Strings::Format("INSERT INTO `found_blocks` "
                        " (`puid`, `worker_id`"
                        ", `worker_full_name`, `chain`"
                        ", `height`, `hash`, `hash_no_nonce`, `nonce`"
                        ", `rewards`"
                        ", `network_diff`, `created_at`)"
                        " VALUES (%ld, %" PRId64
                        ", '%s', '%s'"
                        ", %lu, '%s', '%s', '%s'"
                        ", %" PRId64
                        ", %" PRIu64 ", '%s'); ",
                        worker.userId_, worker.workerHashId_,
                        // filter again, just in case
                        filterWorkerName(worker.fullName_).c_str(), chain.c_str(),
                        height, blockHash.c_str(), header.c_str(), nonce.c_str(),
                        EthConsensus::getStaticBlockReward(height, chain),
                        networkDiff, nowStr.c_str());

  // try connect to DB
  MySQLConnection db(poolDB_);
  for (size_t i = 0; i < 3; i++) {
    if (db.ping())
      break;
    else
      sleep(3);
  }

  if (db.execute(sql) == false) {
    LOG(ERROR) << "insert found block failure: " << sql;
  }
  else
  {
    LOG(INFO) << "insert found block success for height " << height;
  }
}

const int64_t REWARDS = 2e+18;
void BlockMakerEth::isUnclesThread(const uint32_t height, const string &nonce, const string &hash)
{
  sleep(120);
  string strCurrentHeight = BlockMakerEth::getBlockHeight();
  bool is_orphan = true;
  bool is_uncles = false;

  if (strCurrentHeight != "")
  {
    uint32_t currentHeight = (uint32_t)strtoul(strCurrentHeight.c_str(), nullptr, 16);
    LOG(INFO) << "------start to get block states in height: " << height << " && pendingHeight : " << currentHeight;

    for (uint32_t i = 0; i < 8; i++)
    {
      if ((height + i) >= currentHeight)
      {
        break;
      }
      std::string strheight = Strings::Format("0x%x", (height + i));
      BlockRply block = BlockMakerEth::getBlockByHeight(strheight);

      if (BlockMakerEth::matchBlock(block, nonce, hash))
      {
        is_orphan = false;
        LOG(INFO) << "~~~~~~~~ this is normal block: " << height << " -- " << (height + i) << "; [Rewards]:" << REWARDS << "; [hash]: " << block.hash << "; [nonce]: " << block.nonce;
        BlockMakerEth::updateBlockToDB(nonce, height, (height + i), is_orphan, is_uncles, REWARDS);
        break;
      }

      if (block.uncles.empty())
      {
        continue;
      }

      // Trying to find uncle in current block during our forward check
      int count = block.uncles.size();
      LOG(INFO) << "uncles : " << count;
      for (int j = 0; j < count; j++)
      {
        std::string uncleIndex = Strings::Format("0x%x", j);
        BlockRply uncle = BlockMakerEth::getUncleByBlockNumberAndIndex(strheight, uncleIndex);
        if (BlockMakerEth::matchBlock(uncle, nonce, hash))
        {
          is_orphan = false;
          is_uncles = true;
          int64_t uncleRewards;
          uncleRewards = int64_t(REWARDS / 8 * (8 - i));
          LOG(INFO) << "~~~~~~~~ this is uncles block: " << height << " -- " << (height + i) << "; [uncleRewards]:" << uncleRewards << "; [hash]: " << block.hash << "; [nonce]: " << block.nonce;
          BlockMakerEth::updateBlockToDB(nonce, height, (height + i), is_orphan, is_uncles, uncleRewards);
          //change height;
          break;
        }
      }

      // Found block or uncle
      if (!is_orphan)
      {
        break;
      }
    }
    LOG(INFO) << "-------block in height: " << height << "; is_uncles : " << is_uncles << "; is_orphan: " << is_orphan;
  }
}

string BlockMakerEth::getBlockHeight()
{
  string request = "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getBlockByNumber\",\"params\":[\"pending\", false],\"id\":2}";

  for (const auto &itr : def()->nodes)
  {
    string response;
    bool ok = blockchainNodeRpcCall(itr.rpcAddr_.c_str(), itr.rpcUserPwd_.c_str(), request.c_str(), response);
    if (!ok)
    {
      LOG(WARNING) << "Call RPC eth_getBlockByNumber failed, node url: " << itr.rpcAddr_;
      return "";
    }

    JsonNode r;
    if (!JsonNode::parse(response.c_str(), response.c_str() + response.size(), r))
    {
      LOG(WARNING) << "decode response failure, node url: " << itr.rpcAddr_ << ", response: " << response;
      return "";
    }

    JsonNode result = r["result"];
    if (result.type() != Utilities::JS::type::Obj ||
        result["number"].type() != Utilities::JS::type::Str)
    {
      LOG(ERROR) << "block informaiton format not expected: " << response;
      return "";
    }

    return result["number"].str();
  }

  return "";
}

BlockRply BlockMakerEth::getBlockByHeight(string height)
{
  string request = Strings::Format("{\"jsonrpc\": \"2.0\", \"method\": \"eth_getBlockByNumber\", \"params\": [\"%s\",true], \"id\": 2}",
                                   height.c_str());
  BlockRply Res;
  for (const auto &itr : def()->nodes)
  {
    string response;
    bool ok = blockchainNodeRpcCall(itr.rpcAddr_.c_str(), itr.rpcUserPwd_.c_str(), request.c_str(), response);
    if (!ok)
    {
      LOG(WARNING) << "Call RPC eth_getBlockByNumber failed, node url: " << itr.rpcAddr_;
      return Res;
    }

    JsonNode r;
    if (!JsonNode::parse(response.c_str(), response.c_str() + response.size(), r))
    {
      LOG(WARNING) << "decode response failure, node url: " << itr.rpcAddr_ << ", response: " << response;
      return Res;
    }

    JsonNode result = r["result"];
    if (result.type() != Utilities::JS::type::Obj ||
        result["hash"].type() != Utilities::JS::type::Str ||
        result["nonce"].type() != Utilities::JS::type::Str ||
        result["uncles"].type() != Utilities::JS::type::Array)
    {
      LOG(ERROR) << "block informaiton format not expected: " << response;
      return Res;
    }

    Res.hash = result["hash"].str();
    Res.nonce = result["nonce"].str();

    auto uncle = result["uncles"].array();
    for (int i = 0; i < uncle.size(); i++)
    {
      Res.uncles.push_back(uncle[i].str());
    }
    return Res;
  }

  return Res;
}

BlockRply BlockMakerEth::getUncleByBlockNumberAndIndex(string height, string index)
{
  string request = Strings::Format("{\"jsonrpc\": \"2.0\", \"method\": \"eth_getUncleByBlockNumberAndIndex\", \"params\": [\"%s\",\"%s\"], \"id\": 0}\n",
                                   height.c_str(), index.c_str());
  LOG(INFO) << request;
  BlockRply Res;
  for (const auto &itr : def()->nodes)
  {
    string response;
    bool ok = blockchainNodeRpcCall(itr.rpcAddr_.c_str(), itr.rpcUserPwd_.c_str(), request.c_str(), response);
    if (!ok)
    {
      LOG(WARNING) << "Call RPC eth_getBlockByNumber failed, node url: " << itr.rpcAddr_;
      return Res;
    }

    JsonNode r;
    if (!JsonNode::parse(response.c_str(), response.c_str() + response.size(), r))
    {
      LOG(WARNING) << "decode response failure, node url: " << itr.rpcAddr_ << ", response: " << response;
      return Res;
    }

    JsonNode result = r["result"];
    if (result.type() != Utilities::JS::type::Obj ||
        result["hash"].type() != Utilities::JS::type::Str ||
        result["nonce"].type() != Utilities::JS::type::Str ||
        result["uncles"].type() != Utilities::JS::type::Array)
    {
      LOG(ERROR) << "block informaiton format not expected: " << response;
      return Res;
    }
    Res.hash = result["hash"].str();
    Res.nonce = result["nonce"].str();

    return Res;
  }

  return Res;
}

bool BlockMakerEth::matchBlock(BlockRply block, const string &nonce, const string &hash)
{
  // Just compare hash if block is unlocked as immature
  if (hash.length() > 0 && hash == block.hash)
  {
    LOG(INFO) << "---HASH EQUAL ---[hash]:" << block.hash;
    return true;
  }
  // Geth-style candidate matching
  if (block.nonce.length() > 0 && nonce == block.nonce)
  {
    LOG(INFO) << "---NONCE EQUAL ---[nonce]:" << block.nonce;
    return true;
  }

  return false;
}

void BlockMakerEth::updateBlockToDB(const string &nonce, const uint32_t height, const uint32_t height_rel,
                                    const int is_orphaned, const int is_uncle, const int64_t reward)
{
  string sql;
  sql = Strings::Format(" UPDATE `found_blocks` SET `height`= %lu, "
                        " `is_orphaned`=%d, `is_uncle`=%d, `rewards`= %" PRId64 ""
                        " WHERE `height`= %lu AND `nonce`= \"%s\";",
                        height_rel,
                        is_orphaned, is_uncle, reward,
                        height, nonce.c_str());
  LOG(INFO) << sql;

  // try connect to DB
  MySQLConnection db(poolDB_);
  for (size_t i = 0; i < 3; i++)
  {
    if (db.ping())
      break;
    else
      sleep(3);
  }

  if (db.execute(sql) == false)
  {
    LOG(ERROR) << "----update found block failure: " << sql;
  }
  else
  {
    LOG(INFO) << "----update found block success for height " << height;
  }
}