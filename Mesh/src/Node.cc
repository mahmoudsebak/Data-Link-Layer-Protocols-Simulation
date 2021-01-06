//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "Node.h"

Define_Module(Node);

void Node::initialize()
{
    double interval = exponential(1 / par("lambda").doubleValue());
    scheduleAt(simTime() + interval, new FramedMessage_Base(""));

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
    dest =0;
}

void Node::handleMessage(cMessage *msg)
{
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

        if (getIndex() != 0 && getIndex() != 1)
            return;
          //  dest = 0;
        //else
          //  return;

        std::stringstream ss;
//        ss << rand;
        ss << dest;
        EV << "Sending to "<< ss.str() <<" from source " << getIndex() << "\n";
        delete fmsg;
        fmsg = new FramedMessage_Base("Hello");
        fmsg->setPayload(buffer[nBuffered].c_str());
        fmsg->setSeq_num(nextFrameToSend);
        fmsg->setAck_num(AckExpected);

        /////////////////////////////////////////////////////////////////
        // To Be Changed According to Main Logic
        // timeOut
        bool repeat = true;
        if(strcmp(msg->getName(), "Continue") == 0)
        {
            repeat = false;
            delete(msg);
        }
        goBackN(fmsg, repeat);
        ////////////////////////////////////////////////////////////////
        double interval = exponential(1 / par("lambda").doubleValue());
        EV << ". Scheduled a new packet after " << interval << "s";
        scheduleAt(simTime() + interval, new FramedMessage_Base("Continue"));
    }
    else {
        goBackN(fmsg, false);
        //atoi functions converts a string to int
        //Check if this is the proper destination
        if (atoi(fmsg->getName()) == getIndex())
            bubble("Message received");
        else
            bubble("Wrong destination");
        delete fmsg;
    }
}
void Node::start_Timer()
{
    std::stringstream ss;
    ss<<nextFrameToSend;
    FramedMessage_Base* timer =  new FramedMessage_Base(ss.str().c_str());
    timers.push_back(timer);
    scheduleAt(simTime() + timeOut, timer);
}
void Node::send_Data()
{
    FramedMessage_Base* fmsg = new FramedMessage_Base("Hello");
    fmsg->setPayload(buffer[nBuffered].c_str());
    fmsg->setSeq_num(nextFrameToSend);
    fmsg->setAck_num((frameExpected + MaxSEQ) % (MaxSEQ + 1));
    EV << "Payload " << fmsg->getPayload();
    EV << std::endl;
    EV << "Sequence number " << fmsg->getSeq_num();
    EV << std::endl;
    EV << "Ack number " << fmsg->getAck_num();
    EV << std::endl;

    //send(fmsg, "outs", dest);
    //bool duplicated = false;
    bool duplicated = NoisySend(fmsg);
    start_Timer();
    nextFrameToSend += (duplicated)? 0 : 1;
    // fileIterator += (duplicated)? 0 : 1;
}
bool Node::between(int a, int b, int c)
{
    return (((a<=b) && (b<c)) || ((c<a) && (a<=b))||((b<c) &&(c<a)))? true : false;
}
bool Node::repeat(int ackNo)
{
    for(int i=0; i<Ack.size(); i++)
    {
        if (Ack[i] == ackNo)
        {
            Ack.erase(Ack.begin() + i);
            return false;
        }
    }
    return true;
}
void Node::goBackN(FramedMessage_Base* msg, bool repeat)
{
    // Frame arrival
    if (!msg->isSelfMessage() && !repeat)
    {
        EV << "Frame arrival " << std::endl;
        EV << "Payload " << msg->getPayload();
        EV << std::endl;
        EV << "Sequence number " << msg->getSeq_num();
        EV << std::endl;
        EV << "Ack number " << msg->getAck_num();
        EV << std::endl;
        if(frameExpected == msg->getSeq_num())
            frameExpected++;
        while(between(AckExpected, msg->getAck_num(), nextFrameToSend))
        {
            nBuffered--;
            for(int i=0; i<timers.size(); i++)
                if (atoi(timers[i]->getName()) == AckExpected)
                {
                    EV<<"Deleted Successfully ";
                    //EV<<atoi(timers[i]->getName().to_ttring();

                    cancelAndDelete(timers[i]);
                    //break;
                }
            AckExpected++;
        }
        //delete(msg);
    }
    // TODO: Read Next frame from file
    if(nBuffered < MaxSEQ)
    {
        EV << "Send next frame " << std::endl;
        buffer[nBuffered++] = "Hello";
        send_Data();
    }
    // Re-send if there is timeout
    else
    {
        delete(msg);
        EV << "Repeat " << std::endl;
        nextFrameToSend = AckExpected;
        for (int i = 1; i <= nBuffered; i++)
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
    std::bitset<8> msgInBits(msg);
    msgInBits[bit] = ~msgInBits[bit];
    EV<<"Modified";
    return msgInBits.to_string();
}
bool Node::NoisySend(FramedMessage_Base*& msg)
{
    if(uniform(0,1) < modification_probability)
        msg->setPayload(modifyMsg(msg->getPayload()).c_str());
    if(uniform(0,1) < loss_probability)
    {
        EV<<"Dropped";
    }
    else if(uniform(0,1) < delay_probability)
    {
        EV<<"Delayed";
        double delay = exponential(1 / delay_lambda);
        sendDelayed(msg, delay, "outs", dest);
    }
    else if(uniform(0,1) < duplication_probability)
    {
        EV<<"Duplicated";
        send(msg, "outs", dest);
        return true;
    }
    else
    {
        send(msg, "outs", dest);
    }
    return false;
}


