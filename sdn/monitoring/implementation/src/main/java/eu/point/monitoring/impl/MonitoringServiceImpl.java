/*
 * Copyright © 2017 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.monitoring.impl;

import java.util.Timer;
import java.util.concurrent.Future;

import com.google.common.util.concurrent.Futures;
import eu.point.monitoring.impl.lsn.LinkStatusListener;
import eu.point.monitoring.impl.statistics.FlowMonitorTool;
import eu.point.monitoring.impl.statistics.GroupMonitorTool;
import eu.point.monitoring.impl.statistics.TrafficMonitorTool;
import org.opendaylight.controller.md.sal.binding.api.DataChangeListener;
import org.opendaylight.controller.md.sal.common.api.data.AsyncDataBroker.DataChangeScope;
import org.opendaylight.yang.gen.v1.urn.eu.point.monitoring.rev150722.InitInput;
import org.opendaylight.yang.gen.v1.urn.eu.point.monitoring.rev150722.MonitoringService;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.node.NodeConnector;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.Node;
import org.opendaylight.yangtools.concepts.ListenerRegistration;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.md.sal.common.api.data.LogicalDatastoreType;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.Nodes;
import org.opendaylight.yangtools.yang.common.RpcResult;
import org.opendaylight.yangtools.yang.common.RpcResultBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * The class which implements the REST API for monitoring module.
 *
 * @author George Petropoulos
 * @version cycle-2
 */
public class MonitoringServiceImpl implements MonitoringService, AutoCloseable {

    private DataBroker db;
    private Timer trafficTimer;
    private Timer flowTimer;
    private Timer groupTimer;
    private static final Logger LOG = LoggerFactory.getLogger(MonitoringServiceImpl.class);
    private ListenerRegistration<DataChangeListener> listenerRegistration;

    /**
     * The constructor class.
     */
    public MonitoringServiceImpl(DataBroker db) {
        this.db = db;
        this.trafficTimer = new Timer();
        this.flowTimer = new Timer();
        this.groupTimer = new Timer();

    }

    /**
     * The method which implements the initalization function of the rest api.
     *
     * @param input The input with all the parameters to initialize the monitoring application.
     * @return Void future result.
     */
    public Future<RpcResult<Void>> init(InitInput input) {


        //check traffic monitor enable/disable mode
        if (input.isTrafficmonitorEnabled()) {
            LOG.info("Enabling traffic, flow and group monitors.");
            //schedule a timer for traffic monitoring
            try {
                trafficTimer = new Timer();
                trafficTimer.schedule(new TrafficMonitorTool(this.db), 0, input.getTrafficmonitorPeriod());

                flowTimer = new Timer();
                flowTimer.schedule(new FlowMonitorTool(this.db), 5000, input.getTrafficmonitorPeriod());

                groupTimer = new Timer();
                groupTimer.schedule(new GroupMonitorTool(this.db), 10000, input.getTrafficmonitorPeriod());

            } catch (Exception e) {
                LOG.error("Error starting traffic, flow and group monitors.");
            }
        }
        else {
            LOG.info("Disabling traffic, flow and group monitors.");
            trafficTimer.cancel();
            flowTimer.cancel();
            groupTimer.cancel();
        }

        //check link monitor enable/disable mode
        if (input.isLinkmonitorEnabled()) {
            LOG.info("Enabling link status monitor.");
            InstanceIdentifier<NodeConnector> niid = InstanceIdentifier.builder(Nodes.class)
                    .child(Node.class)
                    .child(NodeConnector.class).build();
            //register a listener for link monitoring
            listenerRegistration = this.db.registerDataChangeListener(LogicalDatastoreType.OPERATIONAL,
                    niid, new LinkStatusListener(), DataChangeScope.SUBTREE);
        }
        else {
            LOG.info("Disabling link state monitor.");
            listenerRegistration.close();
        }


        return Futures.immediateFuture(RpcResultBuilder.<Void>success().build());
    }

    @Override
    public void close() throws Exception {
        LOG.info("Closing monitoring module.");
    }
}
