//
// Created by moriya on 7/24/19.
//

#include "ProtocolParty.h"

ProtocolParty::ProtocolParty(int argc, char* argv[]) : Protocol("PSI", argc, argv)
{

    partyId = stoi(this->getParser().getValueByKey(arguments, "partyID"));

    this->times = stoi(this->getParser().getValueByKey(arguments, "internalIterationsNumber"));
    this->hashSize = stoi(this->getParser().getValueByKey(arguments, "hashSize"));
    this->fieldSize = stoi(this->getParser().getValueByKey(arguments, "fieldSize"));
    fieldSizeBytes = fieldSize % 8 == 0 ? fieldSize/8 : fieldSize/8 + 1;
    int zeroBits = 8 - fieldSize % 8;
    unsigned char zeroMask = 0xff >> zeroBits;

    cout << "fieldSizeBytes: " << fieldSizeBytes << endl;
    cout << "zeroBits: " << zeroBits << endl;
    cout << "zero mask: " << (int)zeroMask << "  " << std::bitset<8>(zeroMask) << endl;

    auto c2 = stof(this->getParser().getValueByKey(arguments, "c2"));
    auto c1 = stof(this->getParser().getValueByKey(arguments, "c1"));
    cout<<"c2 = "<<c2<<" c1 = "<<c1<<endl;
    q = stoi(this->getParser().getValueByKey(arguments, "q"));
    auto numThreads = stoi(this->getParser().getValueByKey(arguments, "numThreads"));


    gamma = 60;
    vector<string> subTaskNames{"Online", "CreateDictionary", "OT", "ComputeXors", "ReceiveAndCalc"};
    timer = new Measurement(*this, subTaskNames);

    MPCCommunication comm;
    string partiesFile = this->getParser().getValueByKey(arguments, "partiesFile");

    reportStatistics = stoi(this->getParser().getValueByKey(arguments, "reportStatistics"));
    isMalicious = stoi(this->getParser().getValueByKey(arguments, "malicious")) == 0 ? false : true;
cout<<"malicious = "<<isMalicious<<endl;

    version = this->getParser().getValueByKey(arguments, "version");

    if(reportStatistics==0) {
        otherParty = comm.setCommunication(io_service, partyId, 2, partiesFile)[0];

        ConfigFile cf(partiesFile);

        string portString, ipString;
        vector<int> ports(2);
        vector<string> ips(2);

        for (int i = 0; i < 2; i++) {
            portString = "party_" + to_string(i) + "_port";
            ipString = "party_" + to_string(i) + "_ip";

            //get partys IPs and ports data
            ports[i] = stoi(cf.Value("", portString));
            ips[i] = cf.Value("", ipString);
        }
        //osuCrypto::IOService ios(0);
        addressForOT = ips[1];
        portForOT = ports[1] + 1;

        cout<<"addressForOT = "<<addressForOT<<endl;
        cout<<"portForOT = "<<portForOT<<endl;

        string tmp = "init times";
        //cout<<"before sending any data"<<endl;
        byte tmpBytes[20];
        if (otherParty->getID() < partyId) {
            otherParty->getChannel()->write(tmp);
            otherParty->getChannel()->read(tmpBytes, tmp.size());
        } else {
            otherParty->getChannel()->read(tmpBytes, tmp.size());
            otherParty->getChannel()->write(tmp);
        }
    }


    if (version.compare("2Tables") == 0) {
        dic = new OBD2Tables(hashSize, c1, fieldSize, gamma, 20);
    } else if (version.compare("3Tables") == 0) {
        dic = new OBD3Tables(hashSize, c1, fieldSize, gamma, 20);
    } else if (version.compare("star") == 0) {
        double numItemsForBin = c2*(hashSize/q);
        gamma = 40 + 0.5*log(numItemsForBin);
        //dic = new StarDictionary(hashSize, 1.25, 1.04, 160, fieldSize, gamma, 0.5*log(hashSize), numThreads);
        dic = new StarDictionary(hashSize, c1, c2, q, fieldSize, gamma, 0.5*log(numItemsForBin), numThreads);

    }

    dic->setReportStatstics(reportStatistics);

    hashSize = dic->getHashSize();
    tableRealSize = dic->getTableSize();
    numOTs = tableRealSize;
    cout<<"after create dictionary. hashSize = "<<hashSize<<endl;
    cout<<"after create dictionary. tableRealSize = "<<tableRealSize<<endl;
    keys.resize(hashSize);
    vals.resize(hashSize*fieldSizeBytes);


    //    auto key = prg.generateKey(128);
    cout<<"before prg"<<endl;
    prg = PrgFromOpenSSLAES(hashSize*fieldSizeBytes*2);
    cout<<"after prg"<<endl;
    vector<unsigned char> seed(128, 0);

    SecretKey key(seed, "");
    prg.setKey(key);

    for (int i=0; i < hashSize; i++){
        keys[i] = i;//prg.getRandom64();
    }
    prg.getPRGBytes(vals, 0, hashSize*fieldSizeBytes);

    cout << "masking the values so they will not have random in the padding bits" << endl;
    for (int i=0; i<hashSize; i++){
//        cout << "key = " << keys[i] << " val = ";
//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<(int)(vals[i*fieldSizeBytes + j])<<" " << std::bitset<8>(vals[i*fieldSizeBytes + j]) << " ";
//        }
//        cout<<endl;

        vals[(i+1)*fieldSizeBytes-1] = vals[(i+1)*fieldSizeBytes-1]  >> zeroBits;
//
//        cout << "key = " << keys[i] << " val = ";
//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<(int)(vals[i*fieldSizeBytes + j])<<" " << std::bitset<8>(vals[i*fieldSizeBytes + j]) << " ";
//        }
//        cout<<endl;
    }

    aes = EVP_CIPHER_CTX_new();
    const EVP_CIPHER* cipher = EVP_aes_128_ecb();

    vector<unsigned char> fixedKey(128);
    prg.getPRGBytes(fixedKey, 0, 128);
    EVP_EncryptInit(aes, cipher, fixedKey.data(), NULL);

}

