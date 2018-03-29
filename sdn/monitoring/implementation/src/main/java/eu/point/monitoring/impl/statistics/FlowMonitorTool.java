/*
 * Copyright © 2017 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.monitoring.impl.statistics;

import com.google.common.base.Optional;
import com.google.common.util.concurrent.CheckedFuture;
import eu.point.client.Client;
import eu.point.registry.impl.NodeRegistryUtils;
import eu.point.tmsdn.impl.TmSdnMessages;
import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.md.sal.binding.api.ReadOnlyTransaction;
import org.opendaylight.controller.md.sal.common.api.data.LogicalDatastoreType;
import org.opendaylight.controller.md.sal.common.api.data.ReadFailedException;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.registry.NodeRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.ietf.params.xml.ns.yang.ietf.yang.types.rev130715.MacAddress;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.FlowCapableNode;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.Table;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.table.Flow;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.statistics.rev130819.FlowStatisticsData;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.Nodes;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.Node;
import org.opendaylight.yang.gen.v1.urn.opendaylight.l2.types.rev130827.EtherType;
import org.opendaylight.yang.gen.v1.urn.opendaylight.model.match.types.rev131026.match.layer._3.match.Ipv6MatchArbitraryBitMask;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.TimerTask;

/**
 * The class which implements the ICN flow monitoring timer task.
 *
 * @author George Petropoulos, Marievi Xezonaki
 * @version cycle-2
 */
public class FlowMonitorTool extends TimerTask {

    private DataBroker db;
    private static final Logger LOG = LoggerFactory.getLogger(FlowMonitorTool.class);

    /**
     * The constructor class.
     */
    public FlowMonitorTool(DataBroker db) {
        this.db = db;
    }

    /**
     * The method which runs the flow monitoring task.
     * It monitors the configured flow rules and their bytes and packets counters.
     */
    public void run() {
        //Read Nodes from datastore
        List<Node> nodeList = null;
        InstanceIdentifier<Nodes> nodesIid = InstanceIdentifier
                .builder(Nodes.class)
                .build();
        ReadOnlyTransaction nodesTransaction = db.newReadOnlyTransaction();
        CheckedFuture<Optional<Nodes>, ReadFailedException> nodesFuture =
                nodesTransaction.read(LogicalDatastoreType.OPERATIONAL, nodesIid);
        Optional<Nodes> nodesOptional = Optional.absent();
        try {
            nodesOptional = nodesFuture.checkedGet();
        } catch (ReadFailedException e) {
            LOG.warn("Nodes reading failed:", e);
        }
        if (nodesOptional.isPresent()) {
            nodeList = nodesOptional.get().getNode();
        }

        //Scan each node and get each node's connectors
        if (nodeList != null) {
            for (Node node : nodeList) {
                NodeRegistryEntry nodeEntry = NodeRegistryUtils.getInstance()
                        .readFromNodeRegistry(node.getId().getValue());
                if (nodeEntry != null) {
                    String nodeId = nodeEntry.getNoneId();
                    LOG.info("Openflow node {}, ICN nodeID {} ", node.getId().getValue(), nodeId);
                    List<Table> tables = node.getAugmentation(FlowCapableNode.class).getTable();
                    if (tables != null) {
                        for (Table t : tables) {
                            List<Flow> flows = t.getFlow();
                            if (flows != null) {
                                for (Flow f : flows) {
                                    try {
                                        if (isIcnFlow(f)) {
                                            FlowStatisticsData flowStats =
                                                    f.getAugmentation(FlowStatisticsData.class);
                                            long bytes = flowStats.getFlowStatistics()
                                                    .getByteCount().getValue().longValue();
                                            long packets = flowStats.getFlowStatistics()
                                                    .getPacketCount().getValue().longValue();
                                            LOG.info("Table {}, Flow {}, packets/bytes {}/{}",
                                                    t.getId(), f.getId().getValue(), packets, bytes);

                                            Ipv6MatchArbitraryBitMask arbitraryMatch = (Ipv6MatchArbitraryBitMask)
                                                    f.getMatch().getLayer3Match();
                                            String src_ipv6 = "";
                                            String dst_ipv6 = "";
                                            if (arbitraryMatch.getIpv6SourceAddressNoMask() != null)
                                                src_ipv6 = arbitraryMatch.getIpv6SourceAddressNoMask().getValue();
                                            if (arbitraryMatch.getIpv6DestinationAddressNoMask() != null)
                                                dst_ipv6 = arbitraryMatch.getIpv6DestinationAddressNoMask().getValue();

                                            //prepare the flow monitoring message and send it to TM
                                            TmSdnMessages.TmSdnMessage.FlowMonitoringMessage.Builder fmBuilder =
                                                    TmSdnMessages.TmSdnMessage.FlowMonitoringMessage.newBuilder();
                                            fmBuilder.setNodeID(nodeId)
                                                    .setBytes(bytes)
                                                    .setPackets(packets)
                                                    .setSrcIpv6(src_ipv6)
                                                    .setDstIpv6(dst_ipv6)
                                                    .setTable(t.getId());
                                            TmSdnMessages.TmSdnMessage message = TmSdnMessages.TmSdnMessage.newBuilder()
                                                    .setFlowMonitoringMessage(fmBuilder.build())
                                                    .setType(TmSdnMessages.TmSdnMessage.TmSdnMessageType.FM)
                                                    .build();
                                            new Client().sendMessage(message);
                                        }
                                    } catch (Exception e) {
                                        LOG.warn("Flow statistics reading and sending failed:", e);
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }
    }

    private boolean isIcnFlow(Flow f) {
        boolean etherTypeMatch = false;
        boolean etherDstMatch = false;
        boolean arbMatch = false;

        if (f.getMatch() != null) {
            if (f.getMatch().getEthernetMatch() != null && f.getMatch().getEthernetMatch().getEthernetType() != null
                && f.getMatch().getEthernetMatch().getEthernetDestination() != null){
                etherTypeMatch = f.getMatch().getEthernetMatch().getEthernetType().getType()
                        .equals(new EtherType(0x86DDL));
                etherDstMatch = f.getMatch().getEthernetMatch().getEthernetDestination().getAddress()
                        .equals(new MacAddress("00:00:00:00:00:00"));
            }
            Ipv6MatchArbitraryBitMask arbitraryMatch = (Ipv6MatchArbitraryBitMask) f.getMatch().getLayer3Match();
            if (arbitraryMatch != null)
                arbMatch = true;
        }
        LOG.debug("ICN flow? {}, {}, {}", etherTypeMatch, etherDstMatch, arbMatch);
        return etherTypeMatch && etherDstMatch && arbMatch;

    }
}
