/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.bootstrapping.impl;

import eu.point.bootstrapping.impl.icn.IcnIdConfigurator;
import eu.point.bootstrapping.impl.switches.ICNPacketListener;
import eu.point.bootstrapping.impl.topology.NetworkGraphImpl;
import eu.point.bootstrapping.impl.topology.TopologyListener;
import eu.point.registry.impl.*;
import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.md.sal.common.api.data.AsyncDataBroker;
import org.opendaylight.controller.md.sal.common.api.data.LogicalDatastoreType;
import org.opendaylight.controller.sal.binding.api.BindingAwareBroker;
import org.opendaylight.controller.sal.binding.api.NotificationProviderService;
import org.opendaylight.controller.sal.binding.api.RpcProviderRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.bootstrapping.rev150722.BootstrappingService;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.service.rev130819.SalFlowService;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.Nodes;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.node.NodeConnector;
import org.opendaylight.yang.gen.v1.urn.opendaylight.packet.service.rev130709.PacketProcessingService;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.NetworkTopology;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.TopologyId;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.Topology;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.TopologyKey;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.topology.Link;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * The bootstrapping application class.
 *
 * @author George Petropoulos, Marievi Xezonaki
 * @version cycle-2
 */
public class BootstrappingApplicationImpl implements AutoCloseable {

    private static final Logger LOG = LoggerFactory.getLogger(BootstrappingApplicationImpl.class);
    private DataBroker dataBrokerService;
    private RpcProviderRegistry rpcProviderRegistry;
    private NotificationProviderService notificationService;
    private BindingAwareBroker.RpcRegistration<BootstrappingService> bootstrappingService;

    /**
     * The constructor of the bootstrapping application.
     * It initializes the registries, the network graph handling and the topology listener.
     *
     * @param dataBrokerService The data broker service to be used.
     * @param rpcProviderRegistry The rpc provider service to be used.
     * @param notificationService The notification service to be used.
     */
    public BootstrappingApplicationImpl(DataBroker dataBrokerService,
                                        RpcProviderRegistry rpcProviderRegistry,
                                        NotificationProviderService notificationService) {
        LOG.info("Initializing Bootstrapping Application.");
        this.dataBrokerService = dataBrokerService;
        this.rpcProviderRegistry = rpcProviderRegistry;
        SalFlowService salFlowService = rpcProviderRegistry.getRpcService(SalFlowService.class);
        PacketProcessingService packetProcessingService = rpcProviderRegistry.
                getRpcService(PacketProcessingService.class);
        this.notificationService = notificationService;

        //Create registries for node and link ids
        NodeRegistryUtils.getInstance().setDb(this.dataBrokerService);
        LinkRegistryUtils.getInstance().setDb(this.dataBrokerService);
        NodeConnectorRegistryUtils.getInstance().setDb(this.dataBrokerService);
        FlowRegistryUtils.getInstance().setDb(this.dataBrokerService);
        LinkInfoRegistryUtils.getInstance().setDb(this.dataBrokerService);
        GroupsRegistryUtils.getInstance().setDb(this.dataBrokerService);

        //register packetprocessing listener
        this.notificationService.registerNotificationListener(new ICNPacketListener(packetProcessingService));

        //initialize network graph
        NetworkGraphImpl.getInstance().setDb(dataBrokerService);
        NetworkGraphImpl.getInstance().init();

        //initialize icn id configurator
        IcnIdConfigurator.getInstance().setDb(this.dataBrokerService);

        //register topology listener
        InstanceIdentifier<Link> linkInstance = InstanceIdentifier.builder(NetworkTopology.class)
                .child(Topology.class, new TopologyKey(new TopologyId("flow:1")))
                .child(Link.class)
                .build();
        this.dataBrokerService.registerDataChangeListener(LogicalDatastoreType.OPERATIONAL,
                linkInstance, new TopologyListener(), AsyncDataBroker.DataChangeScope.BASE);

        //register implementation of bootstrapping api
        this.bootstrappingService = rpcProviderRegistry.addRpcImplementation(BootstrappingService.class,
                new BootstrappingServiceImpl(this.dataBrokerService));

    }

    @Override
    public void close() throws Exception {
        LOG.info("Closing Bootstrapping Application.");
    }


}