ProtocolParty::~ProtocolParty(){

    delete timer;
}

void ProtocolParty::run() {

    for (iteration=0; iteration<times; iteration++){

        auto t1start = high_resolution_clock::now();
        timer->startSubTask("Online", iteration);
        runOnline();
        timer->endSubTask("Online", iteration);

        auto t2end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(t2end-t1start).count();

        cout << "time in milliseconds for protocol: " << duration << endl;
    }


}

Receiver::Receiver(int argc, char *argv[]): ProtocolParty(argc, argv){

    dic->setKeysAndVals(keys, vals);

}

Receiver::~Receiver(){
    delete dic;
}

void Receiver::runOnline() {

    auto start = high_resolution_clock::now();
    auto t1 = high_resolution_clock::now();

    timer->startSubTask("CreateDictionary", iteration);
    auto sigma = createDictionary();

    cout<<"variables size = "<<sigma.size()/fieldSizeBytes<<endl;
    timer->endSubTask("CreateDictionary", iteration);
    auto t2 = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "createDictionary took in milliseconds: " << duration << endl;
//    checkVariables(sigma);
    t1 = high_resolution_clock::now();

    if (reportStatistics == 0) {
        timer->startSubTask("OT", iteration);
        runOOS(sigma);
        timer->endSubTask("OT", iteration);
        t2 = high_resolution_clock::now();

        duration = duration_cast<milliseconds>(t2 - t1).count();
        cout << "run OOS took in milliseconds: " << duration << endl;


        t1 = high_resolution_clock::now();
        timer->startSubTask("ComputeXors", iteration);
        computeXors();
        timer->endSubTask("ComputeXors", iteration);
        t2 = high_resolution_clock::now();

        duration = duration_cast<milliseconds>(t2 - t1).count();
        cout << "computeXors took in milliseconds: " << duration << endl;

        t1 = high_resolution_clock::now();
        timer->startSubTask("ReceiveAndCalc", iteration);
        receiveSenderXors();
        timer->endSubTask("ReceiveAndCalc", iteration);
        t2 = high_resolution_clock::now();

        duration = duration_cast<milliseconds>(t2 - t1).count();
        cout << "receiveXors took in milliseconds: " << duration << endl;
        auto end = high_resolution_clock::now();

        duration = duration_cast<milliseconds>(end - start).count();
        cout << "all protocol took in milliseconds: " << duration << endl;

    }

}

vector<unsigned char> Receiver::createDictionary(){
    dic->init();

    auto start = high_resolution_clock::now();
    auto success = dic->encode();

    if (success == 0){
        cout<<"failure!!"<<endl;
        exit(1);
    }
//    auto t1 = high_resolution_clock::now();
//
//    dic->fillTables();
//    auto t2 = high_resolution_clock::now();
//
//    auto duration = duration_cast<milliseconds>(t2-t1).count();
//    cout << "fillTables took in milliseconds: " << duration << endl;
//
//    t1 = high_resolution_clock::now();
//    dic->peeling();
//
//    t2 = high_resolution_clock::now();
//
//    duration = duration_cast<milliseconds>(t2-t1).count();
//    cout << "peeling took in milliseconds: " << duration << endl;
//
//    t1 = high_resolution_clock::now();
//    dic->generateExternalToolValues();
//    t2 = high_resolution_clock::now();
//
//    duration = duration_cast<milliseconds>(t2-t1).count();
//    cout << "calc equations took in milliseconds: " << duration << endl;
//
//    t1 = high_resolution_clock::now();
//    dic->unpeeling();
//
//    t2 = high_resolution_clock::now();
//
//    duration = duration_cast<milliseconds>(t2 - t1).count();
//    cout << "unpeeling took in milliseconds: " << duration << endl;
//
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start).count();
    cout << "all dictionary took in milliseconds: " << duration << endl;

//        dic->checkOutput();


    return dic->getVariables();
}

