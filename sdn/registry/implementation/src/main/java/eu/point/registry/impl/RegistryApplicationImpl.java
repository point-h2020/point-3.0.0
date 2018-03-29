/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.registry.impl;

import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.sal.binding.api.NotificationProviderService;
import org.opendaylight.controller.sal.binding.api.RpcProviderRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.GroupsRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.LinkInfoRegistry;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.service.rev130819.SalFlowService;
import org.opendaylight.yang.gen.v1.urn.opendaylight.packet.service.rev130709.PacketProcessingService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * The registry application class.
 *
 * @author George Petropoulos
 * @version cycle-2
 */
public class RegistryApplicationImpl implements AutoCloseable {

    private static final Logger LOG = LoggerFactory.getLogger(RegistryApplicationImpl.class);
    private DataBroker dataBrokerService;
    private RpcProviderRegistry rpcProviderRegistry;
    private NotificationProviderService notificationService;

    /**
     * The constructor of the registry application.
     * It initializes all the registries.
     *
     * @param dataBrokerService The data broker service to be used.
     * @param rpcProviderRegistry The rpc provider service to be used.
     * @param notificationService The notification service to be used.
     */
    public RegistryApplicationImpl(DataBroker dataBrokerService,
                                        RpcProviderRegistry rpcProviderRegistry,
                                        NotificationProviderService notificationService) {
        LOG.info("Initializing Registry Application.");
        this.dataBrokerService = dataBrokerService;
        this.rpcProviderRegistry = rpcProviderRegistry;
        this.notificationService = notificationService;

        //Create registries for node and link ids
        NodeRegistryUtils.getInstance().setDb(this.dataBrokerService);
        NodeRegistryUtils.getInstance().initializeDataTree();

        LinkRegistryUtils.getInstance().setDb(this.dataBrokerService);
        LinkRegistryUtils.getInstance().initializeDataTree();

        NodeConnectorRegistryUtils.getInstance().setDb(this.dataBrokerService);
        NodeConnectorRegistryUtils.getInstance().initializeDataTree();

        FlowRegistryUtils.getInstance().setDb(this.dataBrokerService);
        FlowRegistryUtils.getInstance().initializeDataTree();

        LinkInfoRegistryUtils.getInstance().setDb(this.dataBrokerService);
        LinkInfoRegistryUtils.getInstance().initializeDataTree();

        GroupsRegistryUtils.getInstance().setDb(this.dataBrokerService);
        GroupsRegistryUtils.getInstance().initializeDataTree();
    }

    @Override
    public void close() throws Exception {
        LOG.info("Closing Registry Application.");
    }


}
