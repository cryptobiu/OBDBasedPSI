//#include <iostream>

//#include "ProtocolParty.h"
#include <chrono>

int main(int argc, char* argv[]) {

//    srand(25);
//    initField();
//    GF2EMatrix A(20);
//    for(size_t i = 0; i < 20; ++i) {
//        A[i].resize(20);
//        for(size_t j = 0; j < 20; ++j) {
//            A[i][j] = NTL::to_GF2E((i == j)? 1: 0);
//        }
//    }
//
//    GF2EVector B(20);
//    GF2X temp;
//    for(size_t i = 0; i < 20; ++i) {
//        auto r = rand();
//
//        GF2XFromBytes(temp, (byte*)&r ,4);
//        B[i] = NTL::to_GF2E(temp);
//    }
//
//    GF2EVector X;
//    int err = 0;
//    if(0 == (err = solve_api(A, B, X))) {
//        cout << "solve API success" << endl;
//        bool match = true;
//        for(size_t i = 0; i < 20; ++i) {
//            if(B[i] != X[i]) {
//                cout << "X : B mismatch @" << i << endl;
//                match = false;
//            }
//        }
//        if(match) {
//            cout << "X and B match." << endl;
//        }
//    } else {
//        cout << "solve API failure: " << err << endl;
//    }
//    GF2X irreduciblePolynomial = BuildSparseIrred_GF2X(132);
//    GF2E::init(irreduciblePolynomial);
//
//    Mat<GF2E> matrix;
//    matrix.SetDims(2,2);
//    Vec<GF2E> values;
//    values.SetLength(2);
//
//    //Get all the edges that are in the graph's circles and calc the polynomial values that should be for them.
//
//    byte fuckingByte[2];
//    GF2X fuckingGF2X;
//
//    fuckingByte[0] = 1;
//    fuckingByte[1] = 8;
//    GF2XFromBytes(fuckingGF2X, fuckingByte,2);
//
//    matrix[0][0] = to_GF2E(fuckingGF2X);
//
//
//    cout<<"GF2X "<<fuckingGF2X<<endl;
//    cout<<"GF2E "<<to_GF2E(fuckingGF2X)<<endl;
//
//    fuckingByte[0] = 2;
//    GF2XFromBytes(fuckingGF2X, &fuckingByte[0],1);
//    cout<<"GF2X "<<fuckingGF2X<<endl;
//    cout<<"GF2E "<<to_GF2E(fuckingGF2X)<<endl;
//
//    matrix[0][1] = to_GF2E(fuckingGF2X);
//
//    fuckingByte[0] = 3;
//    GF2XFromBytes(fuckingGF2X, &fuckingByte[0],1);
//    matrix[1][0] = to_GF2E(fuckingGF2X);
//
//    cout<<"GF2X "<<fuckingGF2X<<endl;
//    cout<<"GF2E "<<to_GF2E(fuckingGF2X)<<endl;
//
//    fuckingByte[0] = 4;
//    GF2XFromBytes(fuckingGF2X, &fuckingByte[0],1);
//
//
//    cout<<"GF2X "<<fuckingGF2X<<endl;
//    cout<<"GF2E "<<to_GF2E(fuckingGF2X)<<endl;
//
//    matrix[1][1] = to_GF2E(fuckingGF2X);
//
//    values[0] = to_GF2E(fuckingGF2X);
//
//    fuckingByte[0] = 1;
//    GF2XFromBytes(fuckingGF2X, &fuckingByte[0],1);
//
//    values[1] = GF2E(0);
//
//    //TODO call the solver and get the results in variables
//    GF2E d;
//    Vec<GF2E> x;
//    x.SetLength(2);
//    solve(d, matrix, x, values);
//
//    matrix[0][0]*x[0];
//    cout<<"first= "<< matrix[0][0]*x[0]<<endl;
//    cout<<"second = "<< matrix[0][1]*x[1]<<endl;
//
//    cout<<"should be 4 : "<< matrix[0][0]*x[0] + matrix[0][1]*x[1]<<endl;
//
//    GF2X val(17);
//    cout<<"val = "<<val<<endl;
//    cout<<"irreduciblePolynomial:"<<endl;
//    cout<<irreduciblePolynomial<<endl;
//    cout<<"matrix:"<<endl;
//    cout<<matrix<<endl;
//    cout<<rep(matrix[0][0])<<" "<<rep(matrix[0][1])<<endl;
//    cout<<rep(matrix[1][0])<<" "<<rep(matrix[1][1])<<endl;
//
//    cout<<"x = "<<x[0]<<" "<<x[1]<<endl;

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