void Receiver::runOOS(vector<unsigned char> & sigma){

//    u64 setSize = hashSize;//1 << 5;
//    u64 numBin = hashSize*2.4; //2.4 * setSize;
//    u64 numOTs = sigma.size()/fieldSizeBytes;

    std::cout << "in runOOS() " << numOTs << "\n";

    std::string name = "n";
    IOService ios(0);
//    Session ep0(ios, "localhost", 1212, SessionMode::Server, name);
    Session ep1(ios, addressForOT, portForOT, SessionMode::Client, name);
    auto recvChl = ep1.addChannel(name, name);
//    auto sendChl = ep0.addChannel(name, name);

    LinearCode code;
    switch(fieldSize){
        case 65:
            code.load(mx65by448, sizeof(mx65by448));
            cout<<"load mx65by448"<<endl;
            break;
        case 72:
            code.load(mx72by462, sizeof(mx72by462));
            cout<<"load mx72by462"<<endl;
            break;
        case 84:
            code.load(mx84by495, sizeof(mx84by495));
            cout<<"load mx84by495"<<endl;
            break;
        case 90:
            code.load(mx90by495, sizeof(mx90by495));
            cout<<"load mx90by495"<<endl;
            break;
        case 132:
            code.load(mx132by583, sizeof(mx132by583));
            cout<<"load mx132by583"<<endl;
            break;

        case 138:
            code.load(mx138by594, sizeof(mx138by594));
            cout<<"load mx132by583"<<endl;
            break;

        case 144:
            code.load(mx144by605, sizeof(mx144by605));
            cout<<"load mx132by583"<<endl;
            break;

        case 150:
            code.load(mx150by616, sizeof(mx150by616));
            cout<<"load mx132by583"<<endl;
            break;

        case 156:
            code.load(mx156by627, sizeof(mx156by627));
            cout<<"load mx132by583"<<endl;
            break;

        case 162:
            code.load(mx162by638, sizeof(mx162by638));
            cout<<"load mx132by583"<<endl;
            break;

        case 168:
            code.load(mx168by649, sizeof(mx168by649));
            cout<<"load mx132by583"<<endl;
            break;

        case 174:
            code.load(mx174by660, sizeof(mx174by660));
            cout<<"load mx132by583"<<endl;
            break;

        case 210:
            code.load(mx210by732, sizeof(mx210by732));
            cout<<"load mx132by583"<<endl;
            break;

        case 217:
            code.load(mx217by744, sizeof(mx217by744));
            cout<<"load mx132by583"<<endl;
            break;

        case 231:
            code.load(mx231by768, sizeof(mx231by768));
            cout<<"load mx132by583"<<endl;
            break;

        case 238:
            code.load(mx238by776, sizeof(mx238by776));
            cout<<"load mx132by583"<<endl;
            break;

    }

//    PrtyMOtSender sender;
    recv.configure(isMalicious, 40, fieldSize);

    //Base OT - simulated
    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
    u64 baseCount = recv.getBaseOTCount();
    std::vector<block> baseRecv(baseCount);
    std::vector<std::array<block, 2>> baseSend(baseCount);
    BitVector baseChoice(baseCount);
    baseChoice.randomize(prng0);
    prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
    for (u64 i = 0; i < baseCount; ++i)
    {
        baseRecv[i] = baseSend[i][baseChoice[i]];
    }

//    sender.setBaseOts(baseRecv, baseChoice);
    recv.setBaseOts(baseSend);

    //OT
    PRNG prng1(_mm_set_epi32(4253465, 3434565, 234435, 23987025));
    recv.init(numOTs, prng1, recvChl);

    for (u64 i = 0; i < numOTs; i += stepSize)
    {
//        cout<<"step size "<<i<<endl;
        auto curStepSize = std::min<u64>(stepSize, numOTs - i);
//        cout<<"curStepSize "<<curStepSize<<endl;
        for (u64 k = 0; k < curStepSize; ++k)
        {
            recv.otCorrection(i + k, &sigma[fieldSizeBytes*(k + i)]);
        }
        recv.sendCorrection(recvChl, curStepSize);
//        cout<<"after send correction"<<endl;
    }
}

void Receiver::computeXors(){

    if (version.compare("2Tables") == 0 || version.compare("3Tables") == 0) {
        computeTablesXors();
    } else {
        computeStarXors();
    }
}

