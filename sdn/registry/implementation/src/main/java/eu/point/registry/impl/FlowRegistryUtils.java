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
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.FlowRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.FlowRegistryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.LinkRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.LinkRegistryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.flow.registry.FlowRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.flow.registry.FlowRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.flow.registry.FlowRegistryEntryKey;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntryKey;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.registry.NodeRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.Nodes;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.Node;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

/**
 * The Flow Registry class.
 *
 * @author Marievi Xezonaki
 * @version cycle-2
 */
public class FlowRegistryUtils {

    private static final Logger LOG = LoggerFactory.getLogger(FlowRegistryUtils.class);
    private static FlowRegistryUtils instance = null;
    private DataBroker db;
    private static int c = 1;

    /**
     * The constructor class.
     */
    protected FlowRegistryUtils() {

    }

    /**
     * The method which returns the singleton instance.
     * @return The singleton instance.
     */
    public static FlowRegistryUtils getInstance() {
        if (instance == null) {
            instance = new FlowRegistryUtils();
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
        LOG.debug("Preparing to initialize the Flow registry");
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<FlowRegistry> iid = InstanceIdentifier.create(FlowRegistry.class);
        FlowRegistry flowRegistry = new FlowRegistryBuilder()
                .build();
        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, flowRegistry);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<>("Failed to create Flow registry", LOG));
    }

    /**
     * The method which writes to the registry a specific entry.
     *
     * @param input The entry to be written in the registry.
     * @see FlowRegistryEntry
     */
    public void writeToFlowRegistry(FlowRegistryEntry input) {
        LOG.debug("Writing to Flow registry input {}.", input);
        LOG.info("Time " + c + " About to write to Flow registry input {}.", input);
        c++;
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<FlowRegistryEntry> iid = toInstanceIdentifier(input);

        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, input);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<Void>("Failed to write to Flow registry", LOG));
    }

    /**
     * The method which deletes from the registry a specific entry.
     *
     * @param key The key of the entry to be deleted.
     */
    public void deleteFromFlowRegistry(String key) {
        LOG.debug("Removing from Flow registry key {}.", key);
        WriteTransaction transaction = db.newReadWriteTransaction();
        InstanceIdentifier<FlowRegistryEntry> iid = toInstanceIdentifier(key);

        transaction.delete(LogicalDatastoreType.OPERATIONAL, iid);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<Void>("Failed to delete from Flow registry", LOG));
    }

    /**
     * The method which maps an entry to an instance identifier.
     *
     * @param input The entry to be mapped to the relevant instance identifier.
     * @see FlowRegistryEntry
     */
    private InstanceIdentifier<FlowRegistryEntry> toInstanceIdentifier(FlowRegistryEntry input) {
        InstanceIdentifier<FlowRegistryEntry> iid = InstanceIdentifier.create(FlowRegistry.class)
                .child(FlowRegistryEntry.class,
                        new FlowRegistryEntryKey(input.getEdgeNodeConnector()));
        return iid;
    }

    /**
     * The method which maps the key of an entry to an instance identifier.
     *
     * @param key The key of the entry to be mapped to the relevant instance identifier.
     * @see FlowRegistryEntry
     */
    private InstanceIdentifier<FlowRegistryEntry> toInstanceIdentifier(String key) {
        InstanceIdentifier<FlowRegistryEntry> iid = InstanceIdentifier.create(FlowRegistry.class)
                .child(FlowRegistryEntry.class,
                        new FlowRegistryEntryKey(key));
        return iid;
    }

    /**
     * The method which reads from the registry an entry with a specific key.
     *
     * @param key The key of the entry to be fetched.
     * @return The relevant entry to be fetched.
     * @see FlowRegistryEntry
     */
    public FlowRegistryEntry readFromFlowRegistry(String key) {
        LOG.debug("Reading from Flow registry for key {}.", key);
        FlowRegistryEntry flow = null;
        ReadOnlyTransaction transaction = db.newReadOnlyTransaction();
        InstanceIdentifier<FlowRegistryEntry> iid = toInstanceIdentifier(key);
        CheckedFuture<Optional<FlowRegistryEntry>, ReadFailedException> future =
                transaction.read(LogicalDatastoreType.OPERATIONAL, iid);
        Optional<FlowRegistryEntry> optional = Optional.absent();
        try {
            optional = future.checkedGet();
        } catch (ReadFailedException e) {
            LOG.error("Reading Link failed:", e);
        }
        if (optional.isPresent()) {
            flow = optional.get();
        }
        return flow;
    }

    /**
     * The method which reads all the entries from the registry.
     *
     * @return The list of all the entries.
     * @see FlowRegistryEntry
     */
    public List<FlowRegistryEntry> readAllFromFlowRegistry(){
        List<FlowRegistryEntry> flowRegistryList = new ArrayList<>();
        try {
            InstanceIdentifier<FlowRegistry> flowRegistryId = InstanceIdentifier.builder(
                    FlowRegistry.class)
                    .build();
            ReadOnlyTransaction flowRegistryTransaction = db.newReadOnlyTransaction();
            CheckedFuture<Optional<FlowRegistry>, ReadFailedException> flowRegistryFuture = flowRegistryTransaction
                    .read(LogicalDatastoreType.OPERATIONAL, flowRegistryId);
            Optional<FlowRegistry> flowRegistryOptional = Optional.absent();
            flowRegistryOptional = flowRegistryFuture.checkedGet();


            if (flowRegistryOptional != null && flowRegistryOptional.isPresent()) {
                flowRegistryList = flowRegistryOptional.get().getFlowRegistryEntry();
            }

            if (flowRegistryList != null) {
                for (FlowRegistryEntry entry : flowRegistryList) {
                    LOG.info("FR Entry " + entry);
                }
            }
        }
        catch (Exception e) {
            LOG.info("Failed to read all from the Flow Registry.");
        }
        return flowRegistryList;
    }
}
