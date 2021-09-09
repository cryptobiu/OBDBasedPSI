//
// Created by moriya on 7/24/19.
//

#ifndef OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
#define OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
#include <cryptoTools/Network/IOService.h>
#include <libPSI/PsiDefines.h>
#include <libPSI/PRTY2/PrtyMOtReceiver.h>
#include <libPSI/PRTY2/PrtyMOtSender.h>
#include <libPSI/Tools/mx_72_by_462.h>
#include <libPSI/Tools/mx_84_by_495.h>
#include <libPSI/Tools/mx_90_by_495.h>
#include <libPSI/Tools/mx_65_by_448.h>
#include <libPSI/Tools/mx_132_by_583.h>
#include <libPSI/Tools/mx_138_by_594.h>
#include <libPSI/Tools/mx_144_by_605.h>
#include <libPSI/Tools/mx_150_by_616.h>
#include <libPSI/Tools/mx_156_by_627.h>
#include <libPSI/Tools/mx_162_by_638.h>
#include <libPSI/Tools/mx_168_by_649.h>
#include <libPSI/Tools/mx_174_by_660.h>
#include <libPSI/Tools/mx_210_by_732.h>
#include <libPSI/Tools/mx_217_by_744.h>
#include <libPSI/Tools/mx_231_by_768.h>
#include <libPSI/Tools/mx_238_by_776.h>
#include <libOTe/Tools/Tools.h>
#include <libOTe/Tools/LinearCode.h>
//#include <cryptoTools/Network/Channel.h>
//#include <cryptoTools/Network/Endpoint.h>
//#include <cryptoTools/Common/Log.h>

#include <libscapi/include/comm/MPCCommunication.hpp>
#include <libscapi/include/cryptoInfra/Protocol.hpp>
#include <libscapi/include/infra/Measurement.hpp>


#include <string.h>
#include <thread>
#include <bitset>



using namespace osuCrypto;

#include "ObliviousDictionary.h"

class ProtocolParty : public Protocol {

protected:


    int partyId;
    int times; //number of times to run the run function
    int iteration; //number of the current iteration
    int numOTs, tableRealSize, hashSize, fieldSize, fieldSizeBytes;
    int reportStatistics = 0;
    int gamma;
    bool isMalicious;
    string version;
    int q;

    string addressForOT;
    int portForOT;

    PrgFromOpenSSLAES prg;
    EVP_CIPHER_CTX* aes;

    vector<uint64_t> keys;
    vector<unsigned char> vals;
    ObliviousDictionary* dic;

    Measurement *timer;
    shared_ptr<ProtocolPartyData> otherParty;
    boost::asio::io_service io_service;

    virtual void preprocess(vector<vector<block>>& lookupTable, int smallTableStartIndex, int blockSize) = 0;
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
    PrtyMOtReceiver recv;
    unordered_set<uint64_t> xorsSet;

    vector<unsigned char> createDictionary();

    void runOOS(vector<unsigned char> & sigma);

    void computeXors();
    void checkVariables(vector<unsigned char> & variables);

    void receiveSenderXors();

    void preprocess(vector<vector<block>>& lookupTable, int smallTableStartIndex, int blockSize) override;
    void computeTablesXors();
    void computeStarXors();

public:

    Receiver(int argc, char *argv[]);
    ~Receiver();

    void runOnline() override;
};

class Sender : public ProtocolParty {
private :
    PrtyMOtSender sender;
    BitVector baseChoice;
    LinearCode code;

    vector<uint64_t> xors;

    void runOOS();

    void computeXors();
    void computeTablesXors();
    void computeStarXors();

    void sendXors();

    void preprocess(vector<vector<block>>& lookupTable, int smallTableStartIndex, int blockSize) override;
public:

    Sender(int argc, char *argv[]);
    ~Sender();
    void runOnline() override;


};


#endif //OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
