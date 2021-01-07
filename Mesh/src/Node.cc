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
        goBackN(fmsg, 0);
        if (atoi(fmsg->getName()) == getIndex())
            bubble("Message received");
        else
            bubble("Wrong destination");
        delete fmsg;
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
void Node::send_Data()
{
    FramedMessage_Base* fmsg = new FramedMessage_Base("Hello");
    fmsg->setPayload(buffer[nextFrameToSend].c_str());
    fmsg->setSeq_num(nextFrameToSend);
    fmsg->setAck_num((frameExpected + MaxSEQ) % (MaxSEQ + 1));

    EV << "Payload " << fmsg->getPayload() << std::endl;
    EV << "Sequence number " << fmsg->getSeq_num() << std::endl;
    EV << "Ack number " << fmsg->getAck_num() << std::endl;

//    bool duplicated = false;
//    send(fmsg, "outs", dest);
    bool duplicated = NoisySend(fmsg);
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
        EV << "Frame arrival " << std::endl;
        EV << "Payload " << msg->getPayload() << std::endl;
        EV << "Sequence number received " << msg->getSeq_num() << " and expected " << frameExpected << std::endl;
        EV << "Ack number " << msg->getAck_num() << std::endl;

        if(frameExpected == msg->getSeq_num())
        {
            frameExpected++;
            frameExpected %= (MaxSEQ + 1);
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
    // TODO: Read Next frame from file
    case 1:
        EV<<nBuffered<<std::endl;
        if (nBuffered < MaxSEQ){
            EV << "Send next frame " << std::endl;
            buffer[nextFrameToSend] = "Hello"; // Here
            nBuffered++;
            delete(msg);
            send_Data();
            scheduleAt(simTime() + interval, new FramedMessage_Base("Continue"));
        }
        else
            scheduleAt(simTime() + interval, new FramedMessage_Base("Continue"));
        break;
    // Re-send if there is timeout
    case 2:
//        delete(msg);
        EV << "Repeat " << std::endl;
        nextFrameToSend = AckExpected;
        for(int i = 1; i <= nBuffered; i++)
        {
            send_Data();
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
bool Node::NoisySend(FramedMessage_Base*& msg)
{
    if(uniform(0,1) < modification_probability)
        msg->setPayload(modifyMsg(msg->getPayload()).c_str());
    if(uniform(0,1) < loss_probability)
    {
        EV<<"Dropped"<<std::endl;
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
    }
    else
    {
        send(msg, "outs", dest);
    }
    return false;
}


