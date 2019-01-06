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
#ifndef BLOCK_MAKER_ETH_H_
#define BLOCK_MAKER_ETH_H_

#include <atomic>

#include "BlockMaker.h"
#include "CommonEth.h"

struct BlockReply {
  string hash;
  string nonce;
  std::vector<string> uncles;
};

class BlockMakerEth : public BlockMaker {
 public:
  BlockMakerEth(shared_ptr<BlockMakerDefinition> def,
                const char* kafkaBrokers,
                const MysqlConnectInfo& poolDB);
  void processSolvedShare(rd_kafka_message_t* rkmessage) override;

 private:
  void submitBlockNonBlocking(const string& nonce,
                              const string& header,
                              const string& mix,
                              const vector<NodeDefinition>& nodes,
                              const uint32_t height,
                              const string& chain,
                              const uint64_t networkDiff,
                              const StratumWorker& worker);
  void _submitBlockThread(const string& nonce,
                          const string& header,
                          const string& mix,
                          const NodeDefinition& node,
                          const uint32_t height,
                          const string& chain,
                          const uint64_t networkDiff,
                          const StratumWorker& worker,
                          std::atomic<bool>* syncSubmitSuccess);
  void saveBlockToDB(const string& nonce,
                     const string& header,
                     const string& blockHash,
                     const uint32_t height,
                     const string& chain,
                     const uint64_t networkDiff,
                     const StratumWorker& worker);

  static bool submitBlock(const string& nonce,
                          const string& header,
                          const string& mix,
                          const string& rpcUrl,
                          const string& rpcUserPass,
                          string& errMsg,
                          string& blockHash);
  static bool submitBlockDetail(const string& nonce,
                                const string& header,
                                const string& mix,
                                const string& rpcUrl,
                                const string& rpcUserPass,
                                string& errMsg,
                                string& blockHash);
  bool checkRpcSubmitBlock();
  bool checkRpcSubmitBlockDetail();

  bool useSubmitBlockDetail_;

  void isUnclesThread(const uint32_t height, const string& nonce);
  string getBlockHeight();
  BlockReply getBlockByHeight(string height);
  BlockReply getUncleByBlockNumberAndIndex(string height, string index);
  bool matchBlock(BlockReply block, const string& nonce);
  void updateBlockToDB(const string& nonce,
                       const uint32_t height,
                       const string& hash,
                       const int is_orphaned,
                       const int is_uncle,
                       const int64_t reward);
};

#endif
