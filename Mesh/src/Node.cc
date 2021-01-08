#include "Node.h"

Define_Module(Node);

void Node::initialize()
{
    MyReadFile.open(std::to_string(getIndex()) + "_send.txt");
    MyoutputFile.open(std::to_string(getIndex()) + "_recive.txt", std::fstream::out);
    /*
    dest = 0;
    double interval2 = exponential(1 / par("lambda").doubleValue());
    if(getIndex() == 0)
        scheduleAt(simTime() + interval2, new FramedMessage_Base("Continue"));
    else if(getIndex() == 1)
        scheduleAt(simTime() + interval2, new FramedMessage_Base("Continue"));
    else
        return;
    */
    for (int i=0; i<MaxSEQ+1; i++)
        timers[i] = nullptr;
    // Initialize Noisy Channel Members
    modification_probability = par("modification_probability").doubleValue();
    modificationFrameLowerBit = par("modificationFrameLowerBit").intValue();
    modificationFrameUpperBit = par("modificationFrameUpperBit").intValue();
    loss_probability = par("loss_probability").doubleValue();
    duplication_probability = par("duplication_probability").doubleValue();
    delay_probability = par("delay_probability").doubleValue();
    delay_lambda = par("delay_lambda").doubleValue();

    nextFrameToSend = 0;
    AckExpected = 0;
    frameExpected = 0;
    nBuffered = 0;

    totalGenerated = 0;
    totalDropped = 0;
    totalRetransmitted = 0;
    usefulTransmittedSize = 0;
    totaltransmittedSize = 0;
}

void Node::finish()
{
    std::ofstream myfile;
    std::stringstream ss;
    ss<<getIndex()<<".txt";
    myfile.open (ss.str());
    myfile << "Node "<<getIndex()<<" statistics for total of "<<simTime()<<" seconds: \n\n";
    myfile <<" Total number of generated frames  = "<<totalGenerated<<std::endl;
    myfile <<" Total number of dropped frames = "<<totalDropped<<std::endl;
    myfile <<" Total number of re-transmitted frames = "<<totalRetransmitted<<std::endl;
    myfile <<" Efficiency of transmission = "<<(usefulTransmittedSize * 1.0 / totaltransmittedSize) * 100 <<" %"<<std::endl;
    //myfile.close();
}
void Node::handleMessage(cMessage *msg)
{
    FramedMessage_Base* fmsg = check_and_cast<FramedMessage_Base*>(msg);
    if (fmsg->isSelfMessage()) {
        EV << "Sending to "<< dest <<" from source " << getIndex() << "\n";
        if(strcmp(msg->getName(), "Continue") != 0)
            goBackN(fmsg, 2);
        else
            goBackN(fmsg, 1);
    }
    else {

        if(strcmp(fmsg->getName(), "start") == 0){
            dest = atoi(fmsg->getPayload());
            scheduleAt(simTime() + interval, new FramedMessage_Base("Continue"));
        } else if (strcmp(fmsg->getName(), "end") == 0){
            return;
        } else {
            goBackN(fmsg, 0);
            if (atoi(fmsg->getName()) == getIndex())
                bubble("Message received");
            else
                bubble("Wrong destination");
        }
        // delete fmsg;
    }
}
void Node::start_Timer()
{
    if(timers[nextFrameToSend] != nullptr)
    {
        EV<<"Timer "<<nextFrameToSend <<" is already created"<<std::endl;
        cancelAndDelete(timers[nextFrameToSend]);
    }
    std::stringstream ss;
    ss<<nextFrameToSend;
    timers[nextFrameToSend] =  new FramedMessage_Base(ss.str().c_str());
    scheduleAt(simTime() + timeOut, timers[nextFrameToSend]);
    EV<<"CREATED Timer "<<nextFrameToSend<<std::endl;
}
void Node::send_Data(bool useful)
{
    FramedMessage_Base* fmsg = new FramedMessage_Base("");
    fmsg->setPayload(buffer[nextFrameToSend].c_str());
    fmsg->setSeq_num(nextFrameToSend);
    fmsg->setAck_num((frameExpected + MaxSEQ) % (MaxSEQ + 1));
    totalGenerated ++;
    EV << "Payload " << fmsg->getPayload() << std::endl;
    EV << "Sequence number " << fmsg->getSeq_num() << std::endl;
    EV << "Ack number " << fmsg->getAck_num() << std::endl;

//    bool duplicated = false;
//    send(fmsg, "outs", dest);

    bool duplicated = NoisySend(fmsg, useful);
    start_Timer();

    nextFrameToSend += (duplicated)? 0 : 1;
    nextFrameToSend %= (MaxSEQ + 1);
//    fileIterator += (duplicated)? 0 : 1;
}
bool Node::between(int a, int b, int c)
{
    return (((a<=b) && (b<c)) || ((c<a) && (a<=b)) || ((b<c) &&(c<a)))? true : false;
}

