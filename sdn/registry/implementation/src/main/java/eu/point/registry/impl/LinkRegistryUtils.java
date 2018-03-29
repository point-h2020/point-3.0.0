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
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.*;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntryKey;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 *The Link Registry class.
 *
 * @author George Petropoulos
 * @version cycle-2
 */
public class LinkRegistryUtils {

    private static final Logger LOG = LoggerFactory.getLogger(LinkRegistryUtils.class);
    private static LinkRegistryUtils instance = null;
    private DataBroker db;

    /**
     * The constructor class.
     */
    protected LinkRegistryUtils() {

    }

    /**
     * The method which returns the singleton instance.
     * @return The singleton instance.
     */
    public static LinkRegistryUtils getInstance() {
        if(instance == null) {
            instance = new LinkRegistryUtils();
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
        LOG.debug("Preparing to initialize the Link registry");
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<LinkRegistry> iid = InstanceIdentifier.create(LinkRegistry.class);
        LinkRegistry greetingRegistry = new LinkRegistryBuilder()
                .build();
        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, greetingRegistry);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<>("Failed to create Link registry", LOG));
    }

    /**
     * The method which writes to the registry a specific entry.
     *
     * @param input The entry to be written in the registry.
     * @see LinkRegistryEntry
     */
    public void writeToLinkRegistry(LinkRegistryEntry input) {
        LOG.debug("Writing to Link registry input {}.", input);
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<LinkRegistryEntry> iid = toInstanceIdentifier(input);

        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, input);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<Void>("Failed to write to Link registry", LOG));
    }

    /**
     * The method which deletes from the registry a specific entry.
     *
     * @param key The key of the entry to be deleted.
     */
    public void deleteFromLinkRegistry(String key) {
        LOG.debug("Removing from Link registry key {}.", key);
        WriteTransaction transaction = db.newReadWriteTransaction();
        InstanceIdentifier<LinkRegistryEntry> iid = toInstanceIdentifier(key);

        transaction.delete(LogicalDatastoreType.OPERATIONAL, iid);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<Void>("Failed to delete from Link registry", LOG));
    }

    /**
     * The method which maps an entry to an instance identifier.
     *
     * @param input The entry to be mapped to the relevant instance identifier.
     * @see LinkRegistryEntry
     */
    private InstanceIdentifier<LinkRegistryEntry> toInstanceIdentifier(LinkRegistryEntry input) {
        InstanceIdentifier<LinkRegistryEntry> iid = InstanceIdentifier.create(LinkRegistry.class)
                .child(LinkRegistryEntry.class,
                        new LinkRegistryEntryKey(input.getLinkName()));
        return iid;
    }

    /**
     * The method which maps the key of an entry to an instance identifier.
     *
     * @param key The key of the entry to be mapped to the relevant instance identifier.
     * @see LinkRegistryEntry
     */
    private InstanceIdentifier<LinkRegistryEntry> toInstanceIdentifier(String key) {
        InstanceIdentifier<LinkRegistryEntry> iid = InstanceIdentifier.create(LinkRegistry.class)
                .child(LinkRegistryEntry.class,
                        new LinkRegistryEntryKey(key));
        return iid;
    }

    /**
     * The method which reads from the registry an entry with a specific key.
     *
     * @param key The key of the entry to be fetched.
     * @return The relevant entry to be fetched.
     * @see LinkRegistryEntry
     */
    public LinkRegistryEntry readFromLinkRegistry(String key) {
        LOG.debug("Reading from Link registry for key {}.", key);
        LinkRegistryEntry link = null;
        ReadOnlyTransaction transaction = db.newReadOnlyTransaction();
        InstanceIdentifier<LinkRegistryEntry> iid = toInstanceIdentifier(key);
        CheckedFuture<Optional<LinkRegistryEntry>, ReadFailedException> future =
                transaction.read(LogicalDatastoreType.OPERATIONAL, iid);
        Optional<LinkRegistryEntry> optional = Optional.absent();
        try {
            optional = future.checkedGet();
        } catch (ReadFailedException e) {
            LOG.error("Reading Link failed:",e);
        }
        if(optional.isPresent()) {
           link = optional.get();
        }
        return link;
    }
}