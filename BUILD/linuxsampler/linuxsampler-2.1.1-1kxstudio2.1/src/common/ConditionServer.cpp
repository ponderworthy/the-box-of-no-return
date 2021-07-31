/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2008 Christian Schoenebeck                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include "global_private.h"

#include "ConditionServer.h"

namespace LinuxSampler {

ConditionServer::ConditionServer() : Reader(Condition) {
    Condition.GetConfigForUpdate() = false;
    Condition.SwitchConfig() = false;
    bOldCondition = false;
}

bool* ConditionServer::Push(bool bCondition, long TimeoutSeconds, long TimeoutNanoSeconds) {
    dmsg(3,("conditionserver:Push() requesting change to %d\n", bCondition));
    PushMutex.Lock();
    bool& c = Condition.GetConfigForUpdate();
    bOldCondition = c;
    c = bCondition;
    Condition.SwitchConfig() = bCondition;
    return &bOldCondition;
}

bool* ConditionServer::PushAndUnlock(bool bCondition, long TimeoutSeconds, long TimeoutNanoSeconds, bool bAlreadyLocked) {
    dmsg(3,("conditionserver:PushAndUnlock() requesting change to %d\n", bCondition));
    if (!bAlreadyLocked) PushMutex.Lock();
    bool& c = Condition.GetConfigForUpdate();
    bOldCondition = c;
    c = bCondition;
    Condition.SwitchConfig() = bCondition;
    Unlock();
    return &bOldCondition;
}

void ConditionServer::Unlock() {
    PushMutex.Unlock();
}

bool ConditionServer::GetUnsafe() {
    return Condition.GetConfigForUpdate();
}

} // namespace LinuxSampler
