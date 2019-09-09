//
// Created by moriya on 7/17/19.
//

#include "ObliviousDictionary.h"
#include "Tools.h"

void ObliviousDictionary::createSets(){
    first = unordered_set<uint64_t, Hasher>(hashSize, Hasher(firstSeed));
    second = unordered_set<uint64_t, Hasher>(hashSize, Hasher(secondSeed));

    tableRealSize = first.bucket_count();
    cout<<"tableRealSize = "<<tableRealSize<<endl;

    while(tableRealSize/1.2 < hashSize){
        first = unordered_set<uint64_t, Hasher>(tableRealSize + 1, Hasher(firstSeed));
        second = unordered_set<uint64_t, Hasher>(tableRealSize + 1, Hasher(secondSeed));

        tableRealSize = first.bucket_count();
        cout<<"tableRealSize = "<<tableRealSize<<endl;
    }


    hashSize = tableRealSize/1.2;
    cout<<"new hashSize = "<<hashSize<<endl;
}

vector<uint64_t> ObliviousDictionary::dec(uint64_t key){

    vector<uint64_t> indices;
    indices.push_back(first.bucket(key));
    indices.push_back(second.bucket(key));

    auto dhBits = getDHBits(key);
    uint64_t mask = 1;
    for (int j=0; j<gamma; j++){
        if ((dhBits & mask) == 1){
            indices.push_back(j); //put 1 in the right vertex of the edge
        }
        dhBits = dhBits >> 1;
    }

    return indices;
}

uint64_t ObliviousDictionary::getDHBits(uint64_t key){
    auto bits = DH(key);
    return bits >> (64-gamma);
}

void ObliviousDictionary::init() {

    vals.clear();
    keys.clear();
    peelingVector.clear();
    peelingCounter = 0;

    keys.resize(hashSize);
    vals.reserve(hashSize);

    uint64_t randomVal;
    GF2X temp;

    for (int i=0; i < hashSize; i++){
        keys[i] = prg.getRandom64();
        randomVal = prg.getRandom64();
        GF2XFromBytes(temp, (byte*)&randomVal ,8);
        vals.insert({keys[i], to_GF2E(temp)});
    }

    int numKeysToCheck = 10;
    cout<<"keys to check with the other party"<<endl;
    for (int i=0; i<numKeysToCheck; i++){
        cout << "key = " << keys[i] << " val = " << vals[keys[i]] << endl;
    }

    first.clear();
    second.clear();
}


ObliviousDictionary::ObliviousDictionary(int hashSize) : hashSize(hashSize) {

    gamma = 60;
    initField();

    auto key = prg.generateKey(128);
    prg.setKey(key);

    firstSeed = prg.getRandom64();
    secondSeed = prg.getRandom64();
    dhSeed = prg.getRandom64();

    DH = Hasher(dhSeed);

    auto start = high_resolution_clock::now();
    createSets();
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end-start).count();

    cout << "time in milliseconds for create sets: " << duration << endl;

    start = high_resolution_clock::now();
    first.insert(1);
    end = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end-start).count();
    cout << "time in milliseconds for insert first element: " << duration << endl;

    start = high_resolution_clock::now();
    first.erase(1);
    end = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(end-start).count();
    cout << "time in milliseconds for erase first element: " << duration << endl;

    cout<<"after create sets"<<endl;
    cout<<"tableRealSize = "<<tableRealSize<<endl;
    cout<<"hashSize = "<<hashSize<<endl;

//    firstEncValues.resize(tableRealSize, 0);
//    secondEncValues.resize(tableRealSize, 0);
    variables.resize(2*tableRealSize + gamma);
    sign.resize(2*tableRealSize, 0);
//    keys.resize(hashSize);
//    vals.reserve(hashSize);
//
//    for (int i=0; i<hashSize; i++){
//        keys[i] = prg.getRandom64() >> 3;
//        vals.insert({keys[i],prg.getRandom64()>>3});
//    }
//
//    int numKeysToCheck = 10;
//    cout<<"keys to check with the other party"<<endl;
//    for (int i=0; i<numKeysToCheck; i++){
//        cout<<"key = "<<keys[i]<<" val = "<<vals[keys[i]]<<endl;
//    }



}

