/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <cmath>

#include <LLD/include/global.h>

#include <TAO/Operation/include/validate.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/objects/account.h>
#include <TAO/Register/objects/token.h>

namespace TAO
{

    namespace Operation
    {

        /** Execute
         *
         *  Execute the validation script.
         *
         **/
        bool Validate::Execute()
        {
            /* Keep track of previous execution return value. */
            bool fRet = false;

            /* Loop through the operation validation code. */
            while(!ssOperations.end())
            {
                /* Grab the first value */
                TAO::Register::Value vFirst;
                if(!GetValue(vFirst))
                    return false;

                /* Grab the next operation. */
                uint8_t OPERATION;
                ssOperations >> OPERATION;

                /* Switch by operation code. */
                switch(OPERATION)
                {

                    /* Handle for the == operator. */
                    case OP::EQUALS:
                    {
                        /* Grab the second value. */
                        TAO::Register::Value vSecond;
                        if(!GetValue(vSecond))
                            return false;

                        /* Compare both values to one another. */
                        fRet = (compare(vFirst, vSecond) == 0);

                        /* Deallocate the values from the VM. */
                        deallocate(vSecond);
                        deallocate(vFirst);

                        break;
                    }


                    /* Handle for < operator. */
                    case OP::LESSTHAN:
                    {
                        /* Grab the second value. */
                        TAO::Register::Value vSecond;
                        if(!GetValue(vSecond))
                            return false;

                        /* Compare both values to one another. */
                        fRet = (compare(vFirst, vSecond) < 0);

                        /* Deallocate the values from the VM. */
                        deallocate(vSecond);
                        deallocate(vFirst);

                        break;
                    }


                    /* Handle for the > operator. */
                    case OP::GREATERTHAN:
                    {
                        /* Grab the second value. */
                        TAO::Register::Value vSecond;
                        if(!GetValue(vSecond))
                            return false;

                        /* Compare both values to one another. */
                        fRet = (compare(vFirst, vSecond) > 0);

                        /* Deallocate the values from the VM. */
                        deallocate(vSecond);
                        deallocate(vFirst);

                        break;
                    }


                    /* Handle for the != operator. */
                    case OP::NOTEQUALS:
                    {
                        /* Grab the second value. */
                        TAO::Register::Value vSecond;
                        if(!GetValue(vSecond))
                            return false;

                        /* Compare both values to one another. */
                        fRet = (compare(vFirst, vSecond) != 0);

                        /* Deallocate the values from the VM. */
                        deallocate(vSecond);
                        deallocate(vFirst);

                        break;
                    }


                    /* Handle to check if a sequence of bytes is inside another. */
                    case OP::CONTAINS:
                    {
                        /* Grab the second value. */
                        TAO::Register::Value vSecond;
                        if(!GetValue(vSecond))
                            return false;

                        /* Compare both values to one another. */
                        fRet = contains(vFirst, vSecond);

                        /* Deallocate the values from the VM. */
                        deallocate(vSecond);
                        deallocate(vFirst);

                        break;
                    }


                    /* Handle for the && operator. */
                    case OP::AND:
                    {
                        /* Recursively call for next statement. */
                        return fRet && Execute();
                    }


                    //OP::GROUP or the ( operator
                    //OP::UNGROUP or the ) operator

                    /** Handle for the || operator. */
                    case OP::OR:
                    {
                        /* Short circuit out early if possible. */
                        if(fRet)
                            return true;

                        /* Recursively call for next statement. */
                        return Execute();
                    }

                    /* For unknown codes, always fail. */
                    default:
                    {
                        return false;
                    }
                }
            }

            /* Return final response. */
            return fRet;
        }


