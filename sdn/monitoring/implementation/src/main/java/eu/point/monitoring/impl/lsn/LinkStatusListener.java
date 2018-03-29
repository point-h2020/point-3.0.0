/*
 * Copyright © 2017 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.monitoring.impl.lsn;

import java.util.*;

import eu.point.client.Client;
import eu.point.registry.impl.NodeConnectorRegistryUtils;
import eu.point.registry.impl.NodeRegistryUtils;
import eu.point.tmsdn.impl.TmSdnMessages;
import org.opendaylight.controller.md.sal.binding.api.DataChangeListener;
import org.opendaylight.controller.md.sal.common.api.data.AsyncDataChangeEvent;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.connector.registry.NodeConnectorRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.registry.NodeRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.FlowCapableNodeConnector;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.port.rev130925.flow.capable.port.State;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.node.NodeConnector;
import org.opendaylight.yangtools.yang.binding.DataObject;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * The class which implements link status listener.
 *
 * @author George Petropoulos, Marievi Xezonaki
 * @version cycle-2
 */
public class LinkStatusListener implements DataChangeListener {

    private static final Logger LOG = LoggerFactory.getLogger(LinkStatusListener.class);
    private static Map<String, Boolean> nodeConnectorsStatus = new HashMap<>();

    /**
     * The constructor class.
     */
    public LinkStatusListener() {
    }

    /**
     * The method which monitors changes in the datastore topology,
     * and sends ADD or RMV link state notifications to the topology manager.
     *
     * @param change The changed data to trigger the monitoring functions.
     */
    @Override
    public void onDataChanged(AsyncDataChangeEvent<InstanceIdentifier<?>, DataObject> change) {
        Map<InstanceIdentifier<?>, DataObject> updatedData = change.getUpdatedData();
        Map<InstanceIdentifier<?>, DataObject> createdData = change.getCreatedData();
        Map<InstanceIdentifier<?>, DataObject> originalData = change.getOriginalData();
        Set<InstanceIdentifier<?>> deletedData = change.getRemovedPaths();

        //if new data then add them
        if (createdData != null && !createdData.isEmpty()) {
            for (Map.Entry<InstanceIdentifier<?>, DataObject> entrySet : createdData.entrySet()) {

                final DataObject dataobject = entrySet.getValue();
                if (dataobject instanceof NodeConnector) {

                    String nodeConnectorId = ((NodeConnector) dataobject).getId().getValue();
                    LOG.info("NC in created:" + nodeConnectorId);
                    FlowCapableNodeConnector fcNodeConnector = ((NodeConnector) dataobject)
                            .getAugmentation(FlowCapableNodeConnector.class);

                    State newState = fcNodeConnector.getState();
                    if (!nodeConnectorsStatus.containsKey(nodeConnectorId)) {
                        nodeConnectorsStatus.put(nodeConnectorId, newState.isLinkDown());
                        return;
                    }
                }
            }
        }

        //if updated data then update them
        if (updatedData != null && !updatedData.isEmpty()) {
            for (Map.Entry<InstanceIdentifier<?>, DataObject> entrySet : updatedData.entrySet()) {

                final DataObject dataobject = entrySet.getValue();
                if (dataobject instanceof NodeConnector) {

                    String sourceNodeConnectorId = ((NodeConnector) dataobject).getId().getValue();
                    FlowCapableNodeConnector fcNodeConnector = ((NodeConnector) dataobject)
                            .getAugmentation(FlowCapableNodeConnector.class);

                    State newState = fcNodeConnector.getState();
                    if (!nodeConnectorsStatus.containsKey(sourceNodeConnectorId)) {
                        nodeConnectorsStatus.put(sourceNodeConnectorId, newState.isLinkDown());
                        return;
                    }
                    NodeConnectorRegistryEntry ncRegistryEntry = NodeConnectorRegistryUtils
                            .getInstance().readFromNodeConnectorRegistry(sourceNodeConnectorId);

                    if (ncRegistryEntry == null)
                        return;

                    String srcNodeId = "";

                    NodeRegistryEntry srcNodeEntry = NodeRegistryUtils.getInstance()
                            .readFromNodeRegistry(ncRegistryEntry.getSrcNode());
                    if (srcNodeEntry != null)
                        srcNodeId = srcNodeEntry.getNoneId();
                    String dstNodeId = "";
                    NodeRegistryEntry dstNodeEntry = NodeRegistryUtils.getInstance()
                            .readFromNodeRegistry(ncRegistryEntry.getDstNode());
                    if (dstNodeEntry != null)
                        dstNodeId = dstNodeEntry.getNoneId();

                    int nodeConnectorId = Integer.valueOf(sourceNodeConnectorId.split(":")[2]);
                    //if link is down, but it wasn't then, then update it and also inform the TM
                    if (newState.isLinkDown() && !nodeConnectorsStatus.get(sourceNodeConnectorId).booleanValue()) {
                        LOG.info("Link is down!");
                        if (!srcNodeId.isEmpty() && !dstNodeId.isEmpty()) {
                            LOG.info("RMV {} {}", srcNodeId, dstNodeId);
                            nodeConnectorsStatus.put(sourceNodeConnectorId, newState.isLinkDown());
                            TmSdnMessages.TmSdnMessage.LinkStatusMessage.Builder lsBuilder =
                                    TmSdnMessages.TmSdnMessage.LinkStatusMessage.newBuilder();
                            lsBuilder.setNodeID1(srcNodeId)
                                    .setNodeID2(dstNodeId)
                                    .setNodeConnector(nodeConnectorId)
                                    .setLsmType(TmSdnMessages.TmSdnMessage.LinkStatusMessage.LSMType.RMV);
                            TmSdnMessages.TmSdnMessage message = TmSdnMessages.TmSdnMessage.newBuilder()
                                    .setLinkStatusMessage(lsBuilder.build())
                                    .setType(TmSdnMessages.TmSdnMessage.TmSdnMessageType.LS)
                                    .build();
                            new Client().sendMessage(message);
                        }
                    }
                    // else if link is up, but it wasn't then, then update it and also inform the TM
                    else if (!newState.isLinkDown() && nodeConnectorsStatus.get(sourceNodeConnectorId).booleanValue()){
                        LOG.info("Link is up!");
                        if (!srcNodeId.isEmpty() && !dstNodeId.isEmpty()) {
                            LOG.info("ADD {} {}", srcNodeId, dstNodeId);
                            nodeConnectorsStatus.put(sourceNodeConnectorId, newState.isLinkDown());
                            TmSdnMessages.TmSdnMessage.LinkStatusMessage.Builder lsBuilder =
                                    TmSdnMessages.TmSdnMessage.LinkStatusMessage.newBuilder();
                            lsBuilder.setNodeID1(srcNodeId)
                                    .setNodeID2(dstNodeId)
                                    .setNodeConnector(nodeConnectorId)
                                    .setLsmType(TmSdnMessages.TmSdnMessage.LinkStatusMessage.LSMType.ADD);
                            TmSdnMessages.TmSdnMessage message = TmSdnMessages.TmSdnMessage.newBuilder()
                                    .setLinkStatusMessage(lsBuilder.build())
                                    .setType(TmSdnMessages.TmSdnMessage.TmSdnMessageType.LS)
                                    .build();
                            new Client().sendMessage(message);
                        }
                    }
                }
            }
        }
    }

}