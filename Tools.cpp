//
// Created by moriya on 7/31/19.
//

#include "Tools.h"

void PolynomialTool::generateToolValues(vector<uint64_t> & edgesForPolynomial, vector<uint64_t> & valuesForPolynomial){
    int size = edgesForPolynomial.size();
    polynomial.resize(size);
    cout<<"poly size =  "<<polynomial.size()<<endl;
    Poly::interpolateMersenne(polynomial, (ZpMersenneLongElement*)edgesForPolynomial.data(), (ZpMersenneLongElement*)valuesForPolynomial.data(), size);

}


uint64_t PolynomialTool::getValue(uint64_t key){

    uint64_t polyVal;
    Poly::evalMersenne((ZpMersenneLongElement*)&polyVal, polynomial, (ZpMersenneLongElement*)&key);
    return polyVal;
}

byte* PolynomialTool::getSendableData(){
    return (byte*) polynomial.data();
}

void PolynomialTool::setSendableData(byte* data){
    int size = 5*log2(hashSize);
    polynomial.resize(size);
    cout<<"in setSendableData. polySize = "<<size*8<<endl;
    memcpy(polynomial.data(), data, size*8);
}

int PolynomialTool::getSendableDataSize(){
    return 5*log2(hashSize) * 8;
}