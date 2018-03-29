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
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.GroupsRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.GroupsRegistryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.flow.registry.FlowRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.groups.registry.GroupsRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.groups.registry.GroupsRegistryEntryKey;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

/**
 * The Groups Registry class.
 *
 * @author Marievi Xezonaki
 * @version cycle-2
 */
public class GroupsRegistryUtils {

    private static final Logger LOG = LoggerFactory.getLogger(GroupsRegistryUtils.class);
    private static GroupsRegistryUtils instance = null;
    private DataBroker db;
    private static int c = 1;

    /**
     * The constructor class.
     */
    protected GroupsRegistryUtils() {

    }

    /**
     * The method which returns the singleton instance.
     * @return The singleton instance.
     */
    public static GroupsRegistryUtils getInstance() {
        if (instance == null) {
            instance = new GroupsRegistryUtils();
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
        InstanceIdentifier<GroupsRegistry> iid = InstanceIdentifier.create(GroupsRegistry.class);
        GroupsRegistry groupsRegistry = new GroupsRegistryBuilder()
                .build();
        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, groupsRegistry);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<>("Failed to create Flow registry", LOG));
    }

    /**
     * The method which writes to the registry a specific entry.
     *
     * @param input The entry to be written in the registry.
     * @see GroupsRegistryEntry
     */
    public void writeToGroupRegistry(GroupsRegistryEntry input) {
        LOG.info("Time " + c + " About to write to Groups registry input {}.", input);
        c++;
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<GroupsRegistryEntry> iid = toInstanceIdentifier(input);

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
    public void deleteFromGroupRegistry(String key) {
        LOG.debug("Removing from Flow registry key {}.", key);
        WriteTransaction transaction = db.newReadWriteTransaction();
        InstanceIdentifier<GroupsRegistryEntry> iid = toInstanceIdentifier(key);

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
    private InstanceIdentifier<GroupsRegistryEntry> toInstanceIdentifier(GroupsRegistryEntry input) {
        InstanceIdentifier<GroupsRegistryEntry> iid = InstanceIdentifier.create(GroupsRegistry.class)
                .child(GroupsRegistryEntry.class,
                        new GroupsRegistryEntryKey(input.getEdgeNodeConnector()));
        return iid;
    }

    /**
     * The method which maps the key of an entry to an instance identifier.
     *
     * @param key The key of the entry to be mapped to the relevant instance identifier.
     * @see FlowRegistryEntry
     */
    private InstanceIdentifier<GroupsRegistryEntry> toInstanceIdentifier(String key) {
        InstanceIdentifier<GroupsRegistryEntry> iid = InstanceIdentifier.create(GroupsRegistry.class)
                .child(GroupsRegistryEntry.class,
                        new GroupsRegistryEntryKey(key));
        return iid;
    }

    /**
     * The method which reads from the registry an entry with a specific key.
     *
     * @param key The key of the entry to be fetched.
     * @return The relevant entry to be fetched.
     * @see FlowRegistryEntry
     */
    public GroupsRegistryEntry readFromGroupRegistry(String key) {
        LOG.debug("Reading from Flow registry for key {}.", key);
        GroupsRegistryEntry groups = null;
        ReadOnlyTransaction transaction = db.newReadOnlyTransaction();
        InstanceIdentifier<GroupsRegistryEntry> iid = toInstanceIdentifier(key);
        CheckedFuture<Optional<GroupsRegistryEntry>, ReadFailedException> future =
                transaction.read(LogicalDatastoreType.OPERATIONAL, iid);
        Optional<GroupsRegistryEntry> optional = Optional.absent();
        try {
            optional = future.checkedGet();
        } catch (ReadFailedException e) {
            LOG.error("Reading Link failed:", e);
        }
        if (optional.isPresent()) {
            groups = optional.get();
        }
        return groups;
    }

    /**
     * The method which reads all the entries from the registry.
     *
     * @return The list of all the entries.
     * @see GroupsRegistryEntry
     */
    public List<GroupsRegistryEntry> readAllFromGroupRegistry(){
        List<GroupsRegistryEntry> groupsRegistryList = new ArrayList<>();
        try {
            InstanceIdentifier<GroupsRegistry> groupsRegistryId = InstanceIdentifier.builder(
                    GroupsRegistry.class)
                    .build();
            ReadOnlyTransaction groupsRegistryTransaction = db.newReadOnlyTransaction();
            CheckedFuture<Optional<GroupsRegistry>, ReadFailedException> groupsRegistryFuture = groupsRegistryTransaction
                    .read(LogicalDatastoreType.OPERATIONAL, groupsRegistryId);
            Optional<GroupsRegistry> groupsRegistryOptional = Optional.absent();
            groupsRegistryOptional = groupsRegistryFuture.checkedGet();


            if (groupsRegistryOptional != null && groupsRegistryOptional.isPresent()) {
                groupsRegistryList = groupsRegistryOptional.get().getGroupsRegistryEntry();
            }

            if (groupsRegistryList != null) {
                for (GroupsRegistryEntry entry : groupsRegistryList) {
                    LOG.info("FR Entry " + entry);
                }
            }
        }
        catch (Exception e) {
            LOG.info("Failed to read all from the Flow Registry.");
        }
        return groupsRegistryList;
    }
}
