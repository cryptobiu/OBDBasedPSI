//
// Created by moriya on 7/31/19.
//

#ifndef OBLIVIOUSDICTIONARY_TOOLS_H
#define OBLIVIOUSDICTIONARY_TOOLS_H


#include <math.h>       /* log2 */
#include "Poly.h"
#include "ObliviousDictionary.h"
#include "Hasher.h"
#include <unordered_set>
#include <unordered_map>
#include <libscapi/include/primitives/Prg.hpp>


class Tools {
public:

    Tools(){}
    virtual void setHashSize(int hashSize) = 0;
    virtual void generateToolValues(vector<uint64_t> & edgesForPolynomial, vector<uint64_t> & valuesForPolynomial) = 0;
    virtual uint64_t getValue(uint64_t key) = 0;
    virtual byte* getSendableData() = 0;
    virtual void setSendableData(byte* data) = 0;
    virtual int getSendableDataSize() = 0;
};

class PolynomialTool : public Tools {
private:
    int hashSize;
    vector<ZpMersenneLongElement> polynomial;

public:

    PolynomialTool(int hashSize) : hashSize(hashSize){}

    void setHashSize(int hashSize) override { this->hashSize = hashSize; }
    void generateToolValues(vector<uint64_t> & edgesForPolynomial, vector<uint64_t> & valuesForPolynomial) override;
    uint64_t getValue(uint64_t key) override;

    byte* getSendableData() override;
    void setSendableData(byte* data) override;
    int getSendableDataSize() override;
};


#endif //OBLIVIOUSDICTIONARY_TOOLS_H
