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

import eu.pursuit.client.BlackAdderClient;
import eu.pursuit.client.BlackadderWrapper;
import eu.pursuit.core.ByteIdentifier;
import eu.pursuit.core.Event;
import eu.pursuit.core.Event.EventType;
import eu.pursuit.core.ItemName;
import eu.pursuit.core.ScopeID;
import eu.pursuit.core.Strategy;

public class Subscriber {

	public static void main(String[] args) {

		String sharedObjPath = "/tmp/eu_pursuit_client_BlackadderWrapper.so";

		BlackadderWrapper.configureObjectFile(sharedObjPath);

		BlackAdderClient client = BlackAdderClient.getInstance();
		ScopeID rootScope = new ScopeID();
		rootScope.addSegment(new ByteIdentifier((byte) 0, 8));

		ByteIdentifier rid = new ByteIdentifier((byte) 1, 8);
		ItemName name = new ItemName(rootScope, rid);
	
		client.subscribeScope(rootScope, null, Strategy.NODE, null);
		client.subscribeItem(name, Strategy.NODE, null);

		System.out.println("Subscribing");
		System.out.println("waiting");
		long start = System.currentTimeMillis();
		for (int i = 0; i < 1; i++) {
			Event event = client.getNextEvent();
			if (event.getType() == EventType.PUBLISHED_DATA) {
				System.out.printf("Got notification %d and %d bytes\n", i, event.getDataLength());
				printbytes(event.getDataCopy());

				// make sure to free buffers
				event.freeNativeBuffer();
			}
		}

		long duration = System.currentTimeMillis() - start;
		System.out.printf("Duration %d\n", duration);
		client.disconnect();

	}

	private static void printbytes(byte[] data) {
		for (byte b : data) {
			System.out.print(b + " ");
		}
		System.out.println();

	}

}