void Receiver::computeStarXors(){

    u64 baseCount = recv.getBaseOTCount();
    int blockSize = baseCount/128;

    vector<block> output(blockSize);
    vector<unsigned char> temp(blockSize*16);
    int size;

    xorsSet = unordered_set<uint64_t>(hashSize);

    auto start = high_resolution_clock::now();
    int smallTableSize = dic->getGamma();
    int binTableSize = dic->getTableSize()/(q+1);
    int smallTableStartIndex = binTableSize - smallTableSize;
    int numLookupTables = smallTableSize % 8 == 0 ? smallTableSize / 8 : smallTableSize / 8 + 1;

    vector<vector<vector<vector<block>>>> lookupTable(q+1, vector<vector<vector<block>>>(numLookupTables));

    for (int j=0; j<q + 1; j++){
        for (int i = 0; i < numLookupTables; i++) {
            preprocess(lookupTable[j][i], j*binTableSize + smallTableStartIndex + i * 8, blockSize);
        }
    }
    auto end = high_resolution_clock::now();
    int preprocessDuration = duration_cast<milliseconds>(end - start).count();
    cout<<"preprocess took = "<<preprocessDuration<<endl;
//    cout<<"baseCount = "<<baseCount<<endl;
//    cout<<"Xors:"<<endl;
    int duration = 0, indicesDuration = 0;
    for (int i=0; i<hashSize; i++){

        start = high_resolution_clock::now();
        vector<uint64_t> indices = dic->decOptimized(keys[i]);

        end = high_resolution_clock::now();
        indicesDuration += duration_cast<nanoseconds>(end - start).count();

        for (int j = 0; j < blockSize; j++) {
            output[j] = _mm_xor_si128(*(recv.mT0.data(indices[1]) + j), *(recv.mT0.data(indices[2]) + j));
        }
        for (int j = 0; j < blockSize; j++) {
            output[j] = _mm_xor_si128(output[j], *(recv.mT0.data(indices[3]) + j));
        }


        for (int k = 12; k < 15; k++) {
            for (int j = 0; j < blockSize; j++) {
                output[j] = _mm_xor_si128(output[j], *(recv.mT0.data(indices[k]) + j));
            }
        }

        start = high_resolution_clock::now();
        int binIndex = indices[0];
        for (int k = 0; k < numLookupTables; k++) {

            for (int j = 0; j < blockSize; j++) {
                output[j] = _mm_xor_si128(output[j], lookupTable[binIndex][k][indices[4+k]][j]);
            }
        }

        for (int k = 0; k < numLookupTables; k++) {

            for (int j = 0; j < blockSize; j++) {
                output[j] = _mm_xor_si128(output[j], lookupTable[q][k][indices[15+k]][j]);
            }
        }


        end = high_resolution_clock::now();

        duration += duration_cast<nanoseconds>(end - start).count();

        EVP_EncryptUpdate(aes, temp.data(), &size, (byte*)output.data(), blockSize*16);
//cout<<"size = "<<size<<endl;
        xorsSet.insert(((uint64_t*)temp.data())[0]);

//        for (int j=0; j<20;j++){
//            cout<<(int)(((byte*)output.data())[j])<<" ";
//        }
//        cout<<endl;
//        cout<<((uint64_t*)temp.data())[0]<<endl;

    }

    cout<<"get indices took = "<<indicesDuration<<endl;
    cout<<"xors little table took = "<<duration<<endl;

}

void Receiver::computeTablesXors(){

    u64 baseCount = recv.getBaseOTCount();
    int blockSize = baseCount/128;

    vector<block> output(blockSize);
    vector<unsigned char> temp(blockSize*16);
    int size;

    xorsSet = unordered_set<uint64_t>(hashSize);

    auto start = high_resolution_clock::now();
    int smallTableSize = dic->getGamma();
    int smallTableStartIndex = dic->getTableSize() - smallTableSize;
    int numLookupTables = smallTableSize % 8 == 0 ? smallTableSize / 8 : smallTableSize / 8 + 1;

    vector<vector<vector<block>>> lookupTable(numLookupTables);

    for (int i = 0; i < numLookupTables; i++) {
        preprocess(lookupTable[i], smallTableStartIndex + i * 8, blockSize);
    }

    auto end = high_resolution_clock::now();
    int preprocessDuration = duration_cast<milliseconds>(end - start).count();
    cout<<"preprocess took = "<<preprocessDuration<<endl;
//    cout<<"baseCount = "<<baseCount<<endl;
//    cout<<"Xors:"<<endl;
    int duration = 0, indicesDuration = 0;
    for (int i=0; i<hashSize; i++){

        start = high_resolution_clock::now();
        vector<uint64_t> indices = dic->decOptimized(keys[i]);

//        for (int k=0; k<indices.size(); k++) {
//            cout<<indices[k]<<" ";
//        }
//        cout<<endl;

        end = high_resolution_clock::now();
        indicesDuration += duration_cast<nanoseconds>(end - start).count();

        for (int j=0; j<blockSize; j++) {
            output[j] = _mm_xor_si128(*(recv.mT0.data(indices[0]) + j),*(recv.mT0.data(indices[1]) + j));
        }

        start = high_resolution_clock::now();
        if (version.compare("2Tables") == 0) {
            start = high_resolution_clock::now();
            for (int k = 0; k < numLookupTables; k++) {

                for (int j = 0; j < blockSize; j++) {
                    output[j] = _mm_xor_si128(output[j], lookupTable[k][indices[2+k]][j]);
                }
            }
        } else if (version.compare("3Tables") == 0) {

            for (int j=0; j<blockSize; j++) {
                output[j] = _mm_xor_si128(output[j],*(recv.mT0.data(indices[2]) + j));
            }
            start = high_resolution_clock::now();
            for (int k = 0; k < numLookupTables; k++) {

                for (int j = 0; j < blockSize; j++) {
                    output[j] = _mm_xor_si128(output[j], lookupTable[k][indices[3+k]][j]);
                }
            }
        }

        end = high_resolution_clock::now();

        duration += duration_cast<nanoseconds>(end - start).count();

        EVP_EncryptUpdate(aes, temp.data(), &size, (byte*)output.data(), blockSize*16);
//cout<<"size = "<<size<<endl;
        xorsSet.insert(((uint64_t*)temp.data())[0]);

//        for (int j=0; j<20;j++){
//            cout<<(int)(((byte*)output.data())[j])<<" ";
//        }
//        cout<<endl;
//        cout<<((uint64_t*)temp.data())[0]<<endl;

    }

    cout<<"get indices took = "<<indicesDuration<<endl;
    cout<<"xors little table took = "<<duration<<endl;

}

