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
    scheduleAt(simTime() + interval, new cMessage(""));

    // Initialize Noisy Channel Members
    modification_probability = par("modification_probability").doubleValue();
    modificationFrameLowerBit = par("modificationFrameLowerBit").intValue();
    modificationFrameUpperBit = par("modificationFrameUpperBit").intValue();
    loss_probability = par("loss_probability").doubleValue();
    duplication_probability = par("duplication_probability").doubleValue();
    delay_probability = par("delay_probability").doubleValue();
    delay_lambda = par("delay_lambda").doubleValue();
}

void Node::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) { //Host wants to send

        int rand, dest;
        do { //Avoid sending to yourself
            rand = uniform(0, gateSize("outs"));
        } while(rand == getIndex());

        //Calculate appropriate gate number
        dest = rand;
        if (rand > getIndex())
            dest--;

        std::stringstream ss;
        ss << rand;
        EV << "Sending "<< ss.str() <<" from source " << getIndex() << "\n";
        delete msg;
        msg = new cMessage(ss.str().c_str());
        send(msg, "outs", dest);

        /////////////////////////////////////////////////////////////////
        // To Be Changed According to Main Logic
        // timeOut
        int ackNo = atoi(msg->getName());
        if(repeat(ackNo))
            goBackN(msg, true);
        ////////////////////////////////////////////////////////////////
        double interval = exponential(1 / par("lambda").doubleValue());
        EV << ". Scheduled a new packet after " << interval << "s";
        scheduleAt(simTime() + interval, new cMessage(""));
    }
    else {
        //atoi functions converts a string to int
        //Check if this is the proper destination
        if (atoi(msg->getName()) == getIndex())
            bubble("Message received");
        else
            bubble("Wrong destination");
        delete msg;
    }
}
void Node::start_Timer()
{
    std::stringstream ss;
    ss<<nextFrameToSend;
    scheduleAt(simTime() + timeOut, new cMessage(ss.str().c_str()));
}
void Node::send_Data()
{
    //prepare msg
    bool duplicated = NoisySend(buffer[nBuffered]);
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
void Node::goBackN(cMessage* msg, bool repeat)
{
    if(repeat)
    {
        nextFrameToSend = AckExpected;
        for (int i = 1; i < nBuffered; i++)
        {
            send_Data();
        }
    }
    else if (msg != nullptr)
    {
        // if not expected return
        int rAck;
        frameExpected ++;
        while(between(AckExpected, rAck, nextFrameToSend))
        {
            nBuffered --;
            Ack.push_back(AckExpected++);
        }
    }
    if(nBuffered < MaxSEQ)
    {
        cMessage* toSend; // read next frame
        buffer[nBuffered++] = toSend;
        send_Data();
    }
}

cMessage* Node::modifyMsg(cMessage* msg)
{
    // To be Adjusted according to bitset

    std::string msgAsString = msg->str();
    modificationFrameLowerBit = std::max(0, modificationFrameLowerBit);
    modificationFrameUpperBit = std::min(modificationFrameUpperBit, int(msgAsString.size()));
    int bit = int(uniform(modificationFrameLowerBit, modificationFrameUpperBit));
    msgAsString[bit] = ~msgAsString[bit];
    delete(msg);
    msg = new cMessage(msgAsString.c_str());
    EV<<"Modified";
    return msg;
}
bool Node::NoisySend(cMessage*& msg)
{
    if(uniform(0,1) < modification_probability)
        msg = modifyMsg(msg);
    if(uniform(0,1) < loss_probability)
    {
        EV<<"Dropped";
    }
    else if(uniform(0,1) < delay_probability)
    {
        EV<<"Delayed";
        double delay = exponential(1 / delay_lambda);
        sendDelayed(msg, delay, "out");
    }
    else if(uniform(0,1) < duplication_probability)
    {
        EV<<"Duplicated";
        send(msg, "out");
        return true;
    }
    else
    {
        send(msg, "out");
    }
    return false;
}


