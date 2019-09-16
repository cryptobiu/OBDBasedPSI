//
// Created by moriya on 7/24/19.
//

#ifndef OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
#define OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
#include <libscapi/include/comm/MPCCommunication.hpp>
#include <libscapi/include/cryptoInfra/Protocol.hpp>
#include <libscapi/include/infra/Measurement.hpp>


#include <cryptoTools/Network/IOService.h>
#include <libOTe/NChooseOne/Oos/OosNcoOtReceiver.h>
#include <libOTe/NChooseOne/Oos/OosNcoOtSender.h>
#include <libOTe/Tools/Tools.h>
#include <libOTe/Tools/LinearCode.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/Endpoint.h>
#include <cryptoTools/Common/Log.h>
#include <thread>



using namespace osuCrypto;

#include "ObliviousDictionary.h"

class ProtocolParty : public Protocol {

protected:


    int partyId;
    int times; //number of times to run the run function
    int iteration; //number of the current iteration
    int hashSize, fieldSize, fieldSizeBytes;
    int reportStatistics = 0;

    Measurement *timer;
    shared_ptr<ProtocolPartyData> otherParty;
    boost::asio::io_service io_service;

public:

    ProtocolParty(int argc, char *argv[]);
    ~ProtocolParty();

    void run() override;

    bool hasOffline() override {
        return false;
    }


    bool hasOnline() override {
        return true;
    }

    void runOffline() override {}


};

class Receiver : public ProtocolParty {
private :
    ObliviousDictionary* dic;

    vector<byte> createDictionary();
    void runOT(vector<byte> & sigma);
public:

    Receiver(int argc, char *argv[]);
    ~Receiver();

    void runOnline() override;
};

class Sender : public ProtocolParty {
private :
    void runOT();
public:

    Sender(int argc, char *argv[]);
    ~Sender();
    void runOnline() override;


};


#endif //OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
