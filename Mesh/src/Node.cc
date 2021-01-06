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
            //atoi functions converts a string to int
            //Check if this is the proper destination
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
    std::stringstream ss;
    ss<<nextFrameToSend;
    FramedMessage_Base* timer =  new FramedMessage_Base(ss.str().c_str());
    scheduleAt(simTime() + timeOut, timer);
    //timers.push_back(timer);
    timers[nextFrameToSend % (MaxSEQ+1)] = timer;
}
void Node::send_Data(FramedMessage_Base* fmsg)
{
    //delete(fmsg);
    fmsg = new FramedMessage_Base("Hello");
    fmsg->setPayload(buffer[nBuffered].c_str());
    fmsg->setSeq_num(nextFrameToSend);
    fmsg->setAck_num((frameExpected + MaxSEQ) % (MaxSEQ + 1));
    EV << "Payload " << fmsg->getPayload();
    EV << std::endl;
    EV << "Sequence number " << fmsg->getSeq_num();
    EV << std::endl;
    EV << "Ack number " << fmsg->getAck_num();
    EV << std::endl;
    send(fmsg, "outs", dest);
    //sendDelayed(fmsg, 0.1, "outs", dest);
    bool duplicated = false;
    //bool duplicated = NoisySend(fmsg);
    nBuffered ++;
    start_Timer();
    nextFrameToSend += (duplicated)? 0 : 1;
    // fileIterator += (duplicated)? 0 : 1;
}
bool Node::between(int a, int b, int c)
{
    return (((a<=b) && (b<c)) || ((c<a) && (a<=b)) || ((b<c) &&(c<a)))? true : false;
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
        EV << "Payload " << msg->getPayload();
        EV << std::endl;
        EV << "Sequence number " << msg->getSeq_num();
        EV << std::endl;
        EV << "Ack number " << msg->getAck_num();
        EV << std::endl;
        if(frameExpected == msg->getSeq_num())
            frameExpected++;
        EV<<"ACKEXPECTED"<<AckExpected<<std::endl;
        EV<<"GETACKNUM"<<msg->getAck_num()<<std::endl;
        EV<<"NEXTTOSEND"<<nextFrameToSend<<std::endl;
        while(between(AckExpected, msg->getAck_num(), nextFrameToSend))
        {
            nBuffered--;
            //if(atoi(timers[i]->getName()) == AckExpected)
            if(timers[AckExpected] != nullptr)
            {
                EV<<"CancelANDDELETE ";
                cancelAndDelete(timers[AckExpected]);
                timers[AckExpected] = nullptr;
            }
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
            //Ack.push_back(AckExpected);
            //EV<<"Accepted Ack No";
            //EV<<AckExpected;
            EV<<std::endl;
            AckExpected++;
            AckExpected %= (MaxSEQ);
        }
        //delete(msg);
        break;
    // TODO: Read Next frame from file
    //else if(send && nBuffered < MaxSEQ)
    case 1:
        EV<<nBuffered<<std::endl;
        if (nBuffered < MaxSEQ){
            EV << "Send next frame " << std::endl;
            buffer[nBuffered] = "Hello";
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
        nextFrameToSend = AckExpected;
        for (int i = 1; i <= nBuffered; i++)
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
        EV<<"Dropped";
    }
    else if(uniform(0,1) < delay_probability)
    {
        EV<<"Delayed";
        double delay = exponential(1 / delay_lambda);
        send(msg, "outs", dest);
        //sendDelayed(msg, delay, "outs", dest);
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


