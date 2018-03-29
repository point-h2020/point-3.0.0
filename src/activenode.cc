/*
 * Copyright (C) 2015-2018  Mays AL-Naday
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See LICENSE and COPYING for more details.
 */

#include "activenode.hh"

CLICK_DECLS

ActiveNode::ActiveNode(String _nodeID) {
    nodeID = _nodeID;
}

ActiveNode::~ActiveNode() {
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(ActiveNode)
