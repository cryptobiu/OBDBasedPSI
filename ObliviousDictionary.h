//
// Created by moriya on 7/17/19.
//

#ifndef BENNYPROJECT_OBLIVIOUSDICTIONARY_H
#define BENNYPROJECT_OBLIVIOUSDICTIONARY_H

#include <unordered_set>
#include <unordered_map>
#include <libscapi/include/primitives/Prg.hpp>
#include <libscapi/include/comm/MPCCommunication.hpp>
#include "Hasher.h"
#include <NTL/mat_GF2E.h>
#include <NTL/GF2E.h>
#include <NTL/GF2X.h>
#include <NTL/GF2XFactoring.h>
#include "gf2e_mat_solve.h"

#include <chrono>
#include <queue>

using namespace std::chrono;

using namespace std;
using namespace NTL;


class ObliviousDictionary {
protected:

    int hashSize;
    int fieldSize, fieldSizeBytes;
    int gamma, v;

    PrgFromOpenSSLAES prg;
    vector<uint64_t> keys;
    vector<byte> values;

    unordered_map<uint64_t, GF2E> vals;

    int reportStatistics=0;
    ofstream statisticsFile;

    GF2EVector variables;
    vector<byte> sigma;


public:

    ObliviousDictionary(int hashSize, int fieldSize, int gamma, int v);

     ~ObliviousDictionary(){
         if (reportStatistics == 1) {

             statisticsFile.close();
         }
    }

    virtual void setKeysAndVals(vector<uint64_t>& keys, vector<byte>& values){
         this->keys = keys;
         this->values = values;

//        for (int i=0; i<hashSize; i++){
//            cout << "key = " << keys[i] << " val = ";
//
//            for (int j=0; j<fieldSizeBytes; j++){
//                cout<<(int)(values[i*fieldSizeBytes + j])<<" ";
//            }
//            cout<<endl;
//        }
     }

    virtual void init();

    virtual vector<uint64_t> dec(uint64_t key) = 0;

    virtual vector<byte> decode(uint64_t key) = 0;

    virtual bool encode() = 0;

    void generateRandomEncoding() {

        sigma.resize(variables.size()*fieldSizeBytes);
        prg.getPRGBytes(sigma, 0, sigma.size());


    }

    void setReportStatstics(int flag){
        reportStatistics = flag;
        if (reportStatistics == 1) {

            cout<<"statistics file created"<<endl;
            statisticsFile.open("statistics.csv");
            statisticsFile << "-------------Statistics-----------.\n";
        }};

    virtual vector<byte> getVariables() {
        if (sigma.size() == 0) { //If the variables do not randomly chosen
            sigma.resize(variables.size() * fieldSizeBytes);
            for (int i = 0; i < variables.size(); i++) {
                //            cout<<"variables["<<i<<"] = "<<variables[i]<<endl;
                BytesFromGF2X(sigma.data() + i * fieldSizeBytes, rep(variables[i]), fieldSizeBytes);
                //            for (int j=0; j<fieldSizeBytes; j++){
                //                cout<<(int)*(sigma.data() + i*fieldSizeBytes + j)<< " ";
                //            }
                //            cout<<endl;
            }
        }

        return sigma;
    }

    virtual bool checkOutput() = 0;

    int getHashSize(){return hashSize;}
    virtual int getTableSize() = 0;

};

class OBDTables : public ObliviousDictionary{
protected:

    int tableRealSize;

    uint64_t dhSeed;


    vector<uint64_t> peelingVector;
    int peelingCounter;

    Hasher DH;

    vector<byte> sign;

    uint64_t getDHBits(uint64_t key);

public:

    OBDTables(int hashSize, int fieldSize, int gamma, int v) : ObliviousDictionary(hashSize, fieldSize, gamma, v){
        //the value is fixed for tests reasons
        dhSeed = 3;
        DH = Hasher(dhSeed);
    }

    void init() override;

    virtual void createSets() = 0;

    bool encode() override;

    virtual void fillTables() = 0;

    virtual int peeling() = 0;

    virtual void generateExternalToolValues() = 0;

    virtual void unpeeling() = 0;

    virtual bool hasLoop() = 0;

};

class OBD2Tables : public OBDTables{

private:
    uint64_t firstSeed, secondSeed;
    unordered_set<uint64_t, Hasher> first;
    unordered_set<uint64_t, Hasher> second;

public:
    OBD2Tables(int hashSize, int fieldSize, int gamma, int v);

    void createSets() override;

    void init() override;

    vector<uint64_t> dec(uint64_t key) override;

    vector<byte> decode(uint64_t key) override;

    void fillTables() override;

    int peeling() override;

    void generateExternalToolValues() override;

    void unpeeling() override;

    bool checkOutput() override;

    bool hasLoop() override;

    int getTableSize() override {return 2*tableRealSize;}
};

class OBD3Tables : public OBDTables {
private:
    uint64_t firstSeed, secondSeed, thirdSeed;
    unordered_set<uint64_t, Hasher> first;
    unordered_set<uint64_t, Hasher> second;
    unordered_set<uint64_t, Hasher> third;

    double c1;


    void handleQueue(queue<int> &queueMain, unordered_set<uint64_t, Hasher> &main,
                     queue<int> &queueOther1, unordered_set<uint64_t, Hasher> &other1,
                     queue<int> &queueOther2,unordered_set<uint64_t, Hasher> &other2);

public:

    OBD3Tables(int hashSize, double c1, int fieldSize, int gamma, int v);

    void createSets() override;

    void init() override;

    vector<uint64_t> dec(uint64_t key) override;

    vector<byte> decode(uint64_t key) override;

    void fillTables() override;

    int peeling() override;

    void generateExternalToolValues() override;

    void unpeeling() override;

    bool checkOutput() override;

    bool hasLoop() override;

    int getTableSize() override {return 3*tableRealSize;}


};

class StarDictionary : public ObliviousDictionary {
private:
    vector<OBD3Tables*> bins;
    vector<vector<uint64_t>> keysForBins;
    vector<vector<byte>> valsForBins;

    int q;

    Hasher hashForBins;
    int numItemsForBin;
    int center;
    int numThreads = 1;

public:

    StarDictionary(int numItems, double c1, double c2, int q, int fieldSize, int gamma, int v, int numThreads = 1);

    void setKeysAndVals(vector<uint64_t>& keys, vector<byte>& values) override;

    void init() override;

    vector<uint64_t> dec(uint64_t key) override;

    vector<byte> decode(uint64_t key);

    bool encode() override;

    bool checkOutput() override;

    int getTableSize() override {
        return (q+1)*bins[0]->getTableSize();
    }

    vector<byte> getVariables() override;


    void peelMultipleBinsThread(int start, int end, vector<int> &failureIndices, int threadId);

    void unpeelMultipleBinsThread(int start, int end, int failureIndex);
};




#endif //BENNYPROJECT_OBLIVIOUSDICTIONARY_H
