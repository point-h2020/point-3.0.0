/*
 * Copyright (C) 2011  Christos Tsilopoulos, Mobile Multimedia Laboratory, 
 * Athens University of Economics and Business 
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * See LICENSE and COPYING for more details.
 */

package eu.point;

import java.nio.ByteBuffer;
import java.util.Arrays;

import eu.pursuit.client.BlackAdderClient;
import eu.pursuit.client.BlackadderWrapper;
import eu.pursuit.core.ByteIdentifier;
import eu.pursuit.core.Event;
import eu.pursuit.core.Event.EventType;
import eu.pursuit.core.ItemName;
import eu.pursuit.core.ScopeID;
import eu.pursuit.core.Strategy;

public class Publisher {

	public static void main(String[] args) {

		String sharedObjPath = "/tmp/eu_pursuit_client_BlackadderWrapper.so";

		BlackadderWrapper.configureObjectFile(sharedObjPath);

		BlackAdderClient client = BlackAdderClient.getInstance();
		ScopeID rootScope = new ScopeID();
		ByteIdentifier rootScopeId = new ByteIdentifier((byte) 0, 8);
		rootScope.addSegment(rootScopeId);

		client.publishRootScope(rootScopeId, Strategy.NODE, null);

		ByteIdentifier rid = new ByteIdentifier((byte) 1, 8);
		ItemName name = new ItemName(rootScope, rid);

		System.out.println("Publishing");
		client.publishItem(name, Strategy.NODE, null);
		System.out.println("done");
		System.out.println("waiting for events");
		Event event = client.getNextEvent();
		System.out.println("got a notification");

		byte[] payload = new byte[1000];
		Arrays.fill(payload, (byte) 5);
		ByteBuffer buffer = ByteBuffer.allocateDirect(payload.length);
		buffer.put(payload);
		buffer.flip();
		if (event.getType() == EventType.START_PUBLISH) {
			System.out.println("got event");
			for (int i = 0; i < 1; i++) {
				System.out.println("apcket " + i);
				client.publishData(event.getId(), payload, Strategy.NODE, null);
			}
		}
		client.disconnect();
	}

}