void Receiver::preprocess(vector<vector<block>> & lookupTable, int smallTableStartIndex, int blockSize){

    lookupTable.resize(256);
    uint64_t mask = 1;
    block zero = _mm_setzero_si128();

    for (int k=0; k<lookupTable.size(); k++) {
        lookupTable[k] = vector<block>(blockSize, zero);
        int index = k;

        for (int i=0; i<8; i++) {

            if ((index & mask) == 1) {
                for (int j = 0; j < blockSize; j++) {
                    lookupTable[k][j] = _mm_xor_si128(lookupTable[k][j], *(recv.mT0.data(smallTableStartIndex + i) + j));
                }
            }
            index = index >> 1;
        }
    }

}

void Receiver::receiveSenderXors(){

    uint64_t size;
    otherParty->getChannel()->read((byte*)&size, 8);


    vector<uint64_t> senderOutputs(size);
    otherParty->getChannel()->read((byte*)senderOutputs.data(), size*8);

//    for (int i=0; i<size/blockSize; i++) {
//        for (int j=0; j<20; j++) {
//            cout<<(int)(((byte*)(senderOutputs.data() + i*blockSize))[j])<< " ";
//        }
//        cout<<endl;
//    }

    uint64_t count = 0;
    for (int i=0; i<hashSize; i++){
        if (xorsSet.find(senderOutputs[i]) != xorsSet.end()){
            count++;
//            cout<<"element "<<i<<" is in the intersection"<<endl;
        }
    }

    cout<<"found "<<count<<" matches"<<endl;


}

void Receiver::checkVariables(vector<unsigned char> & variables){

cout<<"in check"<<endl;
    bool error = false;
    vector<unsigned char> check(fieldSizeBytes);
    cout<<"variables size = "<<variables.size()/fieldSizeBytes<<endl;
    for (int i=0; i<hashSize; i++){

//        ((StarDictionary*)dic)->checkOutput(keys[i], i);
//        auto check = dic->decode(keys[i]);
        auto indices = dic->dec(keys[i]);


        for (int j=0; j<fieldSizeBytes; j++) {
            check[j] = variables[indices[0]*fieldSizeBytes + j] ^ variables[indices[1]*fieldSizeBytes + j];
        }

        for (int k=2; k<indices.size(); k++) {
            for (int j=0; j<fieldSizeBytes; j++) {
                check[j] = check[j] ^ variables[indices[k]*fieldSizeBytes + j];
            }
        }

        for (int j=0; j<fieldSizeBytes; j++) {
            if (check[j] != vals[i*fieldSizeBytes + j]){
                error = true;
//                cout<<"error in check! xor of D values is not equal to val"<<endl;
            }
        }

    }

    if (error ==  false){
        cout<<"success!!! all D values equal to val"<<endl;
    } else {
        cout<<"error in check!"<<endl;
    }

}


Sender::Sender(int argc, char *argv[]) : ProtocolParty(argc, argv){
}

Sender::~Sender() {

}



void Sender::runOnline() {

    auto start = high_resolution_clock::now();
    auto t1 = high_resolution_clock::now();

    runOOS();
    auto t2 = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "run OT took in milliseconds: " << duration << endl;

    t1 = high_resolution_clock::now();

    computeXors();
    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "computeXors took in milliseconds: " << duration << endl;

    t1 = high_resolution_clock::now();

    sendXors();
    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "sendXors took in milliseconds: " << duration << endl;
    auto end = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(end-start).count();
    cout << "all protocol took in milliseconds: " << duration << endl;

}

