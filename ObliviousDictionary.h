//
// Created by moriya on 7/17/19.
//

#ifndef BENNYPROJECT_OBLIVIOUSDICTIONARY_H
#define BENNYPROJECT_OBLIVIOUSDICTIONARY_H

#include <unordered_set>
#include <unordered_map>
#include <libscapi/include/primitives/Prg.hpp>
#include <libscapi/include/comm/MPCCommunication.hpp>
#include "Poly.h"
#include "Hasher.h"
#include <NTL/mat_GF2E.h>
#include <NTL/GF2E.h>
#include <NTL/GF2X.h>
#include <NTL/GF2XFactoring.h>

#include <chrono>
class Tools;

using namespace std::chrono;

using namespace std;

class ObliviousDictionary {
protected:

    int hashSize;
    int tableRealSize;
    int gamma;

    PrgFromOpenSSLAES prg;
    vector<uint64_t> keys;

    uint64_t firstSeed, secondSeed, dhSeed;;

    unordered_set<uint64_t, Hasher> first;
    unordered_set<uint64_t, Hasher> second;

    Hasher DH;

    vector<uint64_t> variables;

    Tools* tool;

    vector<uint64_t> dec(uint64_t key);
    uint64_t getDHBits(uint64_t key);

public:

    ObliviousDictionary(int hashSize) : hashSize(hashSize){
        gamma = 60;
    }
//    virtual ~ObliviousDictionary(){
//        delete tool;
//    }

    void createSets();
};


class ObliviousDictionaryDB : public ObliviousDictionary{
protected:
    unordered_map<uint64_t, uint64_t> vals;

    vector<uint64_t> peelingVector;
    int peelingCounter;
    int reportStatistics=0;
    ofstream statisticsFile;

public:

    ObliviousDictionaryDB(int hashSize);

    virtual ~ObliviousDictionaryDB() {
        if (reportStatistics == 1) {

            statisticsFile.close();
        }
    };

    void fillTables();

    void peeling();

    void generateExternalToolValues();

    void unpeeling();

    void checkOutput();

    bool hasLoop();

    void sendData(shared_ptr<ProtocolPartyData> otherParty);

    void setReportStatstics(int flag){
        reportStatistics = flag;
        if (reportStatistics == 1) {

            cout<<"statistics file created"<<endl;
            statisticsFile.open("statistics.csv");
            statisticsFile << "-------------Statistics-----------.\n";
        }};

    void init();
};

class ObliviousDictionaryQuery : public ObliviousDictionary{
private:


public:

    ObliviousDictionaryQuery(int hashSize);

    virtual void readData(shared_ptr<ProtocolPartyData> otherParty);

    virtual void calcRealValues();

    virtual void output();
};


#endif //BENNYPROJECT_OBLIVIOUSDICTIONARY_H