void ObliviousDictionary::fillTables(){

    for (int i=0; i<hashSize; i++){

//            cout<<"key is "<<keys[i]<<endl;
//        auto pair = first.insert(keys[i]);
        first.insert(keys[i]);
        second.insert(keys[i]);

//        if (pair.second == false){
//            cout<<"key = "<<keys[i]<<" i = "<<i<<endl;
//        }
    }


    cout<<"first set contains "<<first.size()<<endl;
    cout<<"second set contains "<<second.size()<<endl;

}

void ObliviousDictionary::peeling(){

    peelingVector.resize(hashSize);
    peelingCounter = 0;

    auto t1 = high_resolution_clock::now();
    //Goes on the first hash
    for (int position = 0; position<tableRealSize; position++){
        if (first.bucket_size(position) == 1){
            //Delete the vertex from the graph
            auto key = *first.begin(position);
//                cout<<"remove key "<<key<<endl;
            peelingVector[peelingCounter++] = key;
            first.erase(key);

            //Update the second vertex on the edge
            second.erase(key);
        }
    }
    auto t2 = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(t2-t1).count();

    cout << "time in milliseconds for first loop: " << duration << endl;

    t1 = high_resolution_clock::now();
    //goes on the second hash
    for (int position = 0; position<tableRealSize; position++){
        if (second.bucket_size(position) == 1){
            //Delete the vertex from the graph
            auto key = *second.begin(position);
//                peelingVector.push_back(key);
//                cout<<"remove key "<<key<<endl;
//                second.erase(key);

            int secondBucket = 0;

            while(secondBucket <= position) {

                peelingVector[peelingCounter++] = key;
//                    cout<<"remove key "<<key<<endl;
                second.erase(key);

//                    if (secondBucket>0) cout<<"loop in peeling"<<endl;
                //Update the second vertex on the edge
                int bucket = first.bucket(key);
                first.erase(key);
                if (first.bucket_size(bucket) == 1) {
                    key = *first.begin(bucket);
//                        cout<<"remove key from first "<<key<<endl;
                    peelingVector[peelingCounter++] = key;
                    first.erase(key);

                    //Update the second vertex on the edge
                    secondBucket = second.bucket(key);
                    second.erase(key);
                    if (second.bucket_size(secondBucket) == 1) {
                        key = *second.begin(secondBucket);
//                            peelingVector.push_back(key);
//                            cout<<"remove key "<<key<<endl;
//                            second.erase(key);
                    } else {
                        secondBucket = position + 1;
                    }
                } else {
                    secondBucket = position + 1;
                }

            }
        }
    }
    t2 = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(t2-t1).count();

    cout << "time in milliseconds for second loop: " << duration << endl;

    if (hasLoop()){
        cout << "remain loops!!!" << endl;
    }

    cout<<"peelingCounter = "<<peelingCounter<<endl;

}