void Sender::runOOS(){

//    u64 setSize = hashSize;//1 << 5;
//    u64 numBin = hashSize*2.4; //2.4 * setSize;
//    u64 numOTs = tableRealSize*2 + gamma;

    std::cout << "in runOOS() = " << numOTs << "\n";

    std::string name = "n";
    IOService ios(0);
    Session ep0(ios, addressForOT, portForOT, SessionMode::Server, name);
//    Session ep1(ios, "localhost", 1212, SessionMode::Client, name);
//    auto recvChl = ep1.addChannel(name, name);
    auto sendChl = ep0.addChannel(name, name);

    switch(fieldSize){
        case 65:
            code.load(mx65by448, sizeof(mx65by448));
            cout<<"load mx65by448"<<endl;
            break;
        case 72:
            code.load(mx72by462, sizeof(mx72by462));
            cout<<"load mx72by462"<<endl;
            break;
        case 84:
            code.load(mx84by495, sizeof(mx84by495));
            cout<<"load mx84by495"<<endl;
            break;
        case 90:
            code.load(mx90by495, sizeof(mx90by495));
            cout<<"load mx90by495"<<endl;
            break;
        case 132:
            code.load(mx132by583, sizeof(mx132by583));
            cout<<"load mx132by583"<<endl;
            break;

        case 138:
            code.load(mx138by594, sizeof(mx138by594));
            cout<<"load mx132by583"<<endl;
            break;

        case 144:
            code.load(mx144by605, sizeof(mx144by605));
            cout<<"load mx132by583"<<endl;
            break;

        case 150:
            code.load(mx150by616, sizeof(mx150by616));
            cout<<"load mx132by583"<<endl;
            break;

        case 156:
            code.load(mx156by627, sizeof(mx156by627));
            cout<<"load mx132by583"<<endl;
            break;

        case 162:
            code.load(mx162by638, sizeof(mx162by638));
            cout<<"load mx132by583"<<endl;
            break;

        case 168:
            code.load(mx168by649, sizeof(mx168by649));
            cout<<"load mx132by583"<<endl;
            break;

        case 174:
            code.load(mx174by660, sizeof(mx174by660));
            cout<<"load mx132by583"<<endl;
            break;

        case 210:
            code.load(mx210by732, sizeof(mx210by732));
            cout<<"load mx132by583"<<endl;
            break;

        case 217:
            code.load(mx217by744, sizeof(mx217by744));
            cout<<"load mx132by583"<<endl;
            break;

        case 231:
            code.load(mx231by768, sizeof(mx231by768));
            cout<<"load mx132by583"<<endl;
            break;

        case 238:
            code.load(mx238by776, sizeof(mx238by776));
            cout<<"load mx132by583"<<endl;
            break;

    }


//    PrtyMOtReceiver recv;
    sender.configure(isMalicious, 40, fieldSize);

    //Base OT - simulated
    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
    u64 baseCount = sender.getBaseOTCount();

    cout<<"base count = "<<baseCount<<endl;
    std::vector<block> baseRecv(baseCount);
    std::vector<std::array<block, 2>> baseSend(baseCount);
    baseChoice.resize(baseCount);
    baseChoice.randomize(prng0);
    prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
    for (u64 i = 0; i < baseCount; ++i)
    {
        baseRecv[i] = baseSend[i][baseChoice[i]];
    }

    sender.setBaseOts(baseRecv, baseChoice);
//    recv.setBaseOts(baseSend);

    //OT
    sender.init(numOTs, prng0, sendChl);

    // Get the random OT messages
    for (u64 i = 0; i < numOTs; i += stepSize)
    {
//        cout<<"step size "<<i<<endl;
        auto curStepSize = std::min<u64>(stepSize, numOTs - i);
//        cout<<"curStepSize "<<curStepSize<<endl;
        sender.recvCorrection(sendChl, curStepSize);
        for (u64 k = 0; k < curStepSize; ++k)
        {
            sender.otCorrection(i + k);
        }
//        cout<<"after correction"<<endl;
    }

}

