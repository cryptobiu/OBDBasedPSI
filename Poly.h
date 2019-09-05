//
// Created by meital on 14/01/18.
//

#ifndef PSI_POLY_H
#define PSI_POLY_H


#include "../../include/primitives/Mersenne.hpp"

class Poly {

public:

    static void evalMersenne(ZpMersenneLongElement* b, const vector<ZpMersenneLongElement>& coeff, ZpMersenneLongElement* a);
    static void evalMersenneMultipoint(vector<uint64_t> & b, const vector<ZpMersenneLongElement>& coeff, vector<uint64_t> & a);
    static void interpolateMersenne(vector<ZpMersenneLongElement>& coeff, const ZpMersenneLongElement* a, ZpMersenneLongElement* b, int size);

};


#endif //PSI_POLY_H