        /** get value
         *
         *  Get a value from the register virtual machine.
         *
         **/
        bool Validate::GetValue(TAO::Register::Value& vRet)
        {

            /* Iterate until end of stream. */
            while(!ssOperations.end())
            {

                /* Extract the operation byte. */
                uint8_t OPERATION;
                ssOperations >> OPERATION;


                /* Switch based on the operation. */
                switch(OPERATION)
                {

                    /* Add two 64-bit numbers. */
                    case OP::ADD:
                    {
                        /* Get the add from r-value. */
                        TAO::Register::Value vAdd;
                        if(!GetValue(vAdd))
                            return false;

                        /* Check computational bounds. */
                        if(vAdd.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::ADD computation greater than 64-bits"));

                        /* Compute the return value. */
                        at(vRet) += at(vAdd);

                        /* Deallocate r-value from memory. */
                        deallocate(vAdd);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

                        break;
                    }


                    /* Subtract one number from another. */
                    case OP::SUB:
                    {
                        /* Get the sub from r-value. */
                        TAO::Register::Value vSub;
                        if(!GetValue(vSub))
                            return false;

                        /* Check computational bounds. */
                        if(vSub.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::SUB computation greater than 64-bits"));

                        /* Compute the return value. */
                        at(vRet) -= at(vSub);

                        /* Deallocate r-value from memory. */
                        deallocate(vSub);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

                        break;
                    }


                    /* Increment a number by an order of 1. */
                    case OP::INC:
                    {
                        /* Check computational bounds. */
                        if(vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::INC computation greater than 64-bits"));

                        /* Compute the return value. */
                        ++at(vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

                        break;
                    }


                    /* De-increment a number by an order of 1. */
                    case OP::DEC:
                    {
                        /* Check computational bounds. */
                        if(vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::DEC computation greater than 64-bits"));

                        /* Compute the return value. */
                        --at(vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

                        break;
                    }


                    /* Divide a number by another. */
                    case OP::DIV:
                    {
                        /* Get the divisor from r-value. */
                        TAO::Register::Value vDiv;
                        if(!GetValue(vDiv))
                            return false;

                        /* Check computational bounds. */
                        if(vDiv.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::DIV computation greater than 64-bits"));

                        /* Compute the return value. */
                        at(vRet) /= at(vDiv);

                        /* Deallocate r-value from memory. */
                        deallocate(vDiv);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Multiply a number by another. */
                    case OP::MUL:
                    {
                        /* Get the multiplier from r-value. */
                        TAO::Register::Value vMul;
                        if(!GetValue(vMul))
                            return false;

                        /* Check computational bounds. */
                        if(vMul.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::MUL computation greater than 64-bits"));

                        /* Compute the return value. */
                        at(vRet) *= at(vMul);

                        /* Deallocate r-value from memory. */
                        deallocate(vMul);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Raise a number by the power of another. */
                    case OP::EXP:
                    {
                        /* Get the exponent from r-value. */
                        TAO::Register::Value vExp;
                        if(!GetValue(vExp))
                            return false;

                        /* Check computational bounds. */
                        if(vExp.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::EXP computation greater than 64-bits"));

                        /* Compute the return value. */
                        at(vRet) = pow(at(vRet), at(vExp));

                        /* Deallocate r-value from memory. */
                        deallocate(vExp);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 256;

                        break;
                    }


                    /* Get the remainder after a division. */
                    case OP::MOD:
                    {
                        /* Get the modulus from r-value. */
                        TAO::Register::Value vMod;
                        if(!GetValue(vMod))
                            return false;

                        /* Check computational bounds. */
                        if(vMod.size() > 1 || vRet.size() > 1)
                            throw std::runtime_error(debug::safe_printstr("OP::MOD computation greater than 64-bits"));

                        /* Compute the return value. */
                        at(vRet) %= at(vMod);

                        /* Deallocate r-value from memory. */
                        deallocate(vMod);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Parse out subdata from bytes. */
                    case OP::SUBDATA:
                    {
                        /* Get the beginning iterator. */
                        uint16_t nBegin;
                        ssOperations >> nBegin;

                        /* Get the size to extract. */
                        uint16_t nSize;
                        ssOperations >> nSize;

                        /* Extract the string. */
                        std::vector<uint8_t> vData(vRet.size() * 8, 0);
                        deallocate(vData, vRet);

                        /* Set the register value. */
                        std::vector<uint8_t> vAlloc(vData.begin() + nBegin, vData.begin() + nBegin + nSize);
                        allocate(vAlloc, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= vData.size();

                        break;
                    }


                    /* Extract an uint8_t from the stream. */
                    case OP::TYPES::UINT8_T:
                    {
                        /* Extract the byte. */
                        uint8_t n;
                        ssOperations >> n;

                        /* Set the register value. */
                        allocate((uint64_t)n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 1;

                        break;
                    }


                    /* Extract an uint16_t from the stream. */
                    case OP::TYPES::UINT16_T:
                    {
                        /* Extract the short. */
                        uint16_t n;
                        ssOperations >> n;

                        /* Set the register value. */
                        allocate((uint64_t)n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 2;

                        break;
                    }


                    /* Extract an uint32_t from the stream. */
                    case OP::TYPES::UINT32_T:
                    {
                        /* Extract the integer. */
                        uint32_t n;
                        ssOperations >> n;

                        /* Set the register value. */
                        allocate((uint64_t)n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 4;

                        break;
                    }


                    /* Extract an uint64_t from the stream. */
                    case OP::TYPES::UINT64_T:
                    {
                        /* Extract the integer. */
                        uint64_t n;
                        ssOperations >> n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 8;

                        break;
                    }


                    /* Extract an uint256_t from the stream. */
                    case OP::TYPES::UINT256_T:
                    {
                        /* Extract the integer. */
                        uint256_t n;
                        ssOperations >> n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 32;

                        break;

                    }


                    /* Extract an uint512_t from the stream. */
                    case OP::TYPES::UINT512_T:
                    {
                        /* Extract the integer. */
                        uint512_t n;
                        ssOperations >> n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

                        break;
                    }


                    /* Extract an uint1024_t from the stream. */
                    case OP::TYPES::UINT1024_T:
                    {
                        /* Extract the integer. */
                        uint1024_t n;
                        ssOperations >> n;

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Extract a string from the stream. */
                    case OP::TYPES::STRING:
                    {
                        /* Extract the string. */
                        std::string str;
                        ssOperations >> str;

                        /* Set the register value. */
                        allocate(str, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= str.size();

                        break;
                    }


                    /* Extract bytes from the stream. */
                    case OP::TYPES::BYTES:
                    {
                        /* Extract the string. */
                        std::vector<uint8_t> vData;
                        ssOperations >> vData;

                        /* Set the register value. */
                        allocate(vData, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= vData.size();

                        break;
                    }


                    /* Get a unified timestamp and push to return value. */
                    case OP::GLOBAL::UNIFIED:
                    {
                        /* Get the current unified timestamp. */
                        uint64_t n = runtime::unifiedtimestamp();

                        /* Set the register value. */
                        allocate(n, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 8;

                        break;
                    }


                    /* Get a register's timestamp and push to the return value. */
                    case OP::REGISTER::TIMESTAMP:
                    {
                        /* Read the register address. */
                        uint256_t hashRegister;
                        deallocate(hashRegister, vRet);

                        /* Read the register states. */
                        TAO::Register::State state;
                        if(!LLD::regDB->Read(hashRegister, state))
                            return false;

                        /* Set the register value. */
                        allocate(state.nTimestamp, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 40;

                        break;
                    }


                    /* Get a register's owner and push to the return value. */
                    case OP::REGISTER::OWNER:
                    {
                        /* Read the register address. */
                        uint256_t hashRegister;
                        deallocate(hashRegister, vRet);

                        /* Read the register states. */
                        TAO::Register::State state;
                        if(!LLD::regDB->Read(hashRegister, state))
                            return false;

                        /* Set the register value. */
                        allocate(state.hashOwner, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 64;

                        break;
                    }


                    /* Get a register's type and push to the return value. */
                    case OP::REGISTER::TYPE:
                    {
                        /* Read the register address. */
                        uint256_t hashRegister;
                        deallocate(hashRegister, vRet);

                        /* Read the register states. */
                        TAO::Register::State state;
                        if(!LLD::regDB->Read(hashRegister, state))
                            return false;

                        /* Push the type onto the return value. */
                        allocate((uint64_t)state.nType, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 33;

                        break;
                    }


                    /* Get a register's state and push to the return value. */
                    case OP::REGISTER::STATE:
                    {
                        /* Read the register address. */
                        uint256_t hashRegister;
                        deallocate(hashRegister, vRet);

                        /* Read the register states. */
                        TAO::Register::State state;
                        if(!LLD::regDB->Read(hashRegister, state))
                            return false;

                        /* Allocate to the registers. */
                        allocate(state.GetState(), vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Get an account register's balance and push to the return value. */
                    case OP::REGISTER::BALANCE:
                    {
                        /* Read the register address. */
                        uint256_t hashRegister;
                        deallocate(hashRegister, vRet);

                        /* Read the register states. */
                        TAO::Register::State state;
                        if(!LLD::regDB->Read(hashRegister, state))
                            return false;

                        /* Check for account type. */
                        if(state.nType == TAO::Register::OBJECT::ACCOUNT)
                        {
                            /* Get the account object from register. */
                            TAO::Register::Account account;
                            state >> account;

                            /* Allocate to the registers. */
                            allocate(account.nBalance, vRet);
                        }

                        /* Check for token type. */
                        else if(state.nType == TAO::Register::OBJECT::TOKEN)
                        {
                            /* Get the account object from register. */
                            TAO::Register::Token token;
                            state >> token;

                            /* Allocate to the registers. */
                            allocate(token.nBalance, vRet);
                        }
                        else
                            return false;

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Get an account register's balance and push to the return value. */
                    case OP::REGISTER::IDENTIFIER:
                    {
                        /* Read the register address. */
                        uint256_t hashRegister;
                        deallocate(hashRegister, vRet);

                        /* Read the register states. */
                        TAO::Register::State state;
                        if(!LLD::regDB->Read(hashRegister, state))
                            return false;

                        /* Check for account type. */
                        if(state.nType == TAO::Register::OBJECT::ACCOUNT)
                        {
                            /* Get the account object from register. */
                            TAO::Register::Account account;
                            state >> account;

                            /* Allocate to the registers. */
                            allocate(account.nIdentifier, vRet);
                        }

                        /* Check for token type. */
                        else if(state.nType == TAO::Register::OBJECT::TOKEN)
                        {
                            /* Get the account object from register. */
                            TAO::Register::Token token;
                            state >> token;

                            /* Allocate to the registers. */
                            allocate(token.nIdentifier, vRet);
                        }
                        else
                            return false;

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Get the genesis id of the transaction caller. */
                    case OP::CALLER::GENESIS:
                    {
                        /* Allocate to the registers. */
                        allocate(tx.hashGenesis, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 128;

                        break;
                    }


                    /* Get the timestamp of the transaction caller. */
                    case OP::CALLER::TIMESTAMP:
                    {
                        /* Allocate to the registers. */
                        allocate(tx.nTimestamp, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 1;

                        break;
                    }


                    /* Get the operations of the transaction caller. */
                    case OP::CALLER::OPERATIONS:
                    {
                        /* Allocate to the registers. */
                        allocate(tx.ssOperation.Bytes(), vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= tx.ssOperation.Bytes().size();

                        break;
                    }


                    /* Get the current height of the chain. */
                    case OP::LEDGER::HEIGHT:
                    {
                        /* Allocate to the registers. */
                        allocate((uint64_t)TAO::Ledger::ChainState::stateBest.load().nHeight, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 8;

                        break;
                    }


                    /* Get the current supply of the chain. */
                    case OP::LEDGER::SUPPLY:
                    {
                        /* Allocate to the registers. */
                        allocate((uint64_t)TAO::Ledger::ChainState::stateBest.load().nMoneySupply, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 8;

                        break;
                    }

                    case OP::LEDGER::TIMESTAMP:
                    {
                        /* Allocate to the registers. */
                        allocate((uint64_t)TAO::Ledger::ChainState::stateBest.load().nTime, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 8;

                        break;
                    }


                    /* Compute an SK256 hash of current return value. */
                    case OP::CRYPTO::SK256:
                    {
                        /* Check for hash input availability. */
                        if(vRet.null())
                            throw std::runtime_error("OP::CRYPTO::SK256: can't hash with no input");

                        /* Compute the return hash. */
                        uint256_t hash = LLC::SK256((uint8_t*)begin(vRet), (uint8_t*)end(vRet));

                        /* Free the memory for old ret. */
                        deallocate(vRet);

                        /* Copy new hash into return value. */
                        allocate(hash, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 512;

                        break;
                    }


                    /* Compute an SK512 hash of current return value. */
                    case OP::CRYPTO::SK512:
                    {
                        /* Check for hash input availability. */
                        if(vRet.null())
                            throw std::runtime_error("OP::CRYPTO::SK512: can't hash with no input");

                        /* Compute the return hash. */
                        uint512_t hash = LLC::SK512((uint8_t*)begin(vRet), (uint8_t*)end(vRet));

                        /* Free the memory for old ret. */
                        deallocate(vRet);

                        /* Copy new hash into return value. */
                        allocate(hash, vRet);

                        /* Reduce the limits to prevent operation exhuastive attacks. */
                        nLimits -= 1024;

                        break;
                    }


                    default:
                    {
                        /* If no applicable instruction found, rewind and return. */
                        ssOperations.seek(-1);
                        if(nLimits < 0)
                            debug::error(FUNCTION, "out of computational limits ", nLimits);

                        return (nLimits >= 0);
                    }
                }
            }

            /* Log if computational limits expired. */
            if(nLimits < 0)
                debug::error(FUNCTION, "out of computational limits ", nLimits);

            return (nLimits >= 0);
        }
    }
}
