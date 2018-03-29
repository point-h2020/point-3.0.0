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
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.NodeRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.NodeRegistryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.registry.NodeRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.registry.NodeRegistryEntryKey;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * The Node Registry class.
 *
 * @author George Petropoulos
 * @version cycle-2
 */
public class NodeRegistryUtils {

    private static final Logger LOG = LoggerFactory.getLogger(NodeRegistryUtils.class);
    private static NodeRegistryUtils instance = null;
    private DataBroker db;

    /**
     * The constructor class.
     */
    protected NodeRegistryUtils() {

    }

    /**
     * The method which returns the singleton instance.
     * @return The singleton instance.
     */
    public static NodeRegistryUtils getInstance() {
        if (instance == null) {
            instance = new NodeRegistryUtils();
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
        LOG.debug("Preparing to initialize the Node registry");
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<NodeRegistry> iid = InstanceIdentifier.create(NodeRegistry.class);
        NodeRegistry greetingRegistry = new NodeRegistryBuilder()
                .build();
        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, greetingRegistry);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future, new LoggingFuturesCallBack<>("Failed to create Node registry", LOG));
    }

    /**
     * The method which writes to the registry a specific entry.
     *
     * @param input The entry to be written in the registry.
     * @see NodeRegistryEntry
     */
    public void writeToNodeRegistry(NodeRegistryEntry input) {
        LOG.debug("Writing to Node registry input {}.", input);
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<NodeRegistryEntry> iid = toInstanceIdentifier(input);

        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, input);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<Void>("Failed to write to Node registry", LOG));
    }

    /**
     * The method which deletes from the registry a specific entry.
     *
     * @param key The key of the entry to be deleted.
     */
    public void deleteFromNodeRegistry(String key) {
        LOG.debug("Removing from Node registry key {}.", key);
        WriteTransaction transaction = db.newReadWriteTransaction();
        InstanceIdentifier<NodeRegistryEntry> iid = toInstanceIdentifier(key);

        transaction.delete(LogicalDatastoreType.OPERATIONAL, iid);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<Void>("Failed to delete from Node registry", LOG));
    }

    /**
     * The method which maps an entry to an instance identifier.
     *
     * @param input The entry to be mapped to the relevant instance identifier.
     * @see NodeRegistryEntry
     */
    private InstanceIdentifier<NodeRegistryEntry> toInstanceIdentifier(NodeRegistryEntry input) {
        InstanceIdentifier<NodeRegistryEntry> iid = InstanceIdentifier.create(NodeRegistry.class)
                .child(NodeRegistryEntry.class,
                        new NodeRegistryEntryKey(input.getNoneName()));
        return iid;
    }

    /**
     * The method which maps the key of an entry to an instance identifier.
     *
     * @param key The key of the entry to be mapped to the relevant instance identifier.
     * @see NodeRegistryEntry
     */
    private InstanceIdentifier<NodeRegistryEntry> toInstanceIdentifier(String key) {
        InstanceIdentifier<NodeRegistryEntry> iid = InstanceIdentifier.create(NodeRegistry.class)
                .child(NodeRegistryEntry.class,
                        new NodeRegistryEntryKey(key));
        return iid;
    }

    /**
     * The method which reads from the registry an entry with a specific key.
     *
     * @param nodeName The key of the entry to be fetched.
     * @return The relevant entry to be fetched.
     * @see NodeRegistryEntry
     */
    public NodeRegistryEntry readFromNodeRegistry(String nodeName) {
        LOG.debug("Reading from Node registry for nodeName {}.", nodeName);
        NodeRegistryEntry node = null;
        ReadOnlyTransaction transaction = db.newReadOnlyTransaction();
        InstanceIdentifier<NodeRegistryEntry> iid = toInstanceIdentifier(nodeName);
        CheckedFuture<Optional<NodeRegistryEntry>, ReadFailedException> future =
                transaction.read(LogicalDatastoreType.OPERATIONAL, iid);
        Optional<NodeRegistryEntry> optional = Optional.absent();
        try {
            optional = future.checkedGet();
        } catch (ReadFailedException e) {
            LOG.error("Reading Node failed:", e);
        }
        if (optional.isPresent()) {
            node = optional.get();
        }
        return node;
    }
}