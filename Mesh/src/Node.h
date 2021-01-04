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

#ifndef __MESH_NODE_H_
#define __MESH_NODE_H_

#include <omnetpp.h>
#include <vector>
#define MaxSEQ 7
#define timeOut 10
using namespace omnetpp;

/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
    // Noisy Channel Members
    double modification_probability;
    int modificationFrameLowerBit;
    int modificationFrameUpperBit;
    double loss_probability;
    double duplication_probability;
    double delay_probability;
    double delay_lambda;
    cMessage* modifyMsg(cMessage* msg);
    bool NoisySend(cMessage*& msg);

    // GoBackN Members
    int nextFrameToSend, AckExpected, frameExpected, nBuffered;
    cMessage* buffer [MaxSEQ];
    std::vector<int> Ack;
    void send_Data();
    void start_Timer();
    void goBackN(cMessage* msg, bool selfMsg);
    bool between(int a, int b, int c);
    bool repeat(int ackNo);
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

#endif
