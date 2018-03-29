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
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.FlowCapableNode;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.statistics.rev131111.NodeGroupStatistics;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.group.buckets.Bucket;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.group.statistics.buckets.BucketCounter;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.groups.Group;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.Nodes;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.Node;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TimerTask;

/**
 * The class which implements the fast failover group monitoring timer task.
 *
 * @author George Petropoulos, Marievi Xezonaki
 * @version cycle-2
 */
public class GroupMonitorTool extends TimerTask {

    private DataBroker db;
    private static final Logger LOG = LoggerFactory.getLogger(GroupMonitorTool.class);

    /**
     * The constructor class.
     */
    public GroupMonitorTool(DataBroker db) {
        this.db = db;
    }

    /**
     * The method which runs the group monitoring task.
     * It monitors the configured groups and their bytes and packets counters.
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
                NodeRegistryEntry nodeEntry = NodeRegistryUtils.getInstance()
                        .readFromNodeRegistry(node.getId().getValue());
                if (nodeEntry != null) {
                    String nodeId = nodeEntry.getNoneId();
                    LOG.info("Openflow node {}, ICN nodeID {} ", node.getId().getValue(), nodeId);
                    List<Group> groups = node.getAugmentation(FlowCapableNode.class).getGroup();
                    if (groups != null) {
                        for (Group g : groups) {
                            try {
                                long gBytes = g.getAugmentation(NodeGroupStatistics.class)
                                        .getGroupStatistics().getByteCount().getValue().longValue();
                                long gPackets = g.getAugmentation(NodeGroupStatistics.class)
                                        .getGroupStatistics().getPacketCount().getValue().longValue();
                                LOG.info("Group {} of type {}, packets/bytes {}/{}",
                                        g.getGroupId().getValue(), g.getGroupType(), gPackets, gBytes);

                                List<Bucket> buckets = g.getBuckets().getBucket();
                                Map<Long, Long> bucketPorts = new HashMap<>();
                                for (Bucket b : buckets) {
                                    bucketPorts.put(b.getBucketId().getValue(), b.getWatchPort());
                                }

                                //prepare the group monitoring message and send it to TM
                                TmSdnMessages.TmSdnMessage.GroupMonitoringMessage.Builder gmBuilder =
                                        TmSdnMessages.TmSdnMessage.GroupMonitoringMessage.newBuilder();
                                gmBuilder.setNodeID(nodeId)
                                        .setPackets(gPackets)
                                        .setBytes(gBytes)
                                        .setGroup(g.getGroupId().getValue());

                                List<BucketCounter> bucketCounters = g
                                        .getAugmentation(NodeGroupStatistics.class).getGroupStatistics().
                                                getBuckets().getBucketCounter();
                                for (BucketCounter bc : bucketCounters) {
                                    long bBytes = bc.getByteCount().getValue().longValue();
                                    long bPackets = bc.getPacketCount().getValue().longValue();
                                    LOG.info("Bucket {}, packets/bytes {}/{}",
                                            bc.getBucketId().getValue(), bPackets, bBytes);

                                    TmSdnMessages.TmSdnMessage.BucketMonitor.Builder bBuilder =
                                            TmSdnMessages.TmSdnMessage.BucketMonitor.newBuilder();
                                    bBuilder.setBucket(bc.getBucketId().getValue())
                                            .setPackets(bPackets)
                                            .setBytes(bBytes);
                                    gmBuilder.addBuckets(bBuilder.build());
                                    
                                }

                                TmSdnMessages.TmSdnMessage message = TmSdnMessages.TmSdnMessage.newBuilder()
                                        .setGroupMonitoringMessage(gmBuilder.build())
                                        .setType(TmSdnMessages.TmSdnMessage.TmSdnMessageType.GM)
                                        .build();
                                new Client().sendMessage(message);
                            } catch (Exception e) {
                                LOG.warn("Group statistics reading and sending failed:", e);
                            }
                        }
                    }
                }
            }

        }
    }
}