void Sender::computeXors(){

    if (version.compare("2Tables") == 0 || version.compare("3Tables") == 0) {
        computeTablesXors();
    } else {
        computeStarXors();
    }
//        u64 baseCount = sender.getBaseOTCount();
////cout << "baseCount " << baseCount << endl;
//        int blockSize = baseCount / 128;
//        //cout << "blocksize " << blockSize << endl;
//        vector<block> output(blockSize);
//
////cout <<"before resize" << endl;
//        xors.resize(hashSize);
//        //cout << "after resize" << endl;
//        auto start = high_resolution_clock::now();
//        int smallTableSize = dic->getGamma();
//        int smallTableStartIndex = dic->getTableSize() - smallTableSize;
//        int numLookupTables = smallTableSize % 8 == 0 ? smallTableSize / 8 : smallTableSize / 8 + 1;
//
//        vector<vector<vector<block>>> lookupTable(numLookupTables);
//
//        if (version.compare("2Tables") == 0 || version.compare("3Tables") == 0) {
//            for (int i = 0; i < numLookupTables; i++) {
//                preprocess(lookupTable[i], smallTableStartIndex + i * 8, blockSize);
//            }
//        }
//        auto end = high_resolution_clock::now();
//        int preprocessDuration = duration_cast<milliseconds>(end - start).count();
//        cout << "preprocess took = " << preprocessDuration << endl;
//
//        vector<unsigned char> temp(blockSize * 16);
//        //  cout << "created temp" <<endl;
//        int size;
////    cout<<"indices = "<<endl;
//        for (int i = 0; i < hashSize; i++) {
//
//            vector<uint64_t> indices;
//
//            if (version.compare("2Tables") == 0 || version.compare("3Tables") == 0) {
//                indices = ((OBDTables *) dic)->decOptimized(keys[i]);
//            } else {
//                indices = dic->dec(keys[i]);
//            }
//
////        for (int k=0; k<indices.size(); k++) {
////            cout<<indices[k]<<" ";
////        }
////        cout<<endl;
//
//            for (int j = 0; j < blockSize; j++) {
//                output[j] = _mm_xor_si128(*(sender.mT.data(indices[0]) + j), *(sender.mT.data(indices[1]) + j));
//            }
//
//            if (version.compare("2Tables") == 0) {
//                start = high_resolution_clock::now();
//                for (int k = 0; k < numLookupTables; k++) {
//
//                    for (int j = 0; j < blockSize; j++) {
//                        output[j] = _mm_xor_si128(output[j], lookupTable[k][indices[2 + k]][j]);
//                    }
//                }
//            } else if (version.compare("3Tables") == 0) {
//
//                for (int j = 0; j < blockSize; j++) {
//                    output[j] = _mm_xor_si128(output[j], *(sender.mT.data(indices[2]) + j));
//                }
//                start = high_resolution_clock::now();
//                for (int k = 0; k < numLookupTables; k++) {
//
//                    for (int j = 0; j < blockSize; j++) {
//                        output[j] = _mm_xor_si128(output[j], lookupTable[k][indices[3 + k]][j]);
//                    }
//                }
//            } else {
//                start = high_resolution_clock::now();
//                for (int k = 2; k < indices.size(); k++) {
//                    for (int j = 0; j < blockSize; j++) {
//                        output[j] = _mm_xor_si128(output[j], *(sender.mT.data(indices[k]) + j));
//                    }
//                }
//            }
//
//            std::array<block, 10> codeword = {ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock,
//                                              ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock};
//            memcpy(codeword.data(), vals.data() + i * fieldSizeBytes, fieldSizeBytes);
//            code.encode((u8 *) codeword.data(), (u8 *) codeword.data());
//
//            for (int j = 0; j < blockSize; j++) {
//                codeword[j] = _mm_and_si128(codeword[j], ((block *) baseChoice.data())[j]);
//            }
//
//            for (int j = 0; j < blockSize; j++) {
//                output[j] = _mm_xor_si128(output[j], codeword[j]);
//            }
//
////	cout << "before EVP" << endl;
//
//            EVP_EncryptUpdate(aes, temp.data(), &size, (byte *) output.data(), blockSize * 16);
//
//            xors[i] = ((uint64_t *) temp.data())[0];
////        for (int j=0; j<20;j++){
////            cout<<(int)(((byte*)output.data())[j])<<" ";
////        }
////        cout<<endl;
////
////        cout<<xors[i]<<endl;
//        }
//    }


}

void Sender::computeStarXors(){
    u64 baseCount = sender.getBaseOTCount();
    int blockSize = baseCount/128;

    vector<block> output(blockSize);
    vector<unsigned char> temp(blockSize*16);
    int size;

    xors.resize(hashSize);

    auto start = high_resolution_clock::now();
    int smallTableSize = dic->getGamma();
    int binTableSize = dic->getTableSize()/(q+1);
    int smallTableStartIndex = binTableSize - smallTableSize;
    int numLookupTables = smallTableSize % 8 == 0 ? smallTableSize / 8 : smallTableSize / 8 + 1;

    vector<vector<vector<vector<block>>>> lookupTable(q+1, vector<vector<vector<block>>>(numLookupTables));

    for (int j=0; j<q + 1; j++){
        for (int i = 0; i < numLookupTables; i++) {
            preprocess(lookupTable[j][i], j*binTableSize + smallTableStartIndex + i * 8, blockSize);
        }
    }
    auto end = high_resolution_clock::now();
    int preprocessDuration = duration_cast<milliseconds>(end - start).count();
    cout<<"preprocess took = "<<preprocessDuration<<endl;
//    cout<<"baseCount = "<<baseCount<<endl;
//    cout<<"Xors:"<<endl;
    int duration = 0, indicesDuration = 0;
    for (int i=0; i<hashSize; i++){

        start = high_resolution_clock::now();
        vector<uint64_t> indices = dic->decOptimized(keys[i]);

        end = high_resolution_clock::now();
        indicesDuration += duration_cast<nanoseconds>(end - start).count();

        for (int j = 0; j < blockSize; j++) {
            output[j] = _mm_xor_si128(*(sender.mT.data(indices[1]) + j), *(sender.mT.data(indices[2]) + j));
        }
        for (int j = 0; j < blockSize; j++) {
            output[j] = _mm_xor_si128(output[j], *(sender.mT.data(indices[3]) + j));
        }


        for (int k = 12; k < 15; k++) {
            for (int j = 0; j < blockSize; j++) {
                output[j] = _mm_xor_si128(output[j], *(sender.mT.data(indices[k]) + j));
            }
        }

        start = high_resolution_clock::now();
        int binIndex = indices[0];

        for (int k = 0; k < numLookupTables; k++) {

            for (int j = 0; j < blockSize; j++) {
                output[j] = _mm_xor_si128(output[j], lookupTable[binIndex][k][indices[4+k]][j]);
            }
        }

        for (int k = 0; k < numLookupTables; k++) {

            for (int j = 0; j < blockSize; j++) {
                output[j] = _mm_xor_si128(output[j], lookupTable[q][k][indices[15+k]][j]);
            }
        }


        end = high_resolution_clock::now();

        duration += duration_cast<nanoseconds>(end - start).count();

        std::array<block, 10> codeword = { ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock };
        memcpy(codeword.data(), vals.data() + i*fieldSizeBytes, fieldSizeBytes);
//        std::array<block, 10> tempcodeword = { ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock };
        code.encode((u8*)codeword.data(), (u8*)codeword.data());

        for (int j = 0; j < blockSize; j++) {
            codeword[j] = _mm_and_si128(codeword[j], ((block*)baseChoice.data())[j]);
        }

        for (int j=0; j<blockSize; j++) {
            output[j] = _mm_xor_si128(output[j], codeword[j]);
        }


        EVP_EncryptUpdate(aes, temp.data(), &size, (byte*)output.data(), blockSize*16);

        xors[i] = ((uint64_t*)temp.data())[0];

//        for (int j=0; j<20;j++){
//            cout<<(int)(((byte*)output.data())[j])<<" ";
//        }
//        cout<<endl;
//        cout<<((uint64_t*)temp.data())[0]<<endl;

    }

    cout<<"get indices took = "<<indicesDuration<<endl;
    cout<<"xors little table took = "<<duration<<endl;

}

