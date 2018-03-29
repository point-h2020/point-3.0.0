/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.monitoring.impl;

import eu.point.registry.impl.LinkRegistryUtils;
import eu.point.registry.impl.NodeConnectorRegistryUtils;
import eu.point.registry.impl.NodeRegistryUtils;
import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.sal.binding.api.BindingAwareBroker;
import org.opendaylight.controller.sal.binding.api.NotificationProviderService;
import org.opendaylight.controller.sal.binding.api.RpcProviderRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.monitoring.rev150722.MonitoringService;

public class MonitoringModule extends eu.point.monitoring.impl.AbstractMonitoringModule {
    public MonitoringModule(org.opendaylight.controller.config.api.ModuleIdentifier identifier, org.opendaylight.controller.config.api.DependencyResolver dependencyResolver) {
        super(identifier, dependencyResolver);
    }

    public MonitoringModule(org.opendaylight.controller.config.api.ModuleIdentifier identifier, org.opendaylight.controller.config.api.DependencyResolver dependencyResolver, eu.point.monitoring.impl.MonitoringModule oldModule, java.lang.AutoCloseable oldInstance) {
        super(identifier, dependencyResolver, oldModule, oldInstance);
    }

    @Override
    public void customValidation() {
        // add custom validation form module attributes here.
    }

    @Override
    public java.lang.AutoCloseable createInstance() {
        DataBroker dataBrokerService = getDataBrokerDependency();
        RpcProviderRegistry rpcProviderRegistry = getRpcRegistryDependency();
        NotificationProviderService notificationService = getNotificationServiceDependency();
        MonitoringServiceImpl monitoringServiceImpl = new MonitoringServiceImpl(dataBrokerService);

        //Create registries for node and link ids
        NodeRegistryUtils.getInstance().setDb(dataBrokerService);
        LinkRegistryUtils.getInstance().setDb(dataBrokerService);
        NodeConnectorRegistryUtils.getInstance().setDb(dataBrokerService);

        BindingAwareBroker.RpcRegistration<MonitoringService> monitoringService =
                rpcProviderRegistry.addRpcImplementation(MonitoringService.class,
                        monitoringServiceImpl);
        return monitoringServiceImpl;
    }

}