void Node::goBackN(FramedMessage_Base* msg, int whichCase)
{
    switch (whichCase)
    {
    case 0:
    {
        EV << "Frame arrival " << std::endl;
        EV << "Payload " << msg->getPayload() << std::endl;
        EV << "Sequence number received " << msg->getSeq_num() << " and expected " << frameExpected << std::endl;
        EV << "Ack number " << msg->getAck_num() << std::endl;

        if(frameExpected == msg->getSeq_num())
        {
            frameExpected++;
            frameExpected %= (MaxSEQ + 1);
            std::string frame = bitDeStuffing(msg->getPayload());
            std::string packet = errorDetectionCorrectionHamming(frame);
            MyoutputFile << binary2string(packet) << std::endl;
        }

        EV<<"ACKEXPECTED "<<AckExpected<<std::endl;
        EV<<"GETACKNUM "<<msg->getAck_num()<<std::endl;
        EV<<"NEXTTOSEND "<<nextFrameToSend<<std::endl;

        while(between(AckExpected, msg->getAck_num(), nextFrameToSend))
        {
            nBuffered--;
            if(timers[AckExpected] != nullptr)
            {
                EV<<"CANCELANDDELETE Timer "<<AckExpected<<std::endl;
                cancelAndDelete(timers[AckExpected]);
                timers[AckExpected] = nullptr;
            }
            AckExpected++;
            AckExpected %= (MaxSEQ + 1);

        }
        break;
    }
    // TODO: Read Next frame from file
    case 1:
        {
            std::string text;
            if(getline (MyReadFile, text)){
                std::string line = string2bits(text);
                std::string packet = hammingGenerator(line);
                std::string frame = bitStuffing(packet);
                buffer[nextFrameToSend] = frame;
            } else {
                buffer[nextFrameToSend] = "end";
            }

            EV<<nBuffered<<std::endl;
            if (nBuffered < MaxSEQ){
                EV << "Send next frame " << std::endl;
                nBuffered++;
                // delete(msg);
                send_Data(true);
                if(buffer[nextFrameToSend] != "end")
                    scheduleAt(simTime() + interval, new FramedMessage_Base("Continue"));
            }
            else
                scheduleAt(simTime() + interval, new FramedMessage_Base("Continue"));
            break;
        }
    // Re-send if there is timeout
    case 2:
//        delete(msg);
        EV << "Repeat " << std::endl;
        nextFrameToSend = AckExpected;
        for(int i = 1; i <= nBuffered; i++)
        {
            totalRetransmitted ++;
            send_Data(false);
        }
    }
}
std::string Node::modifyMsg(std::string msg)
{
    modificationFrameLowerBit = std::max(0, modificationFrameLowerBit);
    modificationFrameUpperBit = std::min(modificationFrameUpperBit, int(msg.size()));
    int bit = int(uniform(modificationFrameLowerBit, modificationFrameUpperBit));
    msg[bit] = (msg[bit] == '0') ? '1' : '0';
    EV<<"Modified"<<std::endl;
    return msg;
}
bool Node::NoisySend(FramedMessage_Base*& msg, bool useful)
{
    bool dropped = false;
    if(uniform(0,1) < modification_probability)
        msg->setPayload(modifyMsg(msg->getPayload()).c_str());
    if(uniform(0,1) < loss_probability)
    {
        EV<<"Dropped"<<std::endl;
        totalDropped ++;
        dropped = true;
    }
    else if(uniform(0,1) < delay_probability)
    {
        EV<<"Delayed";
        double delay = exponential(1 / delay_lambda);
        sendDelayed(msg, delay, "outs", dest);
    }
    else if(uniform(0,1) < duplication_probability)
    {
        EV<<"Duplicated"<<std::endl;
        send(msg, "outs", dest);
        send(new FramedMessage_Base(*msg), "outs", dest);
        totaltransmittedSize += sizeof(msg->getPayload()) + sizeof(msg->getSeq_num()) + sizeof(msg->getAck_num());
//        totaltransmittedSize += sizeof(*msg);
    }
    else
        send(msg, "outs", dest);
    if(!dropped)
    {
        totaltransmittedSize += sizeof(msg->getPayload()) + sizeof(msg->getSeq_num()) + sizeof(msg->getAck_num());
        //        totaltransmittedSize += sizeof(*msg);
        if(useful)
            usefulTransmittedSize += sizeof(msg->getPayload());        }
    return false;
}
std::string Node::bitStuffing(const std::string& inputStream){
    // Consider the following flag
    // FLag is : 01111110
    int counter = 0;
    std::string stuffedBitStream = "01111110";
    for (char i : inputStream){
        if((i-48) == 1){
            counter++;
            stuffedBitStream +="1";
            if (counter == 5){
                counter = 0;
                stuffedBitStream += "0";
            }
        }
        else{
            stuffedBitStream += "0";
            counter = 0;
        }
    }
    return stuffedBitStream + "01111110";
}

