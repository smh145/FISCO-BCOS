/**
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief factory to create txpool
 * @file TxPoolFactory.cpp
 * @author: yujiechen
 * @date 2021-05-19
 */
#include "TxPoolFactory.h"
#include "bcos-txpool/sync/TransactionSync.h"
#include "bcos-txpool/sync/protocol/PB/TxsSyncMsgFactoryImpl.h"
#include "bcos-txpool/txpool/validator/TxValidator.h"
#include "bcos-txpool/txpool/validator/Web3NonceChecker.h"
#include "txpool/storage/MemoryStorage.h"
#include "txpool/validator/TxPoolNonceChecker.h"

using namespace bcos;
using namespace bcos::txpool;
using namespace bcos::sync;
using namespace bcos::crypto;
using namespace bcos::protocol;

TxPoolFactory::TxPoolFactory(NodeIDPtr _nodeId, CryptoSuite::Ptr _cryptoSuite,
    TransactionSubmitResultFactory::Ptr _txResultFactory, BlockFactory::Ptr _blockFactory,
    bcos::front::FrontServiceInterface::Ptr _frontService,
    bcos::ledger::LedgerInterface::Ptr _ledger, std::string _groupId, std::string _chainId,
    int64_t _blockLimit, size_t _txpoolLimit, bool checkTransactionSignature)
  : m_nodeId(std::move(_nodeId)),
    m_cryptoSuite(std::move(_cryptoSuite)),
    m_txResultFactory(std::move(_txResultFactory)),
    m_blockFactory(std::move(_blockFactory)),
    m_frontService(std::move(_frontService)),
    m_ledger(std::move(_ledger)),
    m_groupId(std::move(_groupId)),
    m_chainId(std::move(_chainId)),
    m_blockLimit(_blockLimit),
    m_txpoolLimit(_txpoolLimit),
    m_checkTransactionSignature(checkTransactionSignature)
{}


TxPool::Ptr TxPoolFactory::createTxPool(
    size_t _notifyWorkerNum, size_t _verifierWorkerNum, uint64_t _txsExpirationTime)
{
    TXPOOL_LOG(INFO) << LOG_DESC("create transaction validator");
    auto txpoolNonceChecker = std::make_shared<TxPoolNonceChecker>();
    auto web3NonceChecker = std::make_shared<Web3NonceChecker>(m_ledger);
    auto validator = std::make_shared<TxValidator>(
        txpoolNonceChecker, std::move(web3NonceChecker), m_cryptoSuite, m_groupId, m_chainId);

    TXPOOL_LOG(INFO) << LOG_DESC("create transaction config");
    auto txpoolConfig = std::make_shared<TxPoolConfig>(validator, m_txResultFactory, m_blockFactory,
        m_ledger, txpoolNonceChecker, m_blockLimit, m_txpoolLimit, m_checkTransactionSignature);

    TXPOOL_LOG(INFO) << LOG_DESC("create transaction storage");
    auto txpoolStorage =
        std::make_shared<MemoryStorage>(txpoolConfig, _notifyWorkerNum, _txsExpirationTime);

    auto syncMsgFactory = std::make_shared<TxsSyncMsgFactoryImpl>();
    TXPOOL_LOG(INFO) << LOG_DESC("create sync config");
    auto txsSyncConfig = std::make_shared<TransactionSyncConfig>(
        m_nodeId, m_frontService, txpoolStorage, syncMsgFactory, m_blockFactory, m_ledger);
    TXPOOL_LOG(INFO) << LOG_DESC("create sync engine");
    auto txsSync = std::make_shared<TransactionSync>(txsSyncConfig, m_checkTransactionSignature);

    TXPOOL_LOG(INFO) << LOG_DESC("create txpool") << LOG_KV("submitWorkerNum", _verifierWorkerNum)
                     << LOG_KV("notifyWorkerNum", _notifyWorkerNum);
    return std::make_shared<TxPool>(txpoolConfig, txpoolStorage, txsSync, _verifierWorkerNum);
}