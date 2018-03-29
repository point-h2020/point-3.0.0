/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.registry.impl;

import com.google.common.base.Optional;
import com.google.common.util.concurrent.CheckedFuture;
import com.google.common.util.concurrent.Futures;
import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.md.sal.binding.api.ReadOnlyTransaction;
import org.opendaylight.controller.md.sal.binding.api.WriteTransaction;
import org.opendaylight.controller.md.sal.common.api.data.LogicalDatastoreType;
import org.opendaylight.controller.md.sal.common.api.data.ReadFailedException;
import org.opendaylight.controller.md.sal.common.api.data.TransactionCommitFailedException;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.NodeConnectorRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.NodeConnectorRegistryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.connector.registry.NodeConnectorRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.connector.registry.NodeConnectorRegistryEntryKey;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * The Node connector Registry class.
 *
 * @author George Petropoulos
 * @version cycle-2
 */
public class NodeConnectorRegistryUtils {

    private static final Logger LOG = LoggerFactory.getLogger(NodeConnectorRegistryUtils.class);
    private static NodeConnectorRegistryUtils instance = null;
    private DataBroker db;


    /**
     * The constructor class.
     */
    protected NodeConnectorRegistryUtils() {

    }

    /**
     * The method which returns the singleton instance.
     * @return The singleton instance.
     */
    public static NodeConnectorRegistryUtils getInstance() {
        if (instance == null) {
            instance = new NodeConnectorRegistryUtils();
        }
        return instance;
    }

    public void setDb(DataBroker db) {
        this.db = db;
    }

    /**
     * The method which initializes the registry data tree.
     */
    public void initializeDataTree() {
        LOG.debug("Preparing to initialize the NodeConnector registry");
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<NodeConnectorRegistry> iid = InstanceIdentifier.create(NodeConnectorRegistry.class);
        NodeConnectorRegistry greetingRegistry = new NodeConnectorRegistryBuilder()
                .build();
        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, greetingRegistry);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future, new LoggingFuturesCallBack<>("Failed to create NodeConnector registry", LOG));
    }

    /**
     * The method which writes to the registry a specific entry.
     *
     * @param input The entry to be written in the registry.
     * @see NodeConnectorRegistryEntry
     */
    public void writeToNodeConnectorRegistry(NodeConnectorRegistryEntry input) {
        LOG.debug("Writing to NodeConnector registry input {}.", input);
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<NodeConnectorRegistryEntry> iid = toInstanceIdentifier(input);

        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, input);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<Void>("Failed to write to NodeConnector registry", LOG));
    }

    /**
     * The method which deletes from the registry a specific entry.
     *
     * @param key The key of the entry to be deleted.
     */
    public void deleteFromNodeConnectorRegistry(String key) {
        LOG.debug("Removing from NodeConnector registry key {}.", key);
        WriteTransaction transaction = db.newReadWriteTransaction();
        InstanceIdentifier<NodeConnectorRegistryEntry> iid = toInstanceIdentifier(key);

        transaction.delete(LogicalDatastoreType.OPERATIONAL, iid);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<Void>("Failed to delete from NodeConnector registry", LOG));
    }

    /**
     * The method which maps an entry to an instance identifier.
     *
     * @param input The entry to be mapped to the relevant instance identifier.
     * @see NodeConnectorRegistryEntry
     */
    private InstanceIdentifier<NodeConnectorRegistryEntry> toInstanceIdentifier(NodeConnectorRegistryEntry
                                                                                        input) {
        InstanceIdentifier<NodeConnectorRegistryEntry> iid = InstanceIdentifier
                .create(NodeConnectorRegistry.class)
                .child(NodeConnectorRegistryEntry.class,
                        new NodeConnectorRegistryEntryKey(input.getNoneConnectorName()));
        return iid;
    }

    /**
     * The method which maps the key of an entry to an instance identifier.
     *
     * @param key The key of the entry to be mapped to the relevant instance identifier.
     * @see NodeConnectorRegistryEntry
     */
    private InstanceIdentifier<NodeConnectorRegistryEntry> toInstanceIdentifier(String key) {
        InstanceIdentifier<NodeConnectorRegistryEntry> iid = InstanceIdentifier
                .create(NodeConnectorRegistry.class)
                .child(NodeConnectorRegistryEntry.class,
                        new NodeConnectorRegistryEntryKey(key));
        return iid;
    }

    /**
     * The method which reads from the registry an entry with a specific key.
     *
     * @param nodeName The key of the entry to be fetched.
     * @return The relevant entry to be fetched.
     * @see NodeConnectorRegistryEntry
     */
    public NodeConnectorRegistryEntry readFromNodeConnectorRegistry(String nodeName) {
        LOG.debug("Reading from NodeConnector registry for nodeName {}.", nodeName);
        NodeConnectorRegistryEntry node = null;
        ReadOnlyTransaction transaction = db.newReadOnlyTransaction();
        InstanceIdentifier<NodeConnectorRegistryEntry> iid = toInstanceIdentifier(nodeName);
        CheckedFuture<Optional<NodeConnectorRegistryEntry>, ReadFailedException> future =
                transaction.read(LogicalDatastoreType.OPERATIONAL, iid);
        Optional<NodeConnectorRegistryEntry> optional = Optional.absent();
        try {
            optional = future.checkedGet();
        } catch (ReadFailedException e) {
            LOG.error("Reading NodeConnector failed:", e);
        }
        if (optional.isPresent()) {
            node = optional.get();
        }
        return node;
    }
}