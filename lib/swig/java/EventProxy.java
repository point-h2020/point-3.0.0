/*-
 * Copyright (C) 2012  Oy L M Ericsson Ab, NomadicLab
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * See LICENSE and COPYING for more details.
 */

package blackadder_java;

public class EventProxy extends Event {
    public EventProxy(long cPtr) {
        super(cPtr, true);
    }
}
