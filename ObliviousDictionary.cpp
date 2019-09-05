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
            dhBits >> 1;
        }
    }

    return indices;
}

uint64_t ObliviousDictionary::getDHBits(uint64_t key){
    auto bits = DH(key);
    return bits >> (64-gamma);
}

void ObliviousDictionaryDB::init() {

    vals.clear();
    keys.clear();
    peelingVector.clear();
    peelingCounter = 0;

    keys.resize(hashSize);
    vals.reserve(hashSize);

    for (int i=0; i < hashSize; i++){
        keys[i] = prg.getRandom64() >> 3;
        vals.insert({keys[i], prg.getRandom64() >> 3});
    }

    int numKeysToCheck = 10;
    cout<<"keys to check with the other party"<<endl;
    for (int i=0; i<numKeysToCheck; i++){
        cout << "key = " << keys[i] << " val = " << vals[keys[i]] << endl;
    }

    first.clear();
    second.clear();
}


ObliviousDictionaryDB::ObliviousDictionaryDB(int size) : ObliviousDictionary(size) {

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
    variables.resize(2*tableRealSize + gamma, 0);

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

void ObliviousDictionaryDB::fillTables(){

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

void ObliviousDictionaryDB::peeling(){

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

void ObliviousDictionaryDB::generateExternalToolValues(){

    int matrixSize = first.size()*(2*tableRealSize+gamma); // the rows count is the number of edges left after peeling
                                                        //columns count is number of vertexes and gamma bits.

//    byte* matrix = new byte(matrixSize);
//    memset(matrix, 0, matrixSize);
//    byte* values = new byte(first.size());

    GF2X irreduciblePolynomial = BuildSparseIrred_GF2X(8);
    GF2E::init(irreduciblePolynomial);

    Mat<GF2E> matrix;
    matrix.SetDims(first.size(), 2*tableRealSize+gamma);
    Vec<GF2E> values;
    values.SetLength(first.size());

    GF2X temp;
    byte oneb = 1;
    GF2X one;
    GF2XFromBytes(one, &oneb,1);
    byte dhBit;

    //Get all the edges that are in the graph's circles and calc the polynomial values that should be for them.
    int edgesCounter = 0;
    for (int i=0; i<tableRealSize; i++){
        if (first.bucket_size(i) > 1){
            for (auto key = first.begin(i); key!= first.end(i); ++key){
//                matrix[edgesCounter*(2*tableRealSize+60) + i] = 1; //put 1 in the left vertex of the edge
//                matrix[edgesCounter*(2*tableRealSize+60) + tableRealSize + second.bucket(*key)] = 1; //put 1 in the right vertex of the edge

                matrix[edgesCounter][i] = to_GF2E(one); //put 1 in the left vertex of the edge
                matrix[edgesCounter][tableRealSize + second.bucket(*key)] = to_GF2E(one); //put 1 in the right vertex of the edge

                auto dhBits = getDHBits(*key);
                uint64_t mask = 1;
                for (int j=0; j<gamma; j++){
                    dhBit = dhBits & mask;
//                    matrix[edgesCounter*(2*tableRealSize+60) + 2*tableRealSize + j] = dhBIts & mask; //put 1 in the right vertex of the edge
                    GF2XFromBytes(temp, &dhBit, 1);
                    matrix[edgesCounter][ 2*tableRealSize + j] = to_GF2E(temp); //put 1 in the right vertex of the edge
                    dhBits >> 1;

                }
                GF2XFromBytes(temp, (byte*)&(vals[*key]) ,1);
                values[edgesCounter] =to_GF2E(temp);
                edgesCounter++;

            }
        }
    }

    cout<<"num of equations =  "<<edgesCounter<<endl;

    if(reportStatistics==1) {

        statisticsFile << edgesCounter << ", \n";
    }

    //TODO call the solver and get the results in variables
    GF2E d;
    Vec<GF2E> x;
    x.SetLength(2*tableRealSize+gamma);
    solve(d, matrix, x, values);


//    delete [] matrix;
//    delete [] values;
//    int polySize = 5*log2(hashSize);
//    vector<uint64_t> edgesForPolynomial(polySize);
//    vector<uint64_t> valuesForPolynomial(polySize);
//
//    //Assign random values to all vertex in the circles.
//    for (int i=0; i<tableRealSize; i++){
//        if (first.bucket_size(i) > 1){
//            firstEncValues[i] =  prg.getRandom64() >> 3;
////                cout<<"bucket "<<i<<" got random value in first"<<endl;
//        }
//        if (second.bucket_size(i) > 1) {
//            secondEncValues[i] = prg.getRandom64()  >> 3;
////                cout<<"bucket "<<i<<" got random value in second"<<endl;
//        }
//    }
//
//    //Get all the edges that are in the graph's circles and calc the polynomial values that should be for them.
//    int polyCounter = 0;
//    for (int i=0; i<tableRealSize; i++){
//        if (first.bucket_size(i) > 1){
//            for (auto key = first.begin(i); key!= first.end(i); ++key){
//                edgesForPolynomial[polyCounter] = *key;
//                valuesForPolynomial[polyCounter] = firstEncValues[i] ^ secondEncValues[second.bucket(*key)] ^ vals[*key];
//                polyCounter++;
//            }
//        }
//    }
//
//    cout<<"circles size =  "<<polyCounter<<endl;
//
//    if(reportStatistics==1) {
//
//        statisticsFile << polyCounter << ", \n";
//    }


//if (polyCounter < polySize){
//        for (int i=polyCounter; i<polySize; i++){
//            edgesForPolynomial[polyCounter] = peelingVector[i];
//            valuesForPolynomial[polyCounter] = prg.getRandom64() >> 3;
//            polyCounter++;
//        }
//    } else if (polyCounter > polySize){
//        cout<<"error!!! circles size is bigger than polynomial size"<<endl;
//    }
//
//
//    tool->generateToolValues(edgesForPolynomial, valuesForPolynomial);
}




void ObliviousDictionaryDB::unpeeling(){
    cout<<"in unpeeling"<<endl;
    uint64_t firstPosition, secondPosition, dhBitsVal, key;
//    vector<uint64_t> polyVals(peelingCounter);
//    Poly::evalMersenneMultipoint(polyVals, polynomial, peelingVector);

    while (peelingCounter > 0){
//            cout<<"key = "<<key<<endl;
        key = peelingVector[--peelingCounter];
        auto indices = dec(key);

        dhBitsVal = 0;
        for (int j=2; j<indices.size(); j++){
            dhBitsVal ^= variables[2*tableRealSize+ indices[j]]; //put 1 in the right vertex of the edge

        }
//        Poly::evalMersenne((ZpMersenneLongElement*)&poliVal, polynomial, (ZpMersenneLongElement*)&key);
//        poliVal = polyVals[peelingCounter];
        if (variables[indices[0]] == 0 && variables[tableRealSize + indices[1]] == 0){
            variables[indices[1]] = prg.getRandom64()  >> 3;
//                cout<<"set RANDOM value to first in bucket "<<firstPosition<<endl;
        }
        if (variables[indices[0]] == 0){
            variables[indices[0]] = vals[key] ^ variables[tableRealSize + indices[1]] ^ dhBitsVal;
//                cout<<"set value to first in bucket "<<firstPosition<<endl;
        } else {
            variables[tableRealSize + indices[1]] = vals[key] ^ variables[indices[0]] ^ dhBitsVal;
//                cout<<"set value to second in bucket "<<secondPosition<<endl;
        }
    }
    cout<<"peelingCounter = "<<peelingCounter<<endl;
}

void ObliviousDictionaryDB::checkOutput(){

    uint64_t key, val, dhBitsVal;

    for (int i=0; i<hashSize; i++){
        key = keys[i];
        val = vals[key];

        auto indices = dec(key);

        dhBitsVal = 0;
        for (int j=2; j<indices.size(); j++){
            dhBitsVal ^= variables[2*tableRealSize+ indices[j]]; //put 1 in the right vertex of the edge

        }

        if ((variables[indices[0]] ^ variables[indices[1]] ^ dhBitsVal) == val) {
            if (i%100000 == 0)
                cout<<"good value!!! val = "<<val<<endl;
        } else {//if (!hasLoop()){
            cout<<"invalid value :( val = "<<val<<" wrong val = "<<(variables[indices[0]] ^ variables[indices[1]] ^ dhBitsVal)<<endl;
            cout<<"firstEncValues["<<indices[0]<<"] = "<<variables[indices[0]]<<endl;
            cout<<"secondEncValues["<<indices[1]<<"] = "<<variables[indices[1]]<<endl;
            cout<<"dhBitsVal = "<<dhBitsVal<<endl;
        }

    }
}

bool ObliviousDictionaryDB::hasLoop(){
    for (int position = 0; position<tableRealSize; position++) {
        if (first.bucket_size(position) > 1) {
            return true;
        }
    }
    return false;
}


void ObliviousDictionaryDB::sendData(shared_ptr<ProtocolPartyData> otherParty){
//TODO should be deleted!!
    otherParty->getChannel()->write((byte*)&firstSeed, 8);
    otherParty->getChannel()->write((byte*)&secondSeed, 8);
    cout<<"firstSeed = "<<firstSeed<<endl;
    cout<<"secondSeed = "<<secondSeed<<endl;
    cout<<"key size in bytes: "<<keys.size()*8<<endl;
    otherParty->getChannel()->write((byte*)keys.data(), keys.size()*8);
//TODO until here

    otherParty->getChannel()->write((byte*)variables.data(), variables.size()*8);

}

ObliviousDictionaryQuery::ObliviousDictionaryQuery(int hashSize) : ObliviousDictionary(hashSize){

    auto key = prg.generateKey(128);
    prg.setKey(key);
}

void ObliviousDictionaryQuery::readData(shared_ptr<ProtocolPartyData> otherParty){

//TODO should be deleted!!
    otherParty->getChannel()->read((byte*)&firstSeed, 8);
    otherParty->getChannel()->read((byte*)&secondSeed, 8);
    createSets();

    cout<<"firstSeed = "<<firstSeed<<endl;
    cout<<"secondSeed = "<<secondSeed<<endl;
    vector<uint64_t> tmpKeys(hashSize);
    otherParty->getChannel()->read((byte*)tmpKeys.data(), tmpKeys.size()*8);

    int numKeysToCheck = 10;
    keys.resize(numKeysToCheck);
    for (int i=0; i<numKeysToCheck; i++){
        keys[i] = tmpKeys[i];//prg.getRandom32() % hashSize;
    }

    variables.resize(tableRealSize*2 + gamma, 0);
//TODO until here

    otherParty->getChannel()->read((byte*)variables.data(), variables.size()*8);
}

void ObliviousDictionaryQuery::calcRealValues(){

    cout<<"vals:"<<endl;
    uint64_t dhBitsVal;
    int size = keys.size();
    vector<uint64_t> vals(size);
    for (int i=0; i<size; i++){

        auto indices = dec(keys[i]);

        dhBitsVal = 0;
        for (int j=2; j<indices.size(); j++){
            dhBitsVal ^= variables[2*tableRealSize+ indices[j]]; //put 1 in the right vertex of the edge

        }
        vals[i] = variables[indices[0]] ^ variables[indices[1]] ^ dhBitsVal;
        cout<<"key = "<<keys[i]<<" val = "<<vals[i]<<endl;
    }

}

void ObliviousDictionaryQuery::output(){

}

