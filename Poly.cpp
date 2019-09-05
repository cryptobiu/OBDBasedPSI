//
// Created by meital on 14/01/18.
//

#include "Poly.h"



void Poly::evalMersenne(ZpMersenneLongElement* b, const vector<ZpMersenneLongElement>& coeff, ZpMersenneLongElement* a)
// does a Horner evaluation
{
    ZpMersenneLongElement acc = coeff[coeff.size() - 1];

    for (int i = coeff.size()-2; i >= 0; i--) {
        acc = acc*(*a);//mul(acc, acc, a);
        acc = acc + coeff[i];//add(acc, acc, f.rep[i]);
    }

    *b = acc;
}

void Poly::evalMersenneMultipoint(vector<uint64_t> & b, const vector<ZpMersenneLongElement>& coeff, vector<uint64_t> & a)
// does a Horner evaluation
{
    int size = b.size();
    ZpMersenneLongElement acc;
    for (int j=0; j<size; j++) {
        acc = coeff[coeff.size() - 1];

        for (int i = coeff.size() - 2; i >= 0; i--) {
            acc = acc * (*(ZpMersenneLongElement*)&a[j]);//mul(acc, acc, a);
            acc = acc + coeff[i];//add(acc, acc, f.rep[i]);
        }

        b[j] = *(uint64_t*)&acc;
    }
}

void Poly::interpolateMersenne(vector<ZpMersenneLongElement>& coeff, const ZpMersenneLongElement* a, ZpMersenneLongElement* b, int size)
{
    int m = size;

    ZpMersenneLongElement one(1);
    ZpMersenneLongElement zero;

    ZpMersenneLongElement p(ZpMersenneLongElement::p);

    vector<ZpMersenneLongElement> prod(size);
    memcpy(prod.data(), a, size*8);

    ZpMersenneLongElement t1, t2;

    long k, i;

    vector<ZpMersenneLongElement> res;
    res.resize(m);

    for (k = 0; k < m; k++) {

        const ZpMersenneLongElement& aa = a[k];

        t1 = 1;
        for (i = k-1; i >= 0; i--) {
            t1 = t1*aa; //mul(t1, t1, aa);
            t1 = t1 + prod[i];//add(t1, t1, prod[i]);
        }

        t2 = 0; //clear(t2);
        for (i = k-1; i >= 0; i--) {
            t2 = t2*aa; //mul(t2, t2, aa);
            t2 = t2 + res[i];//add(t2, t2, res[i]);
        }


        t1 = one/t1;//inv(t1, t1);
        t2 = b[k] - t2;//sub(t2, b[k], t2);
        t1 = t1*t2;//mul(t1, t1, t2);

        for (i = 0; i < k; i++) {
            t2 = prod[i]*t1;//mul(t2, prod[i], t1);
            res[i] = res[i] + t2;//add(res[i], res[i], t2);
        }

        res[k] = t1;

        if (k < m-1) {
            if (k == 0)
                prod[0] = p - prod[0];//sub(prod[0], to_ZZ_p(ZZ_pInfo->p),prod[0]);//sub(prod[0], ZZ_p::modulus(), prod[0]);//negate(prod[0], prod[0]);
            else {
                t1 = p - a[k];//sub(t1, to_ZZ_p(ZZ_pInfo->p),a[k]);//negate(t1, a[k]);
                prod[k] = t1 + prod[k-1];//add(prod[k], t1, prod[k-1]);
                for (i = k-1; i >= 1; i--) {
                    t2 = prod[i]*t1;//mul(t2, prod[i], t1);
                    prod[i] = t2 + prod[i-1];//add(prod[i], t2, prod[i-1]);
                }
                prod[0] = prod[0]*t1;//mul(prod[0], prod[0], t1);
            }
        }
    }

    while (m > 0 && !(res[m-1]!=zero)) m--;
    res.resize(m);


    coeff = res;
}