void ObliviousDictionary::generateExternalToolValues(){

//    int matrixSize = first.size()*(2*tableRealSize+gamma); // the rows count is the number of edges left after peeling
                                                        //columns count is number of vertexes and gamma bits.

//    byte* matrix = new byte(matrixSize);
//    memset(matrix, 0, matrixSize);
//    byte* values = new byte(first.size());

    GF2EMatrix matrix(first.size());

    cout<<"num of rows = "<<first.size()<<endl;
    cout<<"num of cols = "<<2*tableRealSize+gamma<<endl;
    for(size_t i = 0; i < first.size(); ++i) {
        matrix[i].resize(2*tableRealSize+gamma);
        for(size_t j = 0; j < 2*tableRealSize+gamma; ++j) {
            matrix[i][j] = to_GF2E(0);
        }
    }
    GF2EVector values(first.size());

//    cout<<"matrix:"<<endl;
//    for (int i=0; i<first.size(); i++){
//        for (int j=0; j<2*tableRealSize+gamma; j++){
//            cout<<matrix[i][j]<<" ";
//        }
//        cout<<endl;
//    }
//    GF2X irreduciblePolynomial = BuildSparseIrred_GF2X(132);
//    GF2E::init(irreduciblePolynomial);
    //Get all the edges that are in the graph's circles and calc the polynomial values that should be for them.
    int edgesCounter = 0;
    for (int i=0; i<tableRealSize; i++){
        if (first.bucket_size(i) > 1){
            for (auto key = first.begin(i); key!= first.end(i); ++key){
//                matrix[edgesCounter*(2*tableRealSize+60) + i] = 1; //put 1 in the left vertex of the edge
//                matrix[edgesCounter*(2*tableRealSize+60) + tableRealSize + second.bucket(*key)] = 1; //put 1 in the right vertex of the edge

                auto secondPosition = second.bucket(*key);
                matrix[edgesCounter][i] = to_GF2E(1); //put 1 in the left vertex of the edge
                matrix[edgesCounter][tableRealSize + secondPosition] = to_GF2E(1); //put 1 in the right vertex of the edge
                sign[i] = 1;
                sign[tableRealSize + secondPosition] = 1;
                auto dhBits = getDHBits(*key);
//                cout<<"DH bits: "<<dhBits<<endl;
                uint64_t mask = 1;
                for (int j=0; j<gamma; j++){
//                    matrix[edgesCounter*(2*tableRealSize+60) + 2*tableRealSize + j] = dhBIts & mask; //put 1 in the right vertex of the edge
//                    cout<<(dhBits & mask)<<" ";
                    matrix[edgesCounter][ 2*tableRealSize + j] = to_GF2E(dhBits & mask); //put 1 in the right vertex of the edge
//                    matrix[edgesCounter][ 2*tableRealSize + j] = to_GF2E(1); //put 1 in the right vertex of the edge
                    dhBits = dhBits >> 1;

                }
//                cout<<endl;
                values[edgesCounter] = vals[*key];
                edgesCounter++;

            }
        }
    }

//    cout<<"matrix:"<<endl;
//    for (int i=0; i<first.size(); i++){
//        for (int j=0; j<2*tableRealSize+gamma; j++){
//            cout<<matrix[i][j]<<" ";
//        }
//        cout<<endl;
//    }

    cout<<"num of equations =  "<<edgesCounter<<endl;

    if(reportStatistics==1) {

        statisticsFile << edgesCounter << ", \n";
    }

    //TODO call the solver and get the results in variables
    solve_api(matrix, values, variables);

//    for (int i=0; i<tableRealSize; i++){
//        if (first.bucket_size(i) > 1){
//            cout<<"in check loop"<<endl;
//            for (auto key = first.begin(i); key!= first.end(i); ++key){
////                matrix[edgesCounter*(2*tableRealSize+60) + i] = 1; //put 1 in the left vertex of the edge
////                matrix[edgesCounter*(2*tableRealSize+60) + tableRealSize + second.bucket(*key)] = 1; //put 1 in the right vertex of the edge
//
//                auto val = variables[i] + variables[tableRealSize + second.bucket(*key)] ;
//                auto dhBits = getDHBits(*key);
//                uint64_t mask = 1;
//                for (int j=0; j<gamma; j++){
////                    matrix[edgesCounter*(2*tableRealSize+60) + 2*tableRealSize + j] = dhBIts & mask; //put 1 in the right vertex of the edge
//                    if (dhBits & mask)
//                        val +=variables[ 2*tableRealSize + j];
//                    dhBits = dhBits >> 1;
//
//                }
//
//                if (val != vals[*key]){
//                    cout<<"wrong value!!"<<endl;
//                } else{
//                    cout<<"correct!!"<<endl;
//                }
//                edgesCounter++;
//
//            }
//        }

//    cout<<"variables:"<<endl;
//    for (int i=0; i<variables.size(); i++){
//        cout<<"variable["<<i<<"] = "<<variables[i]<<endl;
//    }

}




