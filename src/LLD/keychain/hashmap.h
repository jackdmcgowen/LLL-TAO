/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_KEYCHAIN_HASHMAP_H
#define NEXUS_LLD_KEYCHAIN_HASHMAP_H

#include <LLD/keychain/keychain.h>
#include <LLD/cache/template_lru.h>
#include <LLD/include/enum.h>
#include <LLD/config/hashmap.h>

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <mutex>

namespace LLD
{
    //forward declaration
    class BloomFilter;


    /** BinaryHashMap
     *
     *  This class is responsible for managing the keys to the sector database.
     *
     *  It contains a Binary Hash Map with a minimum complexity of O(1).
     *  It uses a linked file list based on index to iterate trhough files and binary Positions
     *  when there is a collision that is found.
     *
     **/
    class BinaryHashMap : public Keychain
    {
    protected:

        /** Mutex for Thread Synchronization. **/
        mutable std::mutex KEY_MUTEX;


        /** Internal Hashmap Config Object. **/
        const Config::Hashmap& CONFIG;


        /** Internal pre-calculated index size. **/
        const uint16_t INDEX_FILTER_SIZE;


        /** Keychain stream object. **/
        TemplateLRU<uint16_t, std::fstream*>* pFileStreams;


        /** Keychain index stream. **/
        std::fstream* pindex;


    public:


        /** Default Constructor. **/
        BinaryHashMap() = delete;


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        BinaryHashMap(const Config::Hashmap& config);


        /** Copy Constructor **/
        BinaryHashMap(const BinaryHashMap& map);


        /** Move Constructor **/
        BinaryHashMap(BinaryHashMap&& map);


        /** Copy Assignment Operator **/
        BinaryHashMap& operator=(const BinaryHashMap& map);


        /** Move Assignment Operator **/
        BinaryHashMap& operator=(BinaryHashMap&& map);


        /** Default Destructor **/
        virtual ~BinaryHashMap();


        /** Initialize
         *
         *  Initialize the binary hash map keychain.
         *
         **/
        void Initialize();


        /** Get
         *
         *  Read a key index from the disk hashmaps.
         *
         *  @param[in] vKey The binary data of key.
         *  @param[out] cKey The key object to return.
         *
         *  @return True if the key was found, false otherwise.
         *
         **/
        bool Get(const std::vector<uint8_t>& vKey, SectorKey &cKey);


        /** Put
         *
         *  Write a key to the disk hashmaps.
         *
         *  @param[in] cKey The key object to write.
         *
         *  @return True if the key was written, false otherwise.
         *
         **/
        bool Put(const SectorKey& cKey);


        /** Flush
         *
         *  Flush all buffers to disk if using ACID transaction.
         *
         **/
        void Flush();


        /** Restore
         *
         *  Restore an erased key from keychain.
         *
         *  @param[in] vKey the key to restore.
         *
         *  @return True if the key was restored.
         *
         **/
        bool Restore(const std::vector<uint8_t>& vKey);


        /** Erase
         *
         *  Erase a key from the disk hashmaps.
         *  TODO: This should be optimized further.
         *
         *  @param[in] vKey the key to erase.
         *
         *  @return True if the key was erased, false otherwise.
         *
         **/
        bool Erase(const std::vector<uint8_t>& vKey);


    private:

        /** compress_key
         *
         *  Compresses a given key until it matches size criteria.
         *  This function is one way and efficient for reducing key sizes.
         *
         *  @param[out] vData The binary data of key to compress.
         *  @param[in] nSize The desired size of key after compression.
         *
         **/
        void compress_key(std::vector<uint8_t>& vData, uint16_t nSize = 16);


        /** get_bucket
         *
         *  Calculates a bucket to be used for the hashmap allocation.
         *
         *  @param[in] vKey The key object to calculate with.
         *
         *  @return The bucket assigned to the key.
         *
         **/
        uint32_t get_bucket(const std::vector<uint8_t>& vKey);


        /** get_current_file
         *
         *  Helper method to grab the current file from the input buffer.
         *
         *  @param[in] vBuffer The byte stream of input data
         *  @param[in] nOffset The starting offset in the stream.
         *
         *  @return The current hashmap file for given key index.
         *
         **/
        uint16_t get_current_file(const std::vector<uint8_t>& vBuffer, const uint32_t nOffset = 0);


        /** set_current_file
         *
         *  Helper method to set the current file in the input buffer.
         *
         *  @param[in] nCurrent The current file to set
         *  @param[out] vBuffer The byte stream of output data
         *  @param[in] nOffset The starting offset in the stream.
         *
         **/
        void set_current_file(const uint16_t nCurrent, std::vector<uint8_t> &vBuffer, const uint32_t nOffset = 0);


        /** check_hashmap_available
         *
         *  Helper method to check if a file is unoccupied in input buffer.
         *
         *  @param[in] nHashmap The current file to set
         *  @param[in] vBuffer The byte stream of input data
         *  @param[in] nOffset The starting offset in the stream.
         *
         **/
        bool check_hashmap_available(const uint32_t nHashmap, const std::vector<uint8_t>& vBuffer, const uint32_t nOffset = 0);


        /** secondary_bloom_size
         *
         *  Helper method to get the total bytes in the secondary bloom filter.
         *
         *  @return The total bytes in secondary bloom filter.
         *
         **/
        uint32_t secondary_bloom_size();


        /** set_secondary_bloom
         *
         *  Set a given key is within a secondary bloom filter.
         *
         *  @param[in] vKey The key level data in bytes
         *  @param[out] vBuffer The buffer to write bloom filter into
         *  @param[in] nHashmap The hashmap file that we are writing for
         *  @param[in] nOffset The starting offset in the stream
         *
         *
         **/
        void set_secondary_bloom(const std::vector<uint8_t>& vKey, std::vector<uint8_t> &vBuffer, const uint32_t nHashmap, const uint32_t nOffset = 0);


        /** check_secondary_bloom
         *
         *  Check if a given key is within a secondary bloom filter.
         *
         *  @param[in] vKey The key level data in bytes
         *  @param[in] vBuffer The buffer to check bloom filter within
         *  @param[in] nHashmap The hashmap file that we are checking for
         *  @param[in] nOffset The starting offset in the stream
         *
         *  @return True if given key is in secondary bloom, false otherwise.
         *
         **/
        bool check_secondary_bloom(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vBuffer, const uint32_t nHashmap, const uint32_t nOffset = 0);


        /** primary_bloom_size
         *
         *  Helper method to get the total bytes in the primary bloom filter.
         *
         *  @return The total bytes in primary bloom filter.
         *
         **/
        uint32_t primary_bloom_size();


        /** set_primary_bloom
         *
         *  Set a given key is within a primary bloom filter.
         *
         *  @param[in] vKey The key level data in bytes
         *  @param[out] vBuffer The buffer to write bloom filter into
         *  @param[in] nOffset The starting offset in the stream
         *
         *
         **/
        void set_primary_bloom(const std::vector<uint8_t>& vKey, std::vector<uint8_t> &vBuffer, const uint32_t nOffset = 0);


        /** check_primary_bloom
         *
         *  Check if a given key is within a primary bloom filter.
         *
         *  @param[in] vKey The key level data in bytes
         *  @param[in] vBuffer The buffer to check bloom filter within
         *  @param[in] nOffset The starting offset in the stream
         *
         *  @return True if given key is in secondary bloom, false otherwise.
         *
         **/
        bool check_primary_bloom(const std::vector<uint8_t>& vKey, const std::vector<uint8_t>& vBuffer, const uint32_t nOffset = 0);
    };
}

#endif
