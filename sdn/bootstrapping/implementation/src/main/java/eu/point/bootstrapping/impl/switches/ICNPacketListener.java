/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.bootstrapping.impl.switches;

import eu.point.bootstrapping.impl.icn.IcnIdConfigurator;
import eu.point.bootstrapping.impl.utils.InventoryUtils;
import eu.point.bootstrapping.impl.utils.PacketParsingUtils;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.NodeConnectorId;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.NodeConnectorRef;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.NodeId;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.NodeRef;
import org.opendaylight.yang.gen.v1.urn.opendaylight.packet.service.rev130709.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.math.BigInteger;
import java.net.Inet6Address;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.util.Arrays;

/**
 * The listener for ICN packets.
 *
 * @author George Petropoulos
 * @version cycle-2
 */
public class ICNPacketListener implements PacketProcessingListener {

    private static final Logger LOG = LoggerFactory.getLogger(ICNPacketListener.class);
    private PacketProcessingService packetProcessingService;

    /**
     * The constructor class.
     */
    public ICNPacketListener(PacketProcessingService packetProcessingService) {
        LOG.info("Initializing ICN Packet Listener.");
        this.packetProcessingService = packetProcessingService;
    }


    /**
     * The method which acts upon reception of a packet.
     * It extracts the ethernet fields, ignores lldp packets,
     * and if it is an ICN packet, hence mac address is 00::00 and ipv6 type,
     * and discovery scope, then calculates TMFID and sends a discovery response.
     *
     * @param packetReceived The received packet to be inspected.
     */
    @Override
    public void onPacketReceived(PacketReceived packetReceived) {
        LOG.debug("Received packet {}.", packetReceived);

        NodeConnectorRef ingressNodeConnectorRef = packetReceived.getIngress();
        NodeRef ingressNodeRef = InventoryUtils.getNodeRef(ingressNodeConnectorRef);
        NodeConnectorId ingressNodeConnectorId = InventoryUtils.getNodeConnectorId(ingressNodeConnectorRef);
        NodeId ingressNodeId = InventoryUtils.getNodeId(ingressNodeConnectorRef);

        //extract ethernet header fields
        byte[] etherTypeRaw = PacketParsingUtils.extractEtherType(packetReceived.getPayload());
        int etherType = (0x0000ffff & ByteBuffer.wrap(etherTypeRaw).getShort());
        byte[] payload = packetReceived.getPayload();
        byte[] dstMacRaw = PacketParsingUtils.extractDstMac(payload);
        byte[] srcMacRaw = PacketParsingUtils.extractSrcMac(payload);
        String srcMac = PacketParsingUtils.rawMacToString(srcMacRaw);
        String dstMac = PacketParsingUtils.rawMacToString(dstMacRaw);
        //if lldp ignore
        if (etherType == 0x88cc) {
            LOG.debug("Received LLDP packet, ignoring..");
            return;
        }
        //if ipv6 and destination mac = 00:..:00, then ICN packet.
        if (etherType == 0x86dd && dstMac.equals("00:00:00:00:00:00")) {
            LOG.info("Received packet with src, dst MAC and ethernet type: {} & {} & {}", srcMac, dstMac, etherType);
            LOG.debug("Received ICN over SDN message.");
            byte[] srcIpv6Bytes = PacketParsingUtils.extractIpv6Source(payload);
            String srcIpv6 = PacketParsingUtils.rawBytesToString(srcIpv6Bytes);
            byte[] dsrIpv6Bytes = PacketParsingUtils.extractIpv6Destination(payload);
            String dstIpv6 = PacketParsingUtils.rawBytesToString(dsrIpv6Bytes);
            //LOG.info("Bloom filter {}", srcIpv6+dstIpv6);
            try {
                LOG.info("IPv6 src and dst address: {} and {}", Inet6Address.getByAddress(srcIpv6Bytes).getHostAddress(),
                        Inet6Address.getByAddress(dsrIpv6Bytes).getHostAddress());
            } catch (UnknownHostException e) {
                e.printStackTrace();
            }
            byte[] icnPayload = PacketParsingUtils.extractIcnPayload(payload);
            String str = PacketParsingUtils.rawBytesToString(icnPayload);
            LOG.debug("Payload is {} bytes: {}", icnPayload.length, str);
            byte[] icnScopesBytes = PacketParsingUtils.extractIcnScopesNumber(payload);
            int icnScopes = new BigInteger(icnScopesBytes).intValue();
            LOG.debug("Scopes size = {}", icnScopes);
            byte[] firstScopeBytes = PacketParsingUtils.extractFirstScope(payload);
            String firstScope = PacketParsingUtils.rawBytesToString(firstScopeBytes);
            LOG.debug("First scope {}", firstScope);

            //if scope is discovery scope then handle it appropriately
            if (firstScope.startsWith("DD")) {
                LOG.info("Received DISCOVERY message.");
                try {
                    LOG.info("IPv6 src and dst address: {} and {}", Inet6Address.getByAddress(srcIpv6Bytes).getHostAddress(),
                            Inet6Address.getByAddress(dsrIpv6Bytes).getHostAddress());
                } catch (UnknownHostException e) {
                    e.printStackTrace();
                }
                String offerScope = "EEEEEEEE";
                String hostMac = "host:" + srcMac;
                //calculate TMFID and prepare ICN response
                String tmFid = IcnIdConfigurator.getInstance().calculateTMFID(hostMac);
                prepareIcnOffer(payload, offerScope, tmFid);
                LOG.info("Updated Payload length {} ", payload.length);
                str = PacketParsingUtils.rawBytesToString(payload);
                LOG.info("Updated Payload {}", str);
                //send it to the interface you received it
                TransmitPacketInput input = new TransmitPacketInputBuilder()
                        .setPayload(payload)
                        .setNode(ingressNodeRef)
                        .setEgress(ingressNodeConnectorRef)
                        .build();
                packetProcessingService.transmitPacket(input);
            }
            return;
        }
    }
    /**
     * The utility method which creates the ICN discovery offer response.
     *
     * @param payload The payload to be altered.
     * @param offerScope The discovery offer to be added.
     * @param tmFid The TMFID to be included in the offer.
     */
    private void prepareIcnOffer(byte[] payload, String offerScope, String tmFid) {
        //Add new number of scopes = 2
        Arrays.fill(payload, 22 + 16 + 16 + 1, 22 + 16 + 16 + 2, (byte)2);

        //Add offer scope
        for (int i=0; i<offerScope.getBytes().length; i++)
            Arrays.fill(payload,
                    22 + 16 + 16 + 2 + 8 + i,
                    22 + 16 + 16 + 2 + 8 + i + 1,
                    offerScope.getBytes()[i]);

        //Add tmFid
        for (int i=0; i<tmFid.getBytes().length; i++)
            Arrays.fill(payload,
                    22 + 16 + 16 + 2 + 8 + 8 + i,
                    22 + 16 + 16 + 2 + 8 + 8 + i + 1,
                    tmFid.getBytes()[i]);

    }
}