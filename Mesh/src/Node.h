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

#include "FramedMessage_m.h"
#include <omnetpp.h>
#include <vector>
#include <bitset>
#include <queue>
#include <fstream>
#define MaxSEQ 7
#define timeOut 0.2
#define interval 0.1
using namespace omnetpp;

class Node : public cSimpleModule
{
    // File Handle
    std::fstream MyReadFile;
    std::fstream MyoutputFile;
    // Noisy Channel Members
    double modification_probability;
    int modificationFrameLowerBit;
    int modificationFrameUpperBit;
    double loss_probability;
    double duplication_probability;
    double delay_probability;
    double delay_lambda;
    std::string modifyMsg(std::string msg);
    bool NoisySend(FramedMessage_Base*& msg, bool useful);
    // GoBackN Members
    int nextFrameToSend, AckExpected, frameExpected, nBuffered, dest, id;
    bool selfFinished, pairFinished;
    std::string buffer [MaxSEQ + 1];
    FramedMessage_Base* timers[MaxSEQ + 1];

    void send_Data(bool useful, bool finish);
    void start_Timer();
    void goBackN(FramedMessage_Base* msg, int whichCase);
    bool between(int a, int b, int c);
    std::string string2bits(std::string text);
    std::string binary2string(std::string binary);

    // Statistics Members
    int totalGenerated, totalDropped, totalRetransmitted;
    int usefulTransmittedSize, totaltransmittedSize;
  protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
    std::string bitStuffing(const std::string& inputStream);
    std::string bitDeStuffing(const std::string& inputStream);
    std::string hammingGenerator(const std::string& inputStream);
    std::string errorDetectionCorrectionHamming(std::string message);
};

#endif
