//
// Created by moriya on 7/24/19.
//

#include "ProtocolParty.h"

ProtocolParty::ProtocolParty(int argc, char* argv[]) : Protocol("ObliviousDictionary", argc, argv)
{

    partyId = stoi(this->getParser().getValueByKey(arguments, "partyID"));

    this->times = stoi(this->getParser().getValueByKey(arguments, "internalIterationsNumber"));
    this->hashSize = stoi(this->getParser().getValueByKey(arguments, "hashSize"));

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

    dic = new ObliviousDictionary(hashSize);
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

GF2EVector Receiver::createDictionary(){
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

void testNco(
        NcoOtExtSender &sender,
        const u64 &numOTs,
        PRNG &prng0,
        Channel &sendChl,
        NcoOtExtReceiver &recv,
        PRNG &prng1,
        Channel &recvChl)
{


    u64 stepSize = 33;
    std::vector<block> inputs(stepSize);
    setThreadName("Receiver");

    for (size_t j = 0; j < 10; j++)
    {
        // perform the init on each of the classes. should be performed concurrently
        auto thrd = std::thread([&]() {
            setThreadName("Sender");
            sender.init(numOTs, prng0, sendChl);
        });
        recv.init(numOTs, prng1, recvChl);
        thrd.join();

        std::vector<block> encoding1(stepSize), encoding2(stepSize);

        // Get the random OT messages
        for (u64 i = 0; i < numOTs; i += stepSize)
        {
            auto curStepSize = std::min<u64>(stepSize, numOTs - i);
            std::vector<u8> skips(curStepSize);

            prng0.get(inputs.data(), inputs.size());
            prng0.get((bool*)skips.data(), skips.size());

            for (u64 k = 0; k < curStepSize; ++k)
            {

                // The receiver MUST encode before the sender. Here we are only calling encode(...)
                // for a single i. But the receiver can also encode many i, but should only make one
                // call to encode for any given value of i.
                if (skips[k])
                {
                    recv.zeroEncode(i + k);
                }
                else {
                    recv.encode(i + k, &inputs[k], (u8*)&encoding1[k], sizeof(block));
                }
            }

            // This call will send to the other party the next "curStepSize " corrections to the sender.
            // If we had made more or less calls to encode above (for contigious i), then we should replace
            // curStepSize  with however many calls we made. In an extreme case, the reciever can perform
            // encode for i \in {0, ..., numOTs - 1}  and then call sendCorrection(recvChl, numOTs).
            recv.sendCorrection(recvChl, curStepSize);

            // receive the next curStepSize  correction values. This allows the sender to now call encode
            // on the next curStepSize  OTs.
            sender.recvCorrection(sendChl, curStepSize);

            for (u64 k = 0; k < curStepSize; ++k)
            {
                // the sender can now call encode(i, ...) for k \in {0, ..., i}.
                // Lets encode the same input and then we should expect to
                // get the same encoding.
                sender.encode(i + k, &inputs[k], (u8*)&encoding2[k], sizeof(block));

                // check that we do in fact get the same value
                if (! skips[k] && neq(encoding1[k], encoding2[k]))
                    cout<<"ot[" + std::to_string(i+k) + "] not equal " LOCATION<<endl;

                // In addition to the sender being able to obtain the same value as the receiver,
                // the sender can encode and other codeword. This should result in a different
                // encoding.
                inputs[k] = prng0.get<block>();

                sender.encode(i + k, &inputs[k], (u8*)&encoding2[k], sizeof(block));

                if (eq(encoding1[k], encoding2[k]))
                    cout<<"error"<<endl;
            }
        }
    }

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

void Receiver::runOT(GF2EVector sigma){
    setThreadName("Sender");

    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
    PRNG prng1(_mm_set_epi32(4253465, 3434565, 234435, 23987025));

    u64 numOTs = 128 * 2;


    std::string name = "n";
    IOService ios(0);
    Session ep0(ios, "localhost", 1212, SessionMode::Server, name);
    Session ep1(ios, "localhost", 1212, SessionMode::Client, name);
    auto recvChl = ep1.addChannel(name, name);
    auto sendChl = ep0.addChannel(name, name);


    OosNcoOtSender sender;
    OosNcoOtReceiver recv;

    sender.configure(true, 40, 50);
    recv.configure(true, 40, 50);

    if (1)
    {
        setBaseOts(sender, recv, sendChl, recvChl);
        //for (u64 i = 0; i < sender.mBaseChoiceBits.size(); ++i)
        //{
        //    auto b = sender.mBaseChoiceBits[i];
        //    if (neq(sender.mGens[i].getSeed(), recv.mGens[i][b].getSeed()))
        //        throw RTE_LOC;
        //}
    }
    else
    {
        u64 baseCount = sender.getBaseOTCount();
        //u64 codeSize = (baseCount + 127) / 128;

        std::vector<block> baseRecv(baseCount);
        std::vector<std::array<block, 2>> baseSend(baseCount);
        BitVector baseChoice(baseCount);
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

cout<<"after setBaseOts"<<endl;
    testNco(sender, numOTs, prng0, sendChl, recv, prng1, recvChl);
cout<<"after test"<<endl;
//    auto v = std::async([&] {
//        recv.check(recvChl, toBlock(322334));
//    });
//
//    try {
//        sender.check(sendChl,toBlock(324));
//    }
//    catch (...)
//    {
//        //sendChl.mBase->mLog;
//    }
//    v.get();
//
    auto sender2 = sender.split();
    auto recv2 = recv.split();

    testNco(*sender2, numOTs, prng0, sendChl, *recv2, prng1, recvChl);
    cout<<"after test 2"<<endl;


}

Sender::Sender(int argc, char *argv[]) : ProtocolParty(argc, argv){


}

Sender::~Sender() {

}



void Sender::runOnline() {

    auto start = high_resolution_clock::now();
    auto t1 = high_resolution_clock::now();

//    dic->readData(otherParty);
    auto t2 = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "read data took in milliseconds: " << duration << endl;

    auto end = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(end-start).count();
    cout << "all protocol took in milliseconds: " << duration << endl;



}

