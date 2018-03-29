/*
 * Copyright © 2017 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.monitoring.impl.statistics;

import java.math.BigInteger;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.List;
import java.util.TimerTask;

import eu.point.client.Client;
import eu.point.registry.impl.LinkRegistryUtils;
import eu.point.registry.impl.NodeConnectorRegistryUtils;
import eu.point.registry.impl.NodeRegistryUtils;
import eu.point.tmsdn.impl.TmSdnMessages;
import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.md.sal.binding.api.ReadOnlyTransaction;
import org.opendaylight.controller.md.sal.common.api.data.LogicalDatastoreType;
import org.opendaylight.controller.md.sal.common.api.data.ReadFailedException;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.connector.registry.NodeConnectorRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.registry.NodeRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.ietf.params.xml.ns.yang.ietf.inet.types.rev130715.Ipv6Address;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.FlowCapableNode;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.Table;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.TableKey;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.table.Flow;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.table.FlowKey;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.flow.Match;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.NodeId;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.Nodes;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.node.NodeConnector;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.Node;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.NodeKey;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.instruction.list.Instruction;
import org.opendaylight.yang.gen.v1.urn.opendaylight.model.match.types.rev131026.match.Layer3Match;
import org.opendaylight.yang.gen.v1.urn.opendaylight.model.match.types.rev131026.match.layer._3.match.Ipv6MatchArbitraryBitMaskBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.opendaylight.ipv6.arbitrary.bitmask.fields.rev160224.Ipv6ArbitraryMask;
import org.opendaylight.yang.gen.v1.urn.opendaylight.port.statistics.rev131214.FlowCapableNodeConnectorStatisticsData;
import org.opendaylight.yang.gen.v1.urn.opendaylight.port.statistics.rev131214.flow.capable.node.connector.statistics.FlowCapableNodeConnectorStatistics;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.base.Optional;
import com.google.common.util.concurrent.CheckedFuture;

/**
 * The class which implements the traffic monitoring timer task.
 *
 * @author George Petropoulos, Marievi Xezonaki
 * @version cycle-2
 */
public class TrafficMonitorTool extends TimerTask {

    private DataBroker db;
    private static final Logger LOG = LoggerFactory.getLogger(TrafficMonitorTool.class);

    /**
     * The constructor class.
     */
    public TrafficMonitorTool(DataBroker db) {
        this.db = db;
    }

