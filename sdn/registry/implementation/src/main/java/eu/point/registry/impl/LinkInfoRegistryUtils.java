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
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.flow.registry.FlowRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.info.registry.LinkInfoRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.info.registry.LinkInfoRegistryEntryKey;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntryKey;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

/**
 * The Link Info Registry class.
 *
 * @author Marievi Xezonaki
 * @version cycle-2
 */

public class LinkInfoRegistryUtils {

    private static final Logger LOG = LoggerFactory.getLogger(LinkInfoRegistryUtils.class);
    private static LinkInfoRegistryUtils instance = null;
    private DataBroker db;

    /**
     * The constructor class.
     */
    protected LinkInfoRegistryUtils() {

    }

    /**
     * The method which returns the singleton instance.
     * @return The singleton instance.
     */
    public static LinkInfoRegistryUtils getInstance() {
        if(instance == null) {
            instance = new LinkInfoRegistryUtils();
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
        LOG.debug("Preparing to initialize the Link Info registry");
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<LinkInfoRegistry> iid = InstanceIdentifier.create(LinkInfoRegistry.class);
        LinkInfoRegistry linkInfoRegistry = new LinkInfoRegistryBuilder()
                .build();
        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, linkInfoRegistry);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<>("Failed to create Link Info registry", LOG));
    }

    /**
     * The method which writes to the registry a specific entry.
     *
     * @param input The entry to be written in the registry.
     * @see LinkInfoRegistryEntry
     */
    public void writeToLinkInfoRegistry(LinkInfoRegistryEntry input) {
        LOG.debug("Writing to Link registry input {}.", input);
        WriteTransaction transaction = db.newWriteOnlyTransaction();
        InstanceIdentifier<LinkInfoRegistryEntry> iid = toInstanceIdentifier(input);

        transaction.put(LogicalDatastoreType.OPERATIONAL, iid, input);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<Void>("Failed to write to Link Info registry", LOG));
    }

    /**
     * The method which deletes from the registry a specific entry.
     *
     * @param key The key of the entry to be deleted.
     */
    public void deleteFromLinkInfoRegistry(String key) {
        LOG.debug("Removing from Link Info registry key {}.", key);
        WriteTransaction transaction = db.newReadWriteTransaction();
        InstanceIdentifier<LinkInfoRegistryEntry> iid = toInstanceIdentifier(key);

        transaction.delete(LogicalDatastoreType.OPERATIONAL, iid);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<Void>("Failed to delete from Link Info registry", LOG));
    }

    /**
     * The method which maps an entry to an instance identifier.
     *
     * @param input The entry to be mapped to the relevant instance identifier.
     * @see LinkInfoRegistryEntry
     */
    private InstanceIdentifier<LinkInfoRegistryEntry> toInstanceIdentifier(LinkInfoRegistryEntry input) {
        InstanceIdentifier<LinkInfoRegistryEntry> iid = InstanceIdentifier.create(LinkInfoRegistry.class)
                .child(LinkInfoRegistryEntry.class,
                        new LinkInfoRegistryEntryKey(input.getLid()));
        return iid;
    }

    /**
     * The method which maps the key of an entry to an instance identifier.
     *
     * @param key The key of the entry to be mapped to the relevant instance identifier.
     * @see LinkInfoRegistryEntry
     */
    private InstanceIdentifier<LinkInfoRegistryEntry> toInstanceIdentifier(String key) {
        InstanceIdentifier<LinkInfoRegistryEntry> iid = InstanceIdentifier.create(LinkInfoRegistry.class)
                .child(LinkInfoRegistryEntry.class,
                        new LinkInfoRegistryEntryKey(key));
        return iid;
    }

    /**
     * The method which reads from the registry an entry with a specific key.
     *
     * @param key The key of the entry to be fetched.
     * @return The relevant entry to be fetched.
     * @see LinkInfoRegistryEntry
     */
    public LinkInfoRegistryEntry readFromLinkInfoRegistry(String key) {
        LOG.debug("Reading from Link registry for key {}.", key);
        LinkInfoRegistryEntry linkInfo = null;
        ReadOnlyTransaction transaction = db.newReadOnlyTransaction();
        InstanceIdentifier<LinkInfoRegistryEntry> iid = toInstanceIdentifier(key);
        CheckedFuture<Optional<LinkInfoRegistryEntry>, ReadFailedException> future =
                transaction.read(LogicalDatastoreType.OPERATIONAL, iid);
        Optional<LinkInfoRegistryEntry> optional = Optional.absent();
        try {
            optional = future.checkedGet();
        } catch (ReadFailedException e) {
            LOG.error("Reading Link failed:",e);
        }
        if(optional.isPresent()) {
            linkInfo = optional.get();
        }
        return linkInfo;
    }

    /**
     * The method which reads all the entries from the registry.
     *
     * @return The list of all the entries.
     * @see LinkInfoRegistryEntry
     */
    public List<LinkInfoRegistryEntry> readAllFromLinkInfoRegistry(){
        List<LinkInfoRegistryEntry> linkInfoRegistryList = new ArrayList<>();
        try {
            InstanceIdentifier<LinkInfoRegistry> linkInfoRegistryId = InstanceIdentifier.builder(
                    LinkInfoRegistry.class)
                    .build();
            ReadOnlyTransaction linkInfoRegistryTransaction = db.newReadOnlyTransaction();
            CheckedFuture<Optional<LinkInfoRegistry>, ReadFailedException> linkInfoRegistryFuture = linkInfoRegistryTransaction
                    .read(LogicalDatastoreType.OPERATIONAL, linkInfoRegistryId);
            Optional<LinkInfoRegistry> linkInfoRegistryOptional = Optional.absent();
            linkInfoRegistryOptional = linkInfoRegistryFuture.checkedGet();


            if (linkInfoRegistryOptional != null && linkInfoRegistryOptional.isPresent()) {
                linkInfoRegistryList = linkInfoRegistryOptional.get().getLinkInfoRegistryEntry();
            }

            if (linkInfoRegistryList != null) {
                for (LinkInfoRegistryEntry entry : linkInfoRegistryList) {
                    LOG.info("FR Entry " + entry);
                }
            }
        }
        catch (Exception e) {
            LOG.info("Failed to read all from the Flow Registry.");
        }
        return linkInfoRegistryList;
    }
}
