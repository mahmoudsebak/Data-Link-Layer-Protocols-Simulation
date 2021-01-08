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

#include "Orchestrator.h"

Define_Module(Orchestrator);

void Orchestrator::initialize()
{
    N = par("N").intValue();
    nodes = new bool[N];
    for(int i = 0; i < N; i++) nodes[i] = true;

    scheduleAt( simTime() + 30, new cMessage("Generate new message"));

    // TODO - Generated method body
}

void Orchestrator::handleMessage(cMessage *msg)
{
    if( msg->getKind() == 2){
        nodes[atoi(msg->getName())] = true;

    } else {
        scheduleAt( simTime() + 30, new cMessage("Generate new message", 1));
        int rand = uniform(0, 2);
        if( rand == 1 ){
            int sender, reciver;
            rand = uniform(0, N);
            while(!nodes[rand])
                rand = uniform(0, N);
            nodes[rand] = false;
            sender = rand;
            rand = uniform(0, N);
            while(!nodes[rand])
                rand = uniform(0, N);
            nodes[rand] = false;

            EV <<"Orchestrator: start communication between sender: " << sender << ", reciver: " << rand << endl;
            reciver = (sender > reciver)? rand: rand - 1;
            int port = (sender > reciver)? sender - 1: sender;

            FramedMessage_Base* msg1 = new FramedMessage_Base("start");
            msg1->setPayload(std::to_string(reciver).c_str());
            send(msg1, "outs", sender);

            FramedMessage_Base* msg2 = new FramedMessage_Base("start");
            msg2->setPayload(std::to_string(port).c_str());
//            sendDelayed(msg2, 0.1, "outs", rand);
            send(msg2, "outs", rand);

        }
    }
}
