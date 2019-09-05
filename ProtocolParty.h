//
// Created by moriya on 7/24/19.
//

#ifndef OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
#define OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
#include <libscapi/include/comm/MPCCommunication.hpp>
#include <libscapi/include/cryptoInfra/Protocol.hpp>
#include <libscapi/include/infra/Measurement.hpp>
#include "ObliviousDictionary.h"

class ProtocolParty : public Protocol {

protected:


    int partyId;
    int times; //number of times to run the run function
    int iteration; //number of the current iteration
    int hashSize;
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

class DBParty : public ProtocolParty {
private :
    ObliviousDictionaryDB* dic;
public:

    DBParty(int argc, char *argv[]);
    ~DBParty();

    void runOnline() override;
};

class QueryParty : public ProtocolParty {
private :
    ObliviousDictionaryQuery* dic;
public:

    QueryParty(int argc, char *argv[]);
    ~QueryParty();
    void runOnline() override;


};


#endif //OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
