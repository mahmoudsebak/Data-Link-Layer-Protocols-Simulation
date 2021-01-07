//
// This program is free software: you can redistribute it and/or modify
#include "Node.h"

Define_Module(Node);

void Node::initialize()
{
    dest = 0;
    double interval2 = exponential(1 / par("lambda").doubleValue());
    if(getIndex() == 0)
        scheduleAt(simTime() + interval2, new FramedMessage_Base("Continue"));
    else if(getIndex() == 1)
        scheduleAt(simTime() + interval2, new FramedMessage_Base("Continue"));
    else
        return;
    for (int i=0; i< MaxSEQ+1; i++)
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

}

void Node::handleMessage(cMessage *msg)
{
//    if (getIndex() != 0)// && getIndex() != 1)
//                return;
    FramedMessage_Base* fmsg = check_and_cast<FramedMessage_Base*>(msg);
    if (fmsg->isSelfMessage()) { //Host wants to send

//        int rand;
//        do { //Avoid sending to yourself
//            rand = uniform(0, gateSize("outs"));
//        } while(rand == getIndex());
//
//        //Calculate appropriate gate number
//        dest = rand;
//        if (rand > getIndex())
//            dest--;


          //  dest = 0;
        //else
          //  return;

        std::stringstream ss;
//        ss << rand;
        ss << dest;
        EV << "Sending to "<< ss.str() <<" from source " << getIndex() << "\n";
//        delete fmsg;
//        fmsg = new FramedMessage_Base("Hello");
//        fmsg->setPayload(buffer[nBuffered].c_str());
//        fmsg->setSeq_num(nextFrameToSend);
//        fmsg->setAck_num(AckExpected);

        /////////////////////////////////////////////////////////////////
        // To Be Changed According to Main Logic
        // timeOut
        //bool repeat = false;
        if(strcmp(msg->getName(), "Continue") != 0)
        {
            //bool repeat = repeat1(atoi(msg->getName()));
            //if (repeat)
                goBackN(fmsg, 2);
            //else
              //  goBackN(fmsg, 1);
            //delete(msg);
        }
        else
            goBackN(fmsg, 1);
        ////////////////////////////////////////////////////////////////
        //double interval = exponential(1 / par("lambda").doubleValue());
        //EV << ". Scheduled a new packet after " << interval << "s";
    }
    else {
        if(strcmp(msg->getName(), "Continue") != 0)
        {
            goBackN(fmsg, 0);
            if (atoi(fmsg->getName()) == getIndex())
                bubble("Message received");
            else
                bubble("Wrong destination");
            delete fmsg;
        }
    }
    //delete fmsg;
}
void Node::start_Timer()
{
    if(timers[nextFrameToSend] != nullptr)
    {
        EV<<"Timer "<<nextFrameToSend <<" is already created"<<std::endl;
        return;
    }
    std::stringstream ss;
    ss<<nextFrameToSend;
    FramedMessage_Base* timer =  new FramedMessage_Base(ss.str().c_str());
    scheduleAt(simTime() + timeOut, timer);
    timers[nextFrameToSend] = timer;
    EV<<"CREATED Timer "<<nextFrameToSend<<std::endl;
}
void Node::send_Data(FramedMessage_Base* fmsg)
{
    //delete(fmsg);
    fmsg = new FramedMessage_Base("Hello");
    fmsg->setPayload(buffer[nextFrameToSend].c_str());
    fmsg->setSeq_num(nextFrameToSend);
    fmsg->setAck_num((frameExpected + MaxSEQ) % (MaxSEQ + 1));

    EV << "Payload " << fmsg->getPayload() << std::endl;
    EV << "Sequence number " << fmsg->getSeq_num() << std::endl;
    EV << "Ack number " << fmsg->getAck_num() << std::endl;

//    bool duplicated = false;
//    send(fmsg, "outs", dest);
//    sendDelayed(fmsg, 0.1, "outs", dest);
    bool duplicated = NoisySend(fmsg);

    start_Timer();

    nextFrameToSend += (duplicated)? 0 : 1;
    nextFrameToSend %= (MaxSEQ + 1);

//    fileIterator += (duplicated)? 0 : 1;
}
bool Node::between(int a, int b, int c)
{
    return (((a<=b) && (b<c)) || ((c<a) && (a<=b)) || ((b<c) &&(c<a)))? true : false;
//    return (((a<=b) && (b<c)) || ((c>a) && (a>=b)) || ((b<c) &&(c<a)))? true : false;

}
bool Node::repeat1(int ackNo)
{
    for(int i=0; i<Ack.size(); i++)
    {
        if (Ack[i] == ackNo)
        {
            Ack.erase(Ack.begin() + i);
            EV<<"Deleted Successfully ";
            EV<<ackNo;
            EV<<std::endl;
            return false;
        }
    }
    return true;
}
void Node::goBackN(FramedMessage_Base* msg, int whichCase)
{
    //double interval1 = exponential(1 / par("lambda").doubleValue());
    double interval1 = 0.1;
    // Frame arrival
    //if (!msg->isSelfMessage() && !repeat)
    switch (whichCase)
    {
    case 0:
        EV << "Frame arrival " << std::endl;
        EV << "Payload " << msg->getPayload() << std::endl;
        EV << "Sequence number received " << msg->getSeq_num() << " and expected " << frameExpected << std::endl;
        EV << "Ack number " << msg->getAck_num() << std::endl;

        if(frameExpected == msg->getSeq_num())
        {
            frameExpected++;
            frameExpected %= (MaxSEQ + 1);
        }
//        else
//            return;

        EV<<"ACKEXPECTED "<<AckExpected<<std::endl;
        EV<<"GETACKNUM "<<msg->getAck_num()<<std::endl;
        EV<<"NEXTTOSEND "<<nextFrameToSend<<std::endl;

        while(between(AckExpected, msg->getAck_num(), nextFrameToSend))
        {
            nBuffered--;
            //if(atoi(timers[i]->getName()) == AckExpected)
            if(timers[AckExpected] != nullptr)
            {
                EV<<"CANCELANDDELETE Timer "<<AckExpected<<std::endl;
                cancelAndDelete(timers[AckExpected]);
                timers[AckExpected] = nullptr;
            }
            AckExpected++;
            AckExpected %= (MaxSEQ + 1);
            //int temp = -1;
//            for(int i=0; i<timers.size(); i++)
//                if (timers[i]->isSelfMessage() && atoi(timers[i]->getName()) == AckExpected)
//                {
//                    EV<<"Deleted Successfully ";
//                    EV<<atoi(timers[i]->getName());
//                    EV<<std::endl;
//                   //temp = atoi(timers[i]->getName());
//                    //temp = i;
//                    cancelEvent(timers[i]);
//                    //break;
//                }
//            if(temp != -1)
//            {
//                timers.erase(timers.begin()+temp);
//                cancelEvent(timers[temp]);
//            }

//            Ack.push_back(AckExpected);
//            EV<<"Accepted Ack No";
//            EV<<AckExpected;
//            EV<<std::endl;

        }
        //delete(msg);
        break;
    // TODO: Read Next frame from file
    //else if(send && nBuffered < MaxSEQ)
    case 1:
        EV<<nBuffered<<std::endl;
        if (nBuffered < MaxSEQ){
            EV << "Send next frame " << std::endl;
            buffer[nextFrameToSend] = "Hello";
            nBuffered++;
            delete(msg);
            send_Data(msg);
            scheduleAt(simTime() + interval1, new FramedMessage_Base("Continue"));
        }
        else
            //goBackN(msg, 0);
            scheduleAt(simTime() + interval1, new FramedMessage_Base("Continue"));
        break;
    // Re-send if there is timeout
    //else if(repeat)
    case 2:
        delete(msg);
        EV << "Repeat " << std::endl;
//        int x  = nBuffered;
//        nBuffered -= (nextFrameToSend - AckExpected);
//        nextFrameToSend = AckExpected;
//        for (int i = nBuffered; i < x; i++)
//        {
//            send_Data(msg);
//        }
        nextFrameToSend = AckExpected;
        for(int i = 1; i <= nBuffered; i++)
        {
            send_Data(msg);
        }
//        delete(msg);
    }
}
std::string Node::modifyMsg(std::string msg)
{
    modificationFrameLowerBit = std::max(0, modificationFrameLowerBit);
    modificationFrameUpperBit = std::min(modificationFrameUpperBit, int(msg.size()));
    int bit = int(uniform(modificationFrameLowerBit, modificationFrameUpperBit));
    std::bitset<8> msgInBits(msg);
    msgInBits[bit] = ~msgInBits[bit];
    EV<<"Modified";
    return msgInBits.to_string();
}
bool Node::NoisySend(FramedMessage_Base*& msg)
{
    //if(uniform(0,1) < modification_probability)
      //  msg->setPayload(modifyMsg(msg->getPayload()).c_str());
    if(uniform(0,1) < loss_probability)
    {
        EV<<"Dropped"<<std::endl;
        return true;
    }
//    else if(uniform(0,1) < delay_probability)
//    {
//        EV<<"Delayed";
//        double delay = exponential(1 / delay_lambda);
////        send(msg, "outs", dest);
//        sendDelayed(msg, delay, "outs", dest);
//    }
//    else if(uniform(0,1) < duplication_probability)
//    {
//        EV<<"Duplicated";
//        send(msg, "outs", dest);
//        return true;
//    }
    else
    {
        send(msg, "outs", dest);
    }
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
    for (int i=0;i<inputStream.size();i++){
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
std::string string2bits(std::string text){
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



