/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.bootstrapping.impl.utils;

import org.opendaylight.yang.gen.v1.urn.ietf.params.xml.ns.yang.ietf.yang.types.rev130715.MacAddress;

import java.util.Arrays;

/**
 * The class with utility functions for packet parsing
 *
 * @author George Petropoulos
 * @version cycle-2
 */
public abstract class PacketParsingUtils {

    /**
     * size of MAC address in octets (6*8 = 48 bits)
     */
    private static final int MAC_ADDRESS_SIZE = 6;

    /**
     * start position of destination MAC address in array
     */
    private static final int DST_MAC_START_POSITION = 0;

    /**
     * end position of destination MAC address in array
     */
    private static final int DST_MAC_END_POSITION = 6;

    /**
     * start position of source MAC address in array
     */
    private static final int SRC_MAC_START_POSITION = 6;

    /**
     * end position of source MAC address in array
     */
    private static final int SRC_MAC_END_POSITION = 12;

    /**
     * start position of ethernet type in array
     */
    private static final int ETHER_TYPE_START_POSITION = 12;

    /**
     * end position of ethernet type in array
     */
    private static final int ETHER_TYPE_END_POSITION = 14;

    /**
     * start position of IPv6 header in array
     */
    private static final int IPV6_HEADER_START_POSITION = 14;

    /**
     * end position of IPv6 header in array
     */
    private static final int IPV6_HEADER_END_POSITION = 22;


    /**
     * start position of IPv6 src address in array
     */
    private static final int SRC_IPV6_START_POSITION = 22;

    /**
     * end position of IPv6 src address in array
     */
    private static final int SRC_IPV6_END_POSITION = 22 + 16;

    /**
     * start position of IPv6 dst address in array
     */
    private static final int DST_IPV6_START_POSITION = 22 + 16;

    /**
     * end position of IPv6 dst address in array
     */
    private static final int DST_IPV6_END_POSITION = 22 + 16 + 16;

    /**
     * start position of ICN scopes number in array
     */
    private static final int ICN_SCOPES_NUMBER_POSITION = 22 + 16 + 16 + 1;

    /**
     * start position of ICN payload in array
     */
    private static final int ICN_START_POSITION = 22 + 16 + 16 + 2;

    /**
     * end position of ICN first scope in array
     */
    private static final int ICN_SCOPE_END_POSITION = 22 + 16 + 16 + 2 + 8;



    private PacketParsingUtils() {

    }

    public static byte[] extractDstMac(final byte[] payload) {
        return Arrays.copyOfRange(payload, DST_MAC_START_POSITION, DST_MAC_END_POSITION);
    }

    public static byte[] extractSrcMac(final byte[] payload) {
        return Arrays.copyOfRange(payload, SRC_MAC_START_POSITION, SRC_MAC_END_POSITION);
    }

    public static byte[] extractEtherType(final byte[] payload) {
        return Arrays.copyOfRange(payload, ETHER_TYPE_START_POSITION, ETHER_TYPE_END_POSITION);
    }

    public static byte[] extractIpv6Header(final byte[] payload) {
        return Arrays.copyOfRange(payload, IPV6_HEADER_START_POSITION, IPV6_HEADER_END_POSITION);
    }

    public static byte[] extractIpv6Source(final byte[] payload) {
        return Arrays.copyOfRange(payload, SRC_IPV6_START_POSITION, SRC_IPV6_END_POSITION);
    }

    public static byte[] extractIpv6Destination(final byte[] payload) {
        return Arrays.copyOfRange(payload, DST_IPV6_START_POSITION, DST_IPV6_END_POSITION);
    }

    public static byte[] extractIcnScopesNumber(final byte[] payload) {
        return Arrays.copyOfRange(payload, ICN_SCOPES_NUMBER_POSITION, ICN_START_POSITION);
    }

    public static byte[] extractFirstScope(final byte[] payload) {
        return Arrays.copyOfRange(payload, ICN_START_POSITION, ICN_SCOPE_END_POSITION);
    }

    public static byte[] extractIcnPayload(final byte[] payload) {
        return Arrays.copyOfRange(payload, ICN_START_POSITION, payload.length);
    }

    public static MacAddress rawMacToMac(final byte[] rawMac) {
        MacAddress mac = null;
        if (rawMac != null && rawMac.length == MAC_ADDRESS_SIZE) {
            StringBuilder sb = new StringBuilder();
            for (byte octet : rawMac) {
                sb.append(String.format(":%02X", octet));
            }
            mac = new MacAddress(sb.substring(1));
        }
        return mac;
    }

    public static String rawMacToString(byte[] rawMac) {
        if (rawMac != null && rawMac.length == 6) {
            StringBuffer sb = new StringBuffer();
            for (byte octet : rawMac) {
                sb.append(String.format(":%02X", octet));
            }
            return sb.substring(1);
        }
        return null;
    }

    public static byte[] stringMacToRawMac(String address) {
        String[] elements = address.split(":");
        if (elements.length != MAC_ADDRESS_SIZE) {
            throw new IllegalArgumentException(
                    "Specified MAC Address must contain 12 hex digits" +
                            " separated pairwise by :'s.");
        }

        byte[] addressInBytes = new byte[MAC_ADDRESS_SIZE];
        for (int i = 0; i < MAC_ADDRESS_SIZE; i++) {
            String element = elements[i];
            addressInBytes[i] = (byte) Integer.parseInt(element, 16);
        }
        return addressInBytes;
    }

    public static String rawBytesToString(byte[] payload) {
        StringBuffer sb = new StringBuffer();
        for (byte octet : payload) {
            sb.append(String.format("%02X", octet));
        }
        return sb.substring(1);
    }
}