    /**
     * The method which runs the monitoring task.
     * It monitors the node connector statistics, specifically packets and
     * bytes received and transmitted and sends them to the TM.
     */
    public void run() {
        //Read Nodes from datastore
        List<Node> nodeList = null;
        InstanceIdentifier<Nodes> nodesIid = InstanceIdentifier
                .builder(Nodes.class)
                .build();
        ReadOnlyTransaction nodesTransaction = db.newReadOnlyTransaction();
        CheckedFuture<Optional<Nodes>, ReadFailedException> nodesFuture = nodesTransaction.read(LogicalDatastoreType.OPERATIONAL, nodesIid);
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
                LOG.info("Node ID: " + node.getId().getValue());
                List<NodeConnector> nodeConnectorList = node.getNodeConnector();
                if (nodeConnectorList != null) {
                    for (NodeConnector nodeConnector : nodeConnectorList) {

                        NodeConnectorRegistryEntry ncEntry = NodeConnectorRegistryUtils
                                .getInstance().readFromNodeConnectorRegistry(nodeConnector.getId().getValue());
                        //If there is some entry for this node connector in the registry then send data.
                        if (ncEntry != null) {
                            try {
                                //extract the relevant statistics
                                FlowCapableNodeConnectorStatisticsData statData =
                                        nodeConnector.getAugmentation(FlowCapableNodeConnectorStatisticsData.class);
                                FlowCapableNodeConnectorStatistics statistics =
                                        statData.getFlowCapableNodeConnectorStatistics();
                                BigInteger packetsReceived = statistics.getPackets().getReceived();
                                BigInteger packetsTransmitted = statistics.getPackets().getTransmitted();
                                BigInteger bytesReceived = statistics.getBytes().getReceived();
                                BigInteger bytesTransmitted = statistics.getBytes().getTransmitted();
                                BigInteger receiveErrors = statistics.getReceiveErrors();
                                BigInteger transmitErrors = statistics.getTransmitErrors();
                                BigInteger receiveDrops = statistics.getReceiveDrops();
                                BigInteger transmitDrops = statistics.getTransmitDrops();
                                String srcNode = ncEntry.getSrcNode();
                                String dstNode = ncEntry.getDstNode();
                                String nodeConnectorName = ncEntry.getNoneConnectorName();
                                String srcNodeId = "";

                                NodeRegistryEntry srcNodeEntry = NodeRegistryUtils.getInstance()
                                        .readFromNodeRegistry(ncEntry.getSrcNode());
                                if (srcNodeEntry != null)
                                    srcNodeId = srcNodeEntry.getNoneId();
                                String dstNodeId = "";
                                NodeRegistryEntry dstNodeEntry = NodeRegistryUtils.getInstance()
                                        .readFromNodeRegistry(ncEntry.getDstNode());
                                if (dstNodeEntry != null)
                                    dstNodeId = dstNodeEntry.getNoneId();

                                LOG.info("srcNode {}, dstNode {}", srcNode, dstNode);
                                LOG.info("srcNodeId {}, dstNodeId {}", srcNodeId, dstNodeId);
                                LOG.info("PT, PR, BT, BR, ET, ER, DT, DR: {}, {}, {}, {}, {}, {}, {}, {}",
                                        packetsTransmitted.toString(), packetsReceived.toString(),
                                        bytesTransmitted.toString(), bytesReceived.toString(),
                                        transmitErrors.toString(), receiveErrors.toString(),
                                        transmitDrops.toString(), receiveDrops.toString());
                                int nodeConnectorId = Integer.valueOf(nodeConnector.getId()
                                        .getValue().split(":")[2]);
                                //prepare the traffic monitoring message and send it to TM
                                TmSdnMessages.TmSdnMessage.TrafficMonitoringMessage.Builder tmBuilder =
                                        TmSdnMessages.TmSdnMessage.TrafficMonitoringMessage.newBuilder();

                                tmBuilder.setNodeID1(srcNodeId)
                                        .setNodeID2(dstNodeId)
                                        .setNodeConnector(nodeConnectorId)
                                        .setBytesReceived(bytesReceived.longValue())
                                        .setBytesTransmitted(bytesTransmitted.longValue())
                                        .setPacketsReceived(packetsReceived.longValue())
                                        .setPacketsTransmitted(packetsTransmitted.longValue())
                                        .setReceiveDrops(receiveDrops.longValue())
                                        .setTransmitDrops(transmitDrops.longValue())
                                        .setReceiveErrors(receiveErrors.longValue())
                                        .setTransmitErrors(transmitErrors.longValue())
                                        .setNodeConnectorName(nodeConnectorName);
                                TmSdnMessages.TmSdnMessage message = TmSdnMessages.TmSdnMessage.newBuilder()
                                        .setTrafficMonitoringMessage(tmBuilder.build())
                                        .setType(TmSdnMessages.TmSdnMessage.TmSdnMessageType.TM)
                                        .build();
                                new Client().sendMessage(message);
                            } catch (Exception e) {
                                LOG.warn("Statistics reading and sending failed:", e);
                            }
                        }
                    }
                }
            }
        }
    }

    public void getNodeFlowRules(String switchName){
        if (this.db == null) {
            return;
        }
        ReadOnlyTransaction transaction = db.newReadOnlyTransaction();

        InstanceIdentifier<Node> nodeInstanceIdentifier = InstanceIdentifier
                .builder(Nodes.class)
                .child(Node.class, new NodeKey(new NodeId(switchName))).build();

        CheckedFuture<Optional<Node>, ReadFailedException> future =
                transaction.read(LogicalDatastoreType.CONFIGURATION, nodeInstanceIdentifier);
        Optional<Node> optional = Optional.absent();
        try {
            optional = future.checkedGet();
        }
        catch (ReadFailedException e) {
            e.printStackTrace();
        }
        if (!optional.isPresent()) {
            return;
        }
        Node node = optional.get();
        List<Table> nodeTables = node.getAugmentation(FlowCapableNode.class).getTable();
        if (nodeTables != null && nodeTables.size() > 0) {
            for (Table table : nodeTables) {
                List<Flow> tableFlows = table.getFlow();
                for (Flow flow : tableFlows) {
                    List<Instruction> instructionList = flow.getInstructions().getInstruction();
                    for (Instruction instructionElement : instructionList){

                    }
                }
            }
        }


    }

}
