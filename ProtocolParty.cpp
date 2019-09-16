//
// Created by moriya on 7/24/19.
//

#include "ProtocolParty.h"

ProtocolParty::ProtocolParty(int argc, char* argv[]) : Protocol("ObliviousDictionary", argc, argv)
{

    partyId = stoi(this->getParser().getValueByKey(arguments, "partyID"));

    this->times = stoi(this->getParser().getValueByKey(arguments, "internalIterationsNumber"));
    this->hashSize = stoi(this->getParser().getValueByKey(arguments, "hashSize"));
    this->fieldSize = stoi(this->getParser().getValueByKey(arguments, "fieldSize"));
    fieldSizeBytes = fieldSize/8 + 1;

    vector<string> subTaskNames{"Online"};
    timer = new Measurement(*this, subTaskNames);

    MPCCommunication comm;
    string partiesFile = this->getParser().getValueByKey(arguments, "partiesFile");

    reportStatistics = stoi(this->getParser().getValueByKey(arguments, "reportStatistics"));


    if(reportStatistics==0) {
    otherParty = comm.setCommunication(io_service, partyId, 2, partiesFile)[0];


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

    dic = new ObliviousDictionary(hashSize, fieldSize);
    dic->setReportStatstics(reportStatistics);

}

Receiver::~Receiver(){
    delete dic;
}

void Receiver::runOnline() {

    auto start = high_resolution_clock::now();
    auto t1 = high_resolution_clock::now();

    auto sigma = createDictionary();
    auto t2 = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "createDictionary took in milliseconds: " << duration << endl;

    t1 = high_resolution_clock::now();

    runOT(sigma);
    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "run OT took in milliseconds: " << duration << endl;


    auto end = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(end - start).count();
    cout << "all protocol took in milliseconds: " << duration << endl;

}

vector<byte> Receiver::createDictionary(){
    dic->init();

    auto start = high_resolution_clock::now();
    auto t1 = high_resolution_clock::now();

    dic->fillTables();
    auto t2 = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "fillTables took in milliseconds: " << duration << endl;

    t1 = high_resolution_clock::now();
    dic->peeling();

    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "peeling took in milliseconds: " << duration << endl;

    t1 = high_resolution_clock::now();
    dic->generateExternalToolValues();
    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "calc equations took in milliseconds: " << duration << endl;

    if(reportStatistics==0) {
        t1 = high_resolution_clock::now();
        dic->unpeeling();

        t2 = high_resolution_clock::now();

        duration = duration_cast<milliseconds>(t2 - t1).count();
        cout << "unpeeling took in milliseconds: " << duration << endl;

        auto end = high_resolution_clock::now();

        duration = duration_cast<milliseconds>(end - start).count();
        cout << "all protocol took in milliseconds: " << duration << endl;

        dic->checkOutput();

    }
    return dic->getVariables();
}

void setBaseOts(NcoOtExtSender& sender,
                NcoOtExtReceiver& recv,
                Channel& sendChl,
                Channel& recvChl)
{
    u64 baseCount = sender.getBaseOTCount();

    std::vector<block> baseRecv(baseCount);
    std::vector<std::array<block, 2>> baseSend(baseCount);
    BitVector baseChoice(baseCount);
    PRNG prng0(ZeroBlock);
    baseChoice.randomize(prng0);

    prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
    for (u64 i = 0; i < baseCount; ++i)
    {
        baseRecv[i] = baseSend[i][baseChoice[i]];
    }

    auto a = std::async([&]() {sender.setBaseOts(baseRecv, baseChoice, sendChl); });
    recv.setBaseOts(baseSend, prng0, recvChl);
    a.get();
}

void Receiver::runOT(vector<byte> & sigma){


    cout<<"in runOT receiver"<<endl;

    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

    u64 numOTs = sigma.size()/fieldSizeBytes;

    OosNcoOtReceiver recv;
    recv.configure(true, 40, fieldSize);

    std::string name = "n";
    IOService ios(0);
    Session ep1(ios, "localhost", 1212, SessionMode::Client, name);
    auto recvChl = ep1.addChannel(name, name);

//    setBaseOts(sender, recv, sendChl, recvChl);
//    u64 baseCount = recv.getBaseOTCount();
//
//    std::vector<std::array<block, 2>> baseSend(baseCount);
//    prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
//
//    osuCrypto::NaorPinkas base;
//    base.send(baseSend, prng0, recvChl, 2);
    recv.genBaseOts(prng0, recvChl);

    auto messageCount = 1ull << fieldSize;
//    Matrix<block> sendMessage(numOTs, messageCount);
    std::vector<block> recvMessage(numOTs);

    std::vector<u64> choices(numOTs);
    for (u64 i = 0; i < choices.size(); ++i)
    {
//        choices[i] = *(u64*)(sigma.data() + i*fieldSizeBytes);
        choices[i] = prng0.get<u8>();
    }

    recv.receiveChosen(messageCount, recvMessage, choices, prng0, recvChl);

//    setThreadName("Sender");
//
//    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
//    PRNG prng1(_mm_set_epi32(4253465, 3434565, 234435, 23987025));
//
//    u64 numOTs = 128 * 2;
//
//
//    std::string name = "n";
//    IOService ios(0);
//    Session ep0(ios, "localhost", 1212, SessionMode::Server, name);
//    Session ep1(ios, "localhost", 1212, SessionMode::Client, name);
//    auto recvChl = ep1.addChannel(name, name);
//    auto sendChl = ep0.addChannel(name, name);
//
//
//    OosNcoOtSender sender;
//    OosNcoOtReceiver recv;
//
//    sender.configure(true, 40, 76);
//    recv.configure(true, 40, 76);
//
//    if (1)
//    {
//        setBaseOts(sender, recv, sendChl, recvChl);
//        //for (u64 i = 0; i < sender.mBaseChoiceBits.size(); ++i)
//        //{
//        //    auto b = sender.mBaseChoiceBits[i];
//        //    if (neq(sender.mGens[i].getSeed(), recv.mGens[i][b].getSeed()))
//        //        throw RTE_LOC;
//        //}
//    }
//    else
//    {
//        u64 baseCount = sender.getBaseOTCount();
//        //u64 codeSize = (baseCount + 127) / 128;
//
//        std::vector<block> baseRecv(baseCount);
//        std::vector<std::array<block, 2>> baseSend(baseCount);
//        BitVector baseChoice(baseCount);
//        baseChoice.randomize(prng0);
//
//        prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
//        for (u64 i = 0; i < baseCount; ++i)
//        {
//            baseRecv[i] = baseSend[i][baseChoice[i]];
//        }
//
//        auto a = std::async([&]() {sender.setBaseOts(baseRecv, baseChoice, sendChl); });
//        recv.setBaseOts(baseSend, prng0, recvChl);
//        a.get();
//    }
//
//cout<<"after setBaseOts"<<endl;
//    testNco(sender, numOTs, prng0, sendChl, recv, prng1, recvChl);
//cout<<"after test"<<endl;
////    auto v = std::async([&] {
////        recv.check(recvChl, toBlock(322334));
////    });
////
////    try {
////        sender.check(sendChl,toBlock(324));
////    }
////    catch (...)
////    {
////        //sendChl.mBase->mLog;
////    }
////    v.get();
////
//    auto sender2 = sender.split();
//    auto recv2 = recv.split();
//
//    testNco(*sender2, numOTs, prng0, sendChl, *recv2, prng1, recvChl);
//    cout<<"after test 2"<<endl;


}

Sender::Sender(int argc, char *argv[]) : ProtocolParty(argc, argv){


}

Sender::~Sender() {

}



void Sender::runOnline() {

    auto start = high_resolution_clock::now();
    auto t1 = high_resolution_clock::now();

//    dic->readData(otherParty);
    runOT();
    auto t2 = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "read data took in milliseconds: " << duration << endl;

    auto end = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(end-start).count();
    cout << "all protocol took in milliseconds: " << duration << endl;



}

void Sender::runOT(){
    cout<<"in runOT sender"<<endl;
    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987025));

    u64 numOTs = 314;

    OosNcoOtSender sender;
    sender.configure(true, 40, fieldSize);

    std::string name = "n";
    IOService ios(0);
    Session ep0(ios, "localhost", 1212, SessionMode::Server, name);
    auto sendChl = ep0.addChannel(name, name);

////    setBaseOts(sender, recv, sendChl, recvChl);
//    u64 baseCount = sender.getBaseOTCount();
//
//    std::vector<block> baseRecv(baseCount);
//    BitVector baseChoice(baseCount);
//    baseChoice.randomize(prng0);
//
//    NaorPinkas base;
//    base.receive(baseChoice, baseRecv,prng0, sendChl, 2);

    sender.genBaseOts(prng0, sendChl);


    auto messageCount = 1ull << fieldSize;
    Matrix<block> sendMessage(numOTs, messageCount);

    prng0.get(sendMessage.data(), sendMessage.size());


    sender.sendChosen(sendMessage, prng0, sendChl);

//    for (u64 i = 0; i < choices.size(); ++i)
//    {
//        if (neq(recvMessage[i], sendMessage(i, choices[i])))
//            cout<<"bad message"<<endl;
//    }
}

