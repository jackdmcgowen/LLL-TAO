/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/names.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/create.h>
#include <TAO/Operation/include/write.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{
    /* API Layer namespace. */
    namespace API
    {
        /* History of an asset and its ownership */
        json::json Names::NameHistory(const json::json& params, bool fHelp)
        {
            /* Return JSON object */
            json::json ret;

            /* The register address of the Name object */
            uint256_t hashRegister = 0;

            /* Declare the Name object to look for*/
            TAO::Register::Object name;

            /* If the caller has provided a name parameter then retrieve it by name */
            if(params.find("name") != params.end())
                name = Names::GetName(params, params["name"].get<std::string>(), hashRegister);

            /* Otherwise try to find the name record based on the register address. */
            else if(params.find("address") != params.end())
            {
                /* Check that the caller has passed a valid register address */
                if(!IsRegisterAddress(params["address"].get<std::string>()))
                    throw APIException(-25, "Invalid address");

                hashRegister.SetHex(params["address"].get<std::string>());

                /* Read the Name object from the DB */
                if(!LLD::Register->ReadState(hashRegister, name, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-23, "Invalid address");

                /* Check that the name object is proper type. */
                if(name.nType != TAO::Register::REGISTER::OBJECT
                || !name.Parse()
                || name.Standard() != TAO::Register::OBJECTS::NAME )
                    throw APIException(-23, "Address is not a name register");
            }

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing name / address");


            /* Read the last hash of owner. */
            uint512_t hashLast = 0;
            if(!LLD::Ledger->ReadLast(name.hashOwner, hashLast))
                throw APIException(-24, "No history found");

            /* Iterate through sigchain for register updates. */
            while(hashLast != 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-28, "Failed to read transaction");

                /* Set the next last. */
                hashLast = tx.hashPrevTx;

                /* Check through all the contracts. */
                for(int32_t nContract = tx.Size() - 1; nContract >= 0; --nContract)
                {
                    /* Get the contract. */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Get the operation byte. */
                    uint8_t OPERATION = 0;
                    contract >> OPERATION;

                    /* Check for key operations. */
                    switch(OPERATION)
                    {
                        /* Break when at the register declaration. */
                        case TAO::Operation::OP::CREATE:
                        {
                            /* Get the Address of the Register. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashRegister)
                                break;

                            /* Get the Register Type. */
                            uint8_t nType = 0;
                            contract >> nType;

                            /* Get the register data. */
                            std::vector<uint8_t> vchData;
                            contract >> vchData;

                            if(nType != TAO::Register::REGISTER::OBJECT)
                            {
                                throw APIException(-24, "Specified name/address is not a Name object.");
                            }

                            /* Create the register object. */
                            TAO::Register::Object state;
                            state.nVersion   = 1;
                            state.nType      = nType;
                            state.hashOwner  = contract.Caller();

                            /* Calculate the new operation. */
                            if(!TAO::Operation::Create::Execute(state, vchData, contract.Timestamp()))
                                return false;

                            /* parse object so that the data fields can be accessed */
                            if(!state.Parse())
                                throw APIException(-24, "Failed to parse object register");

                            /* Only include non standard object registers (assets) */
                            if(state.Standard() != TAO::Register::OBJECTS::NAME)
                                throw APIException(-24, "Specified name/address is not a Name object.");

                            /* Generate return object. */
                            json::json obj;
                            obj["type"]     = "CREATE";
                            obj["owner"]    = contract.Caller().ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, false);

                            /* Copy the name data in to the response after the type */
                            obj.insert(data.begin(), data.end());

                            /* Push to return array. */
                            ret.push_back(obj);

                            /* Set hash last to zero to break. */
                            hashLast = 0;

                            break;
                        }


                        case TAO::Operation::OP::WRITE:
                        {
                            /* Get the address. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashRegister)
                                break;

                            /* Get the register data. */
                            std::vector<uint8_t> vchData;
                            contract >> vchData;

                            /* Generate return object. */
                            json::json obj;
                            obj["type"] = "MODIFY";

                            /* Get the flag. */
                            uint8_t nState = 0;
                            contract >>= nState;

                            /* Get the pre-state. */
                            TAO::Register::Object state;
                            contract >>= state;

                            /* Calculate the new operation. */
                            if(!TAO::Operation::Write::Execute(state, vchData, contract.Timestamp()))
                                return false;
                            
                            /* parse object so that the data fields can be accessed */
                            if(!state.Parse())
                                throw APIException(-24, "Failed to parse object register");

                            /* Complete object parameters. */
                            obj["owner"]    = contract.Caller().ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, false);

                            /* Copy the name data in to the response after the type */
                            obj.insert(data.begin(), data.end());


                            /* Push to return array. */
                            ret.push_back(obj);

                            break;
                        }


                        case TAO::Operation::OP::CLAIM:
                        {
                            /* Extract the transaction from contract. */
                            uint512_t hashTx = 0;
                            contract >> hashTx;

                            /* Extract the contract-id. */
                            uint32_t nContract = 0;
                            contract >> nContract;

                            /* Extract the address from the contract. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashRegister)
                                break;

                            /* Generate return object. */
                            json::json obj;
                            obj["type"] = "CLAIM";

                            /* Get the flag. */
                            uint8_t nState = 0;
                            contract >>= nState;

                            /* Get the pre-state. */
                            TAO::Register::Object state;
                            contract >>= state;

                            /* parse object so that the data fields can be accessed */
                            if(!state.Parse())
                                throw APIException(-24, "Failed to parse object register");

                            /* Complete object parameters. */
                            obj["owner"]    = contract.Caller().ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, false);

                            /* Copy the name data in to the response after the type */
                            obj.insert(data.begin(), data.end());

                            /* Push to return array. */
                            ret.push_back(obj);

                            /* Get the previous txid. */
                            hashLast = hashTx;

                            break;
                        }

                        /* Get old owner from transfer. */
                        case TAO::Operation::OP::TRANSFER:
                        {
                            /* Extract the address from the contract. */
                            uint256_t hashAddress = 0;
                            contract >> hashAddress;

                            /* Check for same address. */
                            if(hashAddress != hashRegister)
                                break;

                            /* Read the register transfer recipient. */
                            uint256_t hashTransfer = 0;
                            contract >> hashTransfer;

                            /* Generate return object. */
                            json::json obj;
                            obj["type"] = "TRANSFER";

                            /* Get the flag. */
                            uint8_t nState = 0;
                            contract >>= nState;

                            /* Get the pre-state. */
                            TAO::Register::Object state;
                            contract >>= state;

                            /* parse object so that the data fields can be accessed */
                            if(!state.Parse())
                                throw APIException(-24, "Failed to parse object register");

                            /* Complete object parameters. */
                            obj["owner"]    = hashTransfer.ToString();
                            obj["modified"] = state.nModified;
                            obj["checksum"] = state.hashChecksum;

                            json::json data  =TAO::API::ObjectToJSON(params, state, hashRegister, false);
                            /* Copy the name data in to the response after the type */
                            obj.insert(data.begin(), data.end());

                            /* Push to return array. */
                            ret.push_back(obj);

                            break;
                        }

                        default:
                            continue;
                    }
                }
            }

            return ret;
        }
    }
}
