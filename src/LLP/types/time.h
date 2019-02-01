/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_TYPES_TIME_H
#define NEXUS_LLP_TYPES_TIME_H

#include <LLP/templates/connection.h>

namespace LLP
{
    class TimeNode : public Connection
    {
        enum
        {
            /** DATA PACKETS **/
            TIME_DATA     = 0,
            ADDRESS_DATA  = 1,
            TIME_OFFSET   = 2,

            /** DATA REQUESTS **/
            GET_OFFSET    = 64,


            /** REQUEST PACKETS **/
            GET_TIME      = 129,
            GET_ADDRESS   = 130,


            /** GENERIC **/
            PING          = 253,
            CLOSE         = 254
        };

    public:

        /** Constructor **/
        TimeNode()
        : Connection()
        {
        }


        /** Constructor **/
        TimeNode( Socket_t SOCKET_IN, DDOS_Filter* DDOS_IN, bool isDDOS = false )
        : Connection( SOCKET_IN, DDOS_IN )
        {
        }

        /** Event
         *
         *  Virtual Functions to Determine Behavior of Message LLP.
         *
         *  @param[in] EVENT The byte header of the event type.
         *  @param[in[ LENGTH The size of bytes read on packet read events.
         *
         */
        void Event(uint8_t EVENT, uint32_t LENGTH = 0);


        /** ProcessPacket
         *
         *  Main message handler once a packet is recieved.
         *
         *  @return True is no errors, false otherwise.
         *
         **/
        bool ProcessPacket();
    };
}

#endif