void Sender::computeTablesXors(){
    u64 baseCount = sender.getBaseOTCount();
//cout << "baseCount " << baseCount << endl;
    int blockSize = baseCount/128;
    //cout << "blocksize " << blockSize << endl;
    vector<block> output(blockSize);

//cout <<"before resize" << endl;
    xors.resize(hashSize);
    //cout << "after resize" << endl;
    auto start = high_resolution_clock::now();
    int smallTableSize = dic->getGamma();
    int smallTableStartIndex = dic->getTableSize() - smallTableSize;
    int numLookupTables = smallTableSize % 8 == 0 ? smallTableSize / 8 : smallTableSize / 8 + 1;

    vector<vector<vector<block>>> lookupTable(numLookupTables);

    for (int i = 0; i < numLookupTables; i++) {
        preprocess(lookupTable[i], smallTableStartIndex + i * 8, blockSize);
    }

    auto end = high_resolution_clock::now();
    int preprocessDuration = duration_cast<milliseconds>(end - start).count();
    cout<<"preprocess took = "<<preprocessDuration<<endl;

    vector<unsigned char> temp(blockSize*16);
    //  cout << "created temp" <<endl;
    int size;
//    cout<<"indices = "<<endl;
    for (int i=0; i<hashSize; i++){

        vector<uint64_t> indices = dic->decOptimized(keys[i]);

        for (int j=0; j<blockSize; j++) {
            output[j] = _mm_xor_si128(*(sender.mT.data(indices[0]) + j),*(sender.mT.data(indices[1]) + j));
        }

        if (version.compare("2Tables") == 0) {
            start = high_resolution_clock::now();
            for (int k = 0; k < numLookupTables; k++) {

                for (int j = 0; j < blockSize; j++) {
                    output[j] = _mm_xor_si128(output[j], lookupTable[k][indices[2+k]][j]);
                }
            }
        } else if (version.compare("3Tables") == 0) {

            for (int j=0; j<blockSize; j++) {
                output[j] = _mm_xor_si128(output[j],*(sender.mT.data(indices[2]) + j));
            }
            start = high_resolution_clock::now();
            for (int k = 0; k < numLookupTables; k++) {

                for (int j = 0; j < blockSize; j++) {
                    output[j] = _mm_xor_si128(output[j], lookupTable[k][indices[3+k]][j]);
                }
            }
        }

        std::array<block, 10> codeword = { ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock };
        memcpy(codeword.data(), vals.data() + i*fieldSizeBytes, fieldSizeBytes);
        code.encode((u8*)codeword.data(), (u8*)codeword.data());

        for (int j = 0; j < blockSize; j++) {
            codeword[j] = _mm_and_si128(codeword[j], ((block*)baseChoice.data())[j]);
        }

        for (int j=0; j<blockSize; j++) {
            output[j] = _mm_xor_si128(output[j], codeword[j]);
        }


        EVP_EncryptUpdate(aes, temp.data(), &size, (byte*)output.data(), blockSize*16);

        xors[i] = ((uint64_t*)temp.data())[0];
    }

}

void Sender::preprocess(vector<vector<block>> & lookupTable, int smallTableStartIndex, int blockSize){

    lookupTable.resize(256);
    uint64_t mask = 1;
    block zero = _mm_setzero_si128();

    for (int k=0; k<lookupTable.size(); k++) {
        lookupTable[k] = vector<block>(blockSize, zero);
        int index = k;

        for (int i=0; i<8; i++) {

            if ((index & mask) == 1) {
                for (int j = 0; j < blockSize; j++) {
                    lookupTable[k][j] = _mm_xor_si128(lookupTable[k][j], *(sender.mT.data(smallTableStartIndex + i) + j));
                }
            }
            index = index >> 1;
        }
    }

}

void Sender::sendXors(){

    int64_t size = xors.size();

    otherParty->getChannel()->write((byte*)&size, 8);
    otherParty->getChannel()->write((byte*)xors.data(), size*8);
}