// Error Detection and Correction For the given stream
// Construct Hamming Message
std::string Node::hammingGenerator(const std::string& inputStream){
    // Calculate extra bits (r)
    int m = inputStream.size();
    int r = 0;
    while ((pow(2,r)-r) < (m+1)) r++;

    std::string hammingString= inputStream;
    for(int i =0 ; i< r ;i++){
        hammingString.insert((int)pow(2,i)-1,"?");
    }

    for (int i=0;i < r ;i++){
        std::vector<char>parityArr;
        for (int k= (int)pow(2,i)-1; k< hammingString.size();k+= pow(2,i+1)){
            for (int j = 0 ; j < (int)pow(2,i) ; j++) {
                if(j+k >= hammingString.size()) break;
                parityArr.push_back(hammingString[k+j]);
            }
        }
        std::bitset<1> parityBit (parityArr[1]);
        for(int l =2 ;l< parityArr.size();l++){
            std::bitset<1> temp (parityArr[l]);
            parityBit =parityBit ^ temp;
        }
        hammingString[(int)pow(2,i)-1] = parityBit == 1 ? '1' : '0';
    }
    return hammingString;
}
bool IsPowerOfTwo(unsigned int x){
    return (x != 0) && ((x & (x - 1)) == 0);
}

// Error Detection and correction in Hamming
std::string Node::errorDetectionCorrectionHamming(std::string message){
    std::vector<char>messageParityBit;
    std::string possibleSentMessage;
    for (int i = 0; i < message.size(); ++i) {
        if (IsPowerOfTwo(i+1))
            messageParityBit.push_back(message[i]);
        else
            possibleSentMessage+=message[i];
    }
    std::string standardHammingMessage = hammingGenerator(possibleSentMessage);
    int wrongIndex = 0 ;
    int counter = 0 ;
    for (int i=0; i< standardHammingMessage.size();i++){
        if (IsPowerOfTwo(i+1)){
            if(standardHammingMessage[i] == messageParityBit[counter]){
                counter++;
                continue;
            }
            else {
                counter++;
                wrongIndex+=(i+1);
            }
        }
    }

    if (wrongIndex != 0){
        standardHammingMessage[wrongIndex-1] = (standardHammingMessage[wrongIndex-1] == '1')?'0':'1';
    }

    std::string output;
    for (int i = 0; i < standardHammingMessage.size(); ++i) {
        if (!IsPowerOfTwo(i+1))
            output += standardHammingMessage[i];
    }

    return output;
}
std::string Node::bitDeStuffing(const std::string& inputStream){
    // Consider the following flag
    // FLag is : 01111110
    int counter = 0;
    std::string deStuffedBitStream;
    for (int i=8;i<inputStream.size() - 8; i++){
        if((inputStream[i]-48) == 1){
            counter++;
            deStuffedBitStream +="1";
            if (counter == 5){
                counter = 0;
                i++;
                continue;
            }
        }
        else{
            deStuffedBitStream += "0";
            counter = 0;
        }
    }
    return deStuffedBitStream;
}
std::string Node::string2bits(std::string text){
    std::string transformed;
    char output[9];
    for (int i = 0;i < text.size(); i++) {
        itoa(text[i], output, 2);
        for (int i = 7; i >= 0; --i) {
            if(output[i] != '1' && output[i] != '0')
                transformed += '0';
        }
        transformed += output;
    }
    return transformed;
}

std::string Node::binary2string(std::string binary){
    std::stringstream sstream(binary);
    std::string output;
    while(sstream.good()) {
        std::bitset<8> bits;
        sstream >> bits;
        char c = char(bits.to_ulong());
        output += c;
   }
    return output;
}


