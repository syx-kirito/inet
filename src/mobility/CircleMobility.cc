//
// Copyright (C) 2005 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "CircleMobility.h"
#include "FWMath.h"


Define_Module(CircleMobility);


void CircleMobility::initPos()
{
    cx = par("cx");
    cy = par("cy");
    r = par("r");
    ASSERT(r > 0);
    angle = par("startAngle").doubleValue() / 180.0 * PI;
    updateInterval = par("updateInterval");
    double speed = par("speed");
    omega = speed / r;

    // calculate initial position
    setPos();
}

void CircleMobility::setPos()
{
    pos.x = cx + r * cos(angle);
    pos.y = cy + r * sin(angle);
    // do something if we reach the wall
    Coord dummyCoord; double dummyAngle;
    handleIfOutside(REFLECT, dummyCoord, dummyCoord, dummyAngle);
}

void CircleMobility::initialize(int stage)
{
    BasicMobility::initialize(stage);

    EV << "initializing CircleMobility stage " << stage << endl;

    if (stage == 1)
    {
        // if the initial speed is 0, the node is stationary
        stationary = (omega == 0);

        // host moves the first time after some random delay to avoid synchronized movements
        if (!stationary)
            scheduleAt(simTime() + uniform(0, updateInterval), new cMessage("move"));
    }
}


void CircleMobility::handleSelfMsg(cMessage * msg)
{
    move();
    positionUpdated();
    scheduleAt(simTime() + updateInterval, msg);
}

void CircleMobility::move()
{
    angle += omega * updateInterval;
    setPos();

    EV << " xpos= " << pos.x << " ypos=" << pos.y << endl;
}
