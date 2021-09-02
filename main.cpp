#include <iostream>

#include <chrono>
#include "NTL/GF2XFactoring.h"
#include "NTL/mat_GF2E.h"

#include "gf2e_mat_solve.h"
#include "ObliviousDictionary.h"

typedef unsigned char byte;

int main(int argc, char* argv[]) {
    int hashSize=4, fieldSize=1024, v=20, gamma = 2;
    double c1 = 0.3;
    vector<uint64_t> keys;
    vector<byte> values;
    keys.resize(hashSize);
    int fieldSizeBytes = fieldSize % 8 == 0 ? fieldSize/8 : fieldSize/8 + 1;
    values.resize(hashSize*fieldSizeBytes);

    for (int i=0; i < hashSize; i++){
        keys[i] = i;//prg.getRandom64();
    }
    for (int j=0; j < fieldSizeBytes; j++){
        char x = 'a';
        std::cout << x << ' ';
        values[j] = x;//prg.getRandom64();
    }

    ObliviousDictionary* dic = new OBD2Tables(hashSize, c1, fieldSize, gamma, v);
    dic->setKeysAndVals(keys, values);

    for (int i=0; i<hashSize; i++) {
        vector<uint64_t> indices = dic->decOptimized(keys[i]);
        for (auto j: indices)
            std::cout << j << ' ';
    }

//    int fieldSize = 17;
//
//    srand(25);
//    initField(fieldSize);
//    GF2EMatrix A(20);
//    for(size_t i = 0; i < 20; ++i) {
//        A[i].resize(20);
//        for(size_t j = 0; j < 20; ++j) {
//            A[i][j] = NTL::to_GF2E((i == j)? 1: 0);
//        }
//    }
//
//    GF2EVector B(20);
//    NTL::GF2X temp;
//    for(size_t i = 0; i < 20; ++i) {
//        auto r = rand();
//
//        GF2XFromBytes(temp, (byte*)&r ,4);
//        B[i] = NTL::to_GF2E(temp);
//    }
//
//    GF2EVector X;
//    int err = 0;
//    if(0 == (err = solve_api(A, B, X, fieldSize))) {
//        std::cout << "solve API success" << std::endl;
//        bool match = true;
//        for(size_t i = 0; i < 20; ++i) {
//            if(B[i] != X[i]) {
//                std::cout << "X : B mismatch @" << i << std::endl;
//                match = false;
//            }
//        }
//        if(match) {
//            std::cout << "X and B match." << std::endl;
//        }
//    } else {
//        std::cout << "solve API failure: " << err << std::endl;
//    }
//    NTL::GF2X irreduciblePolynomial = NTL::BuildSparseIrred_GF2X(132);
//    NTL::GF2E::init(irreduciblePolynomial);
//
//    NTL::Mat<NTL::GF2E> matrix;
//    matrix.SetDims(2,2);
//    NTL::Vec<NTL::GF2E> values;
//    values.SetLength(2);
//
//    //Get all the edges that are in the graph's circles and calc the polynomial values that should be for them.
//
//    byte fuckingByte[2];
//    NTL::GF2X fuckingGF2X;
//
//    fuckingByte[0] = 1;
//    fuckingByte[1] = 8;
//    GF2XFromBytes(fuckingGF2X, fuckingByte,2);
//
//    matrix[0][0] = to_GF2E(fuckingGF2X);
//
//
//    std::cout<<"GF2X "<<fuckingGF2X<<std::endl;
//    std::cout<<"GF2E "<<to_GF2E(fuckingGF2X)<<std::endl;
//
//    fuckingByte[0] = 2;
//    GF2XFromBytes(fuckingGF2X, &fuckingByte[0],1);
//    std::cout<<"GF2X "<<fuckingGF2X<<std::endl;
//    std::cout<<"GF2E "<<to_GF2E(fuckingGF2X)<<std::endl;
//
//    matrix[0][1] = to_GF2E(fuckingGF2X);
//
//    fuckingByte[0] = 3;
//    GF2XFromBytes(fuckingGF2X, &fuckingByte[0],1);
//    matrix[1][0] = to_GF2E(fuckingGF2X);
//
//    std::cout<<"GF2X "<<fuckingGF2X<<std::endl;
//    std::cout<<"GF2E "<<to_GF2E(fuckingGF2X)<<std::endl;
//
//    fuckingByte[0] = 4;
//    GF2XFromBytes(fuckingGF2X, &fuckingByte[0],1);
//
//
//    std::cout<<"GF2X "<<fuckingGF2X<<std::endl;
//    std::cout<<"GF2E "<<to_GF2E(fuckingGF2X)<<std::endl;
//
//    matrix[1][1] = to_GF2E(fuckingGF2X);
//
//    values[0] = to_GF2E(fuckingGF2X);
//
//    fuckingByte[0] = 1;
//    GF2XFromBytes(fuckingGF2X, &fuckingByte[0],1);
//
//    values[1] = NTL::GF2E(0);
//
//    //TODO call the solver and get the results in variables
//    NTL::GF2E d;
//    NTL::Vec<NTL::GF2E> x;
//    x.SetLength(2);
//    solve(d, matrix, x, values);
//
//    matrix[0][0]*x[0];
//    std::cout<<"first= "<< matrix[0][0]*x[0]<<std::endl;
//    std::cout<<"second = "<< matrix[0][1]*x[1]<<std::endl;
//
//    std::cout<<"should be 4 : "<< matrix[0][0]*x[0] + matrix[0][1]*x[1]<<std::endl;
//
//    NTL::GF2X val(17);
//    std::cout<<"val = "<<val<<std::endl;
//    std::cout<<"irreduciblePolynomial:"<<std::endl;
//    std::cout<<irreduciblePolynomial<<std::endl;
//    std::cout<<"matrix:"<<std::endl;
//    std::cout<<matrix<<std::endl;
//    std::cout<<rep(matrix[0][0])<<" "<<rep(matrix[0][1])<<std::endl;
//    std::cout<<rep(matrix[1][0])<<" "<<rep(matrix[1][1])<<std::endl;
//
//    std::cout<<"x = "<<x[0]<<" "<<x[1]<<std::endl;

//
//    CmdParser parser;
//    auto parameters = parser.parseArguments("", argc, argv);
//    int partyID = stoi(parser.getValueByKey(parameters, "partyID"));
//
//    if (partyID == 0) {
//        Receiver receiver(argc, argv);
//        receiver.run();
//    } else {
//        Sender sender(argc, argv);
//        sender.run();
//    }
return 0;
}