void ObliviousDictionary::unpeeling(){
    cout<<"in unpeeling"<<endl;
    uint64_t key, randomVal;
    GF2E dhBitsVal;
    GF2X temp;

//    vector<uint64_t> polyVals(peelingCounter);
//    Poly::evalMersenneMultipoint(polyVals, polynomial, peelingVector);

    while (peelingCounter > 0){
//            cout<<"key = "<<key<<endl;
        key = peelingVector[--peelingCounter];
        auto indices = dec(key);
//cout<<"indices = "<<endl;
//for (int i=0; i<indices.size(); i++){
//    cout<<indices[i]<<" ";
//}
//cout<<endl;
        dhBitsVal = 0;
        for (int j=2; j<indices.size(); j++){
//            if (variables[2*tableRealSize+ indices[j]] == 0){
//                randomVal = prg.getRandom64()  >> 3;
//                GF2XFromBytes(temp, (byte*)&randomVal ,8);
//                variables[2*tableRealSize+ indices[j]] = to_GF2E(temp);
//            }
            dhBitsVal += variables[2*tableRealSize+ indices[j]]; //put 1 in the right vertex of the edge

//            cout<<"variable in "<<indices[j]<<" place = "<<variables[2*tableRealSize+ indices[j]]<<endl;
        }
//        Poly::evalMersenne((ZpMersenneLongElement*)&poliVal, polynomial, (ZpMersenneLongElement*)&key);
//        poliVal = polyVals[peelingCounter];
        if (variables[indices[0]] == 0 && variables[tableRealSize + indices[1]] == 0 && sign[indices[0]] == 0 && sign[tableRealSize + indices[1]] == 0){
            randomVal = prg.getRandom64();
            GF2XFromBytes(temp, (byte*)&randomVal ,8);
            variables[indices[0]] = to_GF2E(temp);
//                cout<<"set RANDOM value "<<variables[indices[0]]<<" in index "<<indices[0]<<endl;
        }
        if (variables[indices[0]] == 0 && sign[indices[0]] == 0){
            variables[indices[0]] = vals[key] + variables[tableRealSize + indices[1]] + dhBitsVal;
//                cout<<"set value "<<variables[indices[0]]<<" in index "<<indices[0]<<endl;
//            cout<<"variables["<<indices[0]<<"] = "<<variables[indices[0]]<<endl;
//            cout<<"variables["<<tableRealSize + indices[1]<<"] = "<<variables[tableRealSize + indices[1]]<<endl;
//            cout<<"dhBitsVal = "<<dhBitsVal<<endl;
//            cout<<"val = "<<vals[key]<<endl;
        } if (variables[tableRealSize + indices[1]] == 0 && sign[tableRealSize + indices[1]] == 0){
            variables[tableRealSize + indices[1]] = vals[key] + variables[indices[0]] + dhBitsVal;
//                cout<<"set value "<<variables[tableRealSize + indices[1]]<<" index "<<tableRealSize + indices[1]<<endl;
//            cout<<"variables["<<indices[0]<<"] = "<<variables[indices[0]]<<endl;
//            cout<<"variables["<<tableRealSize + indices[1]<<"] = "<<variables[tableRealSize + indices[1]]<<endl;
//            cout<<"dhBitsVal = "<<dhBitsVal<<endl;
//            cout<<"val = "<<vals[key]<<endl;
        }
    }
    cout<<"peelingCounter = "<<peelingCounter<<endl;

//    cout<<"variables:"<<endl;
//    for (int i=0; i<variables.size(); i++){
//        cout<<"variable["<<i<<"] = "<<variables[i]<<" ";
//    }
//    cout<<endl;
}

void ObliviousDictionary::checkOutput(){

    uint64_t key;
    GF2E val, dhBitsVal;

    for (int i=0; i<hashSize; i++){
        key = keys[i];
        val = vals[key];

        auto indices = dec(key);

        dhBitsVal = 0;
        for (int j=2; j<indices.size(); j++){
            dhBitsVal += variables[2*tableRealSize+ indices[j]]; //put 1 in the right vertex of the edge

        }

        if ((variables[indices[0]] + variables[tableRealSize + indices[1]] + dhBitsVal) == val) {
            if (i%100000 == 0)
                cout<<"good value!!! val = "<<val<<endl;
        } else {//if (!hasLoop()){
            cout<<"invalid value :( val = "<<val<<" wrong val = "<<(variables[indices[0]] + variables[tableRealSize + indices[1]] + dhBitsVal)<<endl;
            cout<<"variables["<<indices[0]<<"] = "<<variables[indices[0]]<<endl;
            cout<<"variables["<<tableRealSize + indices[1]<<"] = "<<variables[tableRealSize + indices[1]]<<endl;
            cout<<"dhBitsVal = "<<dhBitsVal<<endl;
        }

    }
}

bool ObliviousDictionary::hasLoop(){
    for (int position = 0; position<tableRealSize; position++) {
        if (first.bucket_size(position) > 1) {
            return true;
        }
    }
    return false;
}