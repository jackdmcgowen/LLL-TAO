/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/accounts.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get a user's account. */
        json::json Accounts::GetTransactions(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Check for genesis parameter. */
            if(params.find("genesis") == params.end())
                throw APIException(-25, "Missing Genesis ID");

            /* Check for paged parameter. */
            uint32_t nPage = 0;
            if(params.find("page") != params.end())
                nPage = atoi(params["page"].get<std::string>().c_str());

            /* Check for username parameter. */
            uint32_t nLimit = 100;
            if(params.find("limit") != params.end())
                nLimit = atoi(params["limit"].get<std::string>().c_str());

            /* Get the Genesis ID. */
            uint256_t hashGenesis = uint256_t(params["genesis"].get<std::string>());

            /* Get the last transaction. */
            uint512_t hashLast = 0;
            if(!LLD::legDB->ReadLast(hashGenesis, hashLast))
                throw APIException(-28, "No transactions found");

            /* Loop until genesis. */
            uint32_t nTotal = 0;
            while(hashLast != 0)
            {
                /* Get the current page. */
                uint32_t nCurrentPage = nTotal / nLimit;

                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::legDB->ReadTx(hashLast, tx))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;
                ++nTotal;

                /* Check the paged data. */
                if(nCurrentPage < nPage)
                    continue;

                if(nCurrentPage > nPage)
                    break;

                json::json obj;
                obj["version"]   = tx.nVersion;
                obj["sequence"]  = tx.nSequence;
                obj["timestamp"] = tx.nTimestamp;
                obj["genesis"]   = tx.hashGenesis.ToString();
                obj["nexthash"]  = tx.hashNext.ToString();
                obj["prevhash"]  = tx.hashPrevTx.ToString();
                obj["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
                obj["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
                obj["hash"]      = tx.GetHash().ToString();

                ret.push_back(obj);
            }

            return ret;
        }
    }
}
