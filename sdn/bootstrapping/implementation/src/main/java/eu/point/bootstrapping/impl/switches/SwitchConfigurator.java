/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.bootstrapping.impl.switches;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;
import com.google.common.util.concurrent.*;
import eu.point.bootstrapping.impl.BootstrappingServiceImpl;
import eu.point.bootstrapping.impl.icn.IcnIdConfigurator;
import eu.point.registry.impl.FlowRegistryUtils;
import eu.point.registry.impl.GroupsRegistryUtils;
import eu.point.registry.impl.LoggingFuturesCallBack;
import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.md.sal.binding.api.WriteTransaction;
import org.opendaylight.controller.md.sal.common.api.data.LogicalDatastoreType;
import org.opendaylight.controller.md.sal.common.api.data.TransactionCommitFailedException;
import org.opendaylight.openflowplugin.api.OFConstants;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.flow.registry.FlowRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.groups.registry.GroupsRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.ietf.params.xml.ns.yang.ietf.inet.types.rev130715.Ipv6Address;
import org.opendaylight.yang.gen.v1.urn.ietf.params.xml.ns.yang.ietf.inet.types.rev130715.Uri;
import org.opendaylight.yang.gen.v1.urn.ietf.params.xml.ns.yang.ietf.yang.types.rev130715.MacAddress;
import org.opendaylight.yang.gen.v1.urn.opendaylight.action.types.rev131112.action.action.GroupActionCaseBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.action.types.rev131112.action.action.OutputActionCaseBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.action.types.rev131112.action.action.group.action._case.GroupActionBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.action.types.rev131112.action.action.output.action._case.OutputActionBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.action.types.rev131112.action.list.Action;
import org.opendaylight.yang.gen.v1.urn.opendaylight.action.types.rev131112.action.list.ActionBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.FlowCapableNode;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.FlowId;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.Table;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.TableKey;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.table.Flow;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.table.FlowBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.inventory.rev130819.tables.table.FlowKey;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.service.rev130819.*;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.FlowCookie;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.FlowModFlags;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.OutputPortValues;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.flow.Instructions;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.flow.InstructionsBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.flow.Match;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.flow.MatchBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.instruction.instruction.ApplyActionsCaseBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.instruction.instruction.GoToTableCaseBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.instruction.instruction.apply.actions._case.ApplyActions;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.instruction.instruction.apply.actions._case.ApplyActionsBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.instruction.instruction.go.to.table._case.GoToTableBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.instruction.list.InstructionBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.types.rev131026.instruction.list.Instruction;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.*;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.group.Buckets;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.GroupId;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.groups.Group;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.group.BucketsBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.group.buckets.Bucket;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.group.buckets.BucketBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.group.buckets.BucketKey;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.groups.GroupBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.group.types.rev131018.groups.GroupKey;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.NodeId;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.NodeRef;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.Nodes;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.Node;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.NodeKey;
import org.opendaylight.yang.gen.v1.urn.opendaylight.l2.types.rev130827.EtherType;
import org.opendaylight.yang.gen.v1.urn.opendaylight.model.match.types.rev131026.ethernet.match.fields.EthernetDestinationBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.model.match.types.rev131026.ethernet.match.fields.EthernetTypeBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.model.match.types.rev131026.match.EthernetMatchBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.model.match.types.rev131026.match.layer._3.match.Ipv6MatchArbitraryBitMaskBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.opendaylight.ipv6.arbitrary.bitmask.fields.rev160224.Ipv6ArbitraryMask;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.topology.Link;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.opendaylight.yangtools.yang.common.RpcResult;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.common.collect.ImmutableList;

/**
 * The switch configurator class.
 *
 * @author George Petropoulos, Marievi Xezonaki
 * @version cycle-2
 */
public class SwitchConfigurator {

    private static final Logger LOG = LoggerFactory.getLogger(SwitchConfigurator.class);

    private SalFlowService salFlowService;

    private DataBroker db;

    private static int num = 0;

    private static Long groupId = 0L;
    private static Long bucketId = 0L;
    private static HashMap<String, Integer> groupsHashMap = new HashMap();

    /**
     * The constructor class.
     */
    public SwitchConfigurator(SalFlowService salFlowService) {
        this.salFlowService = salFlowService;
    }

    /**
     * The constructor class.
     */
    public SwitchConfigurator(DataBroker db) {
        this.db = db;
    }


    private AtomicLong flowCookieInc = new AtomicLong(0x2a00000000000000L);

    /**
     * The method which installs a flow to the switch, through the sal flow service.
     *
     * @param edge_switch       The switch to be configured.
     * @param edgeNodeConnector The node connector to be configured.
     * @param srcAddress        The source IPv6 address to be added to the arbitrary bitmask rule.
     * @param dstAddress        The destination IPv6 address to be added to the arbitrary bitmask rule.
     */
    public void addFlow(String edge_switch, String edgeNodeConnector, String srcAddress, String dstAddress) {

        LOG.debug("Start executing RPC");

        // create the flow
        Flow createdFlow = createAddedFlow(edgeNodeConnector, srcAddress, dstAddress);

        // build instance identifier for flow
        InstanceIdentifier<Flow> flowPath = InstanceIdentifier
                .builder(Nodes.class)
                .child(Node.class, new NodeKey(new NodeId(edge_switch)))
                .augmentation(FlowCapableNode.class)
                .child(Table.class, new TableKey(createdFlow.getTableId()))
                .child(Flow.class, new FlowKey(createdFlow.getId())).build();


        final AddFlowInputBuilder builder = new AddFlowInputBuilder(createdFlow);
        final InstanceIdentifier<Table> tableInstanceId = flowPath
                .<Table>firstIdentifierOf(Table.class);
        final InstanceIdentifier<Node> nodeInstanceId = flowPath
                .<Node>firstIdentifierOf(Node.class);
        builder.setNode(new NodeRef(nodeInstanceId));
        builder.setFlowTable(new FlowTableRef(tableInstanceId));
        builder.setTransactionUri(new Uri(createdFlow.getId().getValue()));
        final AddFlowInput flow = builder.build();

        LOG.debug("onPacketReceived - About to write flow (via SalFlowService) {}", flow);
        // add flow to sal
        ListenableFuture<RpcResult<AddFlowOutput>> result = JdkFutureAdapters
                .listenInPoolThread(salFlowService.addFlow(flow));
        Futures.addCallback(result, new FutureCallback<RpcResult<AddFlowOutput>>() {
            @Override
            public void onSuccess(final RpcResult<AddFlowOutput> o) {
                LOG.debug("Successful outcome.");
            }

            @Override
            public void onFailure(final Throwable throwable) {
                LOG.debug("Failure.");
                throwable.printStackTrace();
            }
        });
    }


    /**
     * The method which installs a flow to the switch, through the configuration datastore of the controller.
     *
     * @param edge_switch       The switch to be configured.
     * @param edgeNodeConnector The node connector to be configured.
     * @param srcAddress        The source IPv6 address to be added to the arbitrary bitmask rule.
     * @param dstAddress        The destination IPv6 address to be added to the arbitrary bitmask rule.
     */
    public void addFlowConfig(String edge_switch, String dstNodeId, String edgeNodeConnector, String srcAddress, String dstAddress) {

        LOG.debug("Adding flow to config datastore.");

        Flow createdFlow;
        WriteTransaction transaction = db.newWriteOnlyTransaction();

        //if groups option is on, the appropriate group will be created
        if (BootstrappingServiceImpl.groupsFlag) {
            if (!dstNodeId.contains("host")) {
                if (!groupsHashMap.containsKey(edgeNodeConnector)) {

                    LOG.info("Adding to registry for " + edgeNodeConnector + " group id " + groupId);
                    Integer port = findAlternativePort(edgeNodeConnector);
                    if (port != -1) {
                        GroupsRegistryUtils.getInstance().writeToGroupRegistry(
                                new GroupsRegistryEntryBuilder()
                                        .setEdgeNodeConnector(edgeNodeConnector)
                                        .setGroupId(groupId.intValue())
//                                    .setGroupId(Integer.parseInt(edgeNodeConnector.split(":")[1].concat(edgeNodeConnector.split(":")[2])))
                                        .build());
                    }


                    //create the group only for switches inside the domain
                    Group createdGroup = createGroup(edgeNodeConnector);
                    // build instance identifier for group
                    InstanceIdentifier<Group> groupPath = InstanceIdentifier
                            .builder(Nodes.class)
                            .child(Node.class, new NodeKey(new NodeId(edge_switch)))
                            .augmentation(FlowCapableNode.class)
                            .child(Group.class, new GroupKey(createdGroup.getGroupId())).build();
                    transaction.put(LogicalDatastoreType.CONFIGURATION, groupPath, createdGroup, true);
                }
            }
            createdFlow = createAddedFlowGroups(edgeNodeConnector, srcAddress, dstAddress);

        }
        else {
            // create the flow
            createdFlow = createAddedFlow(edgeNodeConnector, srcAddress, dstAddress);
        }

        // build instance identifier for flow
        InstanceIdentifier<Flow> flowPath = InstanceIdentifier
                .builder(Nodes.class)
                .child(Node.class, new NodeKey(new NodeId(edge_switch)))
                .augmentation(FlowCapableNode.class)
                .child(Table.class, new TableKey(createdFlow.getTableId()))
                .child(Flow.class, new FlowKey(createdFlow.getId())).build();

        transaction.put(LogicalDatastoreType.CONFIGURATION, flowPath, createdFlow, true);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<>("Failed to add flow.", LOG));
        LOG.debug("Added flow {} to config datastore.", createdFlow);
    }

    /**
     * The method which prepares the flow to be added to the switch.
     *
     * @param edgeNodeConnector The node connector to be configured.
     * @param srcAddress        The source IPv6 address to be added to the arbitrary bitmask rule.
     * @param dstAddress        The destination IPv6 address to be added to the arbitrary bitmask rule.
     * @return The flow to be added.
     */
    private Flow createAddedFlow(String edgeNodeConnector, String srcAddress, String dstAddress) {

        LOG.info("Creating flow for " + edgeNodeConnector);
        FlowBuilder flowBuilder = new FlowBuilder() //
                .setTableId((short) 0) //
                .setFlowName(edgeNodeConnector);

        flowBuilder.setId(new FlowId(edgeNodeConnector));

        Match match = new MatchBuilder()
                .setEthernetMatch(new EthernetMatchBuilder()
                        .setEthernetType(new EthernetTypeBuilder()
                                .setType(new EtherType(0x86DDL))
                                .build())
                        .setEthernetDestination(new EthernetDestinationBuilder()
                                .setAddress(new MacAddress("00:00:00:00:00:00"))
                                .build())
                        .build())
                .setLayer3Match(new Ipv6MatchArbitraryBitMaskBuilder()
                        .setIpv6SourceAddressNoMask(new Ipv6Address(srcAddress))
                        .setIpv6SourceArbitraryBitmask(new Ipv6ArbitraryMask(srcAddress))
                        .setIpv6DestinationAddressNoMask(new Ipv6Address(dstAddress))
                        .setIpv6DestinationArbitraryBitmask(new Ipv6ArbitraryMask(dstAddress))
                        .build())
                .build();


        ActionBuilder actionBuilder = new ActionBuilder();
        List<Action> actions = new ArrayList<Action>();
        //Actions
        Action outputNodeConnectorAction = actionBuilder
                .setOrder(0).setAction(new OutputActionCaseBuilder()
                        .setOutputAction(new OutputActionBuilder()
                                .setOutputNodeConnector(new Uri(edgeNodeConnector.split(":")[2]))
                                .build())
                        .build())
                .build();
        actions.add(outputNodeConnectorAction);

        //ApplyActions
        ApplyActions applyActions = new ApplyActionsBuilder().setAction(actions).build();

        //Instruction
        Instruction applyActionsInstruction = new InstructionBuilder() //
                .setOrder(0).setInstruction(new ApplyActionsCaseBuilder()//
                        .setApplyActions(applyActions) //
                        .build()) //
                .build();

        Instructions applyInstructions = new InstructionsBuilder()
                .setInstruction(ImmutableList.of(applyActionsInstruction))
                .build();

        // Put our Instruction in a list of Instructions
        flowBuilder
                .setMatch(match)
                .setBufferId(OFConstants.OFP_NO_BUFFER)
                .setInstructions(applyInstructions)
                .setPriority(1000)
                .setCookie(new FlowCookie(BigInteger.valueOf(flowCookieInc.getAndIncrement())))
                .setFlags(new FlowModFlags(false, false, false, false, false));

        return flowBuilder.build();
    }

    /**
     * The method which prepares a flow for groups to be added to the switch.
     *
     * @param edgeNodeConnector The node connector to be configured.
     * @param srcAddress        The source IPv6 address to be added to the arbitrary bitmask rule.
     * @param dstAddress        The destination IPv6 address to be added to the arbitrary bitmask rule.
     * @return The flow to be added.
     */
    private Flow createAddedFlowGroups(String edgeNodeConnector, String srcAddress, String dstAddress) {

        LOG.info("Creating flow for " + edgeNodeConnector);
        FlowBuilder flowBuilder = new FlowBuilder() //
                .setTableId((short) 0) //
                .setFlowName(edgeNodeConnector);

        flowBuilder.setId(new FlowId(edgeNodeConnector));

        Match match = new MatchBuilder()
                .setEthernetMatch(new EthernetMatchBuilder()
                        .setEthernetType(new EthernetTypeBuilder()
                                .setType(new EtherType(0x86DDL))
                                .build())
                        .setEthernetDestination(new EthernetDestinationBuilder()
                                .setAddress(new MacAddress("00:00:00:00:00:00"))
                                .build())
                        .build())
                .setLayer3Match(new Ipv6MatchArbitraryBitMaskBuilder()
                        .setIpv6SourceAddressNoMask(new Ipv6Address(srcAddress))
                        .setIpv6SourceArbitraryBitmask(new Ipv6ArbitraryMask(srcAddress))
                        .setIpv6DestinationAddressNoMask(new Ipv6Address(dstAddress))
                        .setIpv6DestinationArbitraryBitmask(new Ipv6ArbitraryMask(dstAddress))
                        .build())
                .build();

        ActionBuilder actionBuilder = new ActionBuilder();
        List<Action> actions = new ArrayList<>();

        //Actions
        Integer group = groupsHashMap.get(edgeNodeConnector);
        if (group != null) {
            //the link is not connected to host, so group is inserted as action
            Action groupAction = actionBuilder
                    .setOrder(0).setAction(new GroupActionCaseBuilder()
                            .setGroupAction(new GroupActionBuilder()
                                    .setGroupId(group.longValue())
                                    .build())
                            .build())
                    .build();
            actions.add(groupAction);
        }
        else{
            //means that link is connected to host, so it has no failover group
            //output is inserted as action instead of group
            Action outputNodeConnectorAction = actionBuilder
                    .setOrder(1).setAction(new OutputActionCaseBuilder()
                            .setOutputAction(new OutputActionBuilder()
                                    .setOutputNodeConnector(new Uri(edgeNodeConnector.split(":")[2]))
                                    .build())
                            .build())
                    .build();
            actions.add(outputNodeConnectorAction);
        }

        //ApplyActions
        ApplyActions applyActions = new ApplyActionsBuilder().setAction(actions).build();

        //Instruction
        Instruction applyActionsInstruction = new InstructionBuilder() //
                .setOrder(0).setInstruction(new ApplyActionsCaseBuilder()//
                        .setApplyActions(applyActions) //
                        .build()) //
                .build();

        Instructions applyInstructions = new InstructionsBuilder()
                .setInstruction(ImmutableList.of(applyActionsInstruction))
                .build();

        // Put our Instruction in a list of Instructions
        flowBuilder
                .setMatch(match)
                .setBufferId(OFConstants.OFP_NO_BUFFER)
                .setInstructions(applyInstructions)
                .setPriority(1000)
                .setCookie(new FlowCookie(BigInteger.valueOf(flowCookieInc.getAndIncrement())))
                .setFlags(new FlowModFlags(false, false, false, false, false));

        return flowBuilder.build();
    }

    /**
     * The method which deletes a flow from the switch, through the sal flow service.
     *
     * @param edge_switch       The switch to be configured.
     * @param edgeNodeConnector The node connector to be configured.
     * @param srcAddress        The source IPv6 address to be added to the arbitrary bitmask rule.
     * @param dstAddress        The destination IPv6 address to be added to the arbitrary bitmask rule.
     */
    public void deleteFlow(String edge_switch, String edgeNodeConnector, String srcAddress, String dstAddress) {

        LOG.debug("Start executing RPC");

        // create the flow
        Flow removedFlow = createDeletedFlow(edgeNodeConnector, srcAddress, dstAddress);

        // build instance identifier for flow
        InstanceIdentifier<Flow> flowPath = InstanceIdentifier
                .builder(Nodes.class)
                .child(Node.class, new NodeKey(new NodeId(edge_switch)))
                .augmentation(FlowCapableNode.class)
                .child(Table.class, new TableKey(removedFlow.getTableId()))
                .child(Flow.class, new FlowKey(removedFlow.getId())).build();


        final RemoveFlowInputBuilder builder = new RemoveFlowInputBuilder(removedFlow);
        final InstanceIdentifier<Table> tableInstanceId = flowPath
                .<Table>firstIdentifierOf(Table.class);
        final InstanceIdentifier<Node> nodeInstanceId = flowPath
                .<Node>firstIdentifierOf(Node.class);
        builder.setNode(new NodeRef(nodeInstanceId));
        builder.setFlowTable(new FlowTableRef(tableInstanceId));
        builder.setTransactionUri(new Uri(removedFlow.getId().getValue()));
        final RemoveFlowInput flow = builder.build();

        LOG.debug("onPacketReceived - About to remove flow (via SalFlowService) {}", flow);
        // add flow to sal
        ListenableFuture<RpcResult<RemoveFlowOutput>> result = JdkFutureAdapters
                .listenInPoolThread(salFlowService.removeFlow(flow));
        Futures.addCallback(result, new FutureCallback<RpcResult<RemoveFlowOutput>>() {
            @Override
            public void onSuccess(final RpcResult<RemoveFlowOutput> o) {
                LOG.debug("Successful outcome.");
            }

            @Override
            public void onFailure(final Throwable throwable) {
                LOG.debug("Failure.");
                throwable.printStackTrace();
            }
        });
    }


    /**
     * The method which deletes a flow from the switch, through the configuration datastore of the controller,
     * checking first in which table the flow is installed.
     *
     * @param edge_switch       The switch to be configured.
     * @param edgeNodeConnector The node connector to be configured.
     */
    public void deleteFlowConfig(String edge_switch, String edgeNodeConnector) {

        LOG.debug("Deleting flow from config datastore.");
        int table_num;

        //Remove the flow which will be deleted from the Flows registry
        if (FlowRegistryUtils.getInstance().readFromFlowRegistry(edgeNodeConnector) != null) {
            table_num = FlowRegistryUtils.getInstance().readFromFlowRegistry(edgeNodeConnector).getTableNum();
            LOG.info("Will remove flow from table " + table_num + " for switch " + edge_switch + " and edge conn" + edgeNodeConnector);

            updateTablesFreed(edge_switch, table_num, edgeNodeConnector);

            FlowRegistryUtils.getInstance().deleteFromFlowRegistry(edgeNodeConnector);

            // build instance identifier for flow
            InstanceIdentifier<Flow> flowPath = InstanceIdentifier
                    .builder(Nodes.class)
                    .child(Node.class, new NodeKey(new NodeId(edge_switch)))
                    .augmentation(FlowCapableNode.class)
                    .child(Table.class, new TableKey((short) table_num))
                    .child(Flow.class, new FlowKey(new FlowId(edgeNodeConnector))).build();

            WriteTransaction transaction = db.newWriteOnlyTransaction();
            transaction.delete(LogicalDatastoreType.CONFIGURATION, flowPath);
            CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
            Futures.addCallback(future,
                    new LoggingFuturesCallBack<>("Failed to delete flow", LOG));
            LOG.info("Deleted flow with id {} from config datastore.", new FlowId(edgeNodeConnector));
        }
        else{
            LOG.info("Not found table num.");
        }
    }


    /**
     * The method which marks a table as freed when a link is removed.
     *
     * @param edge_switch       The switch to be configured.
     * @param table_num         The number of the table which is being freed.
     * @param edgeNodeConnector The node connector to be configured.
     */
    private void updateTablesFreed(String edge_switch, Integer table_num, String edgeNodeConnector){

        Integer switch_no = Integer.parseInt(edge_switch.split(":")[1]);

        LOG.info("Adding free table " + table_num + " for switch " + switch_no);
        String tableFreed = edge_switch.split(":")[1] + "," + table_num + "," + edgeNodeConnector.split(":")[2];
        if (!IcnIdConfigurator.tablesFreed.contains(tableFreed))
            IcnIdConfigurator.tablesFreed.add(tableFreed);
    }

    /**
     * The method which deletes all the appropriate flows (including flows for combined outputs)
     * for a node connector, through the configuration datastore of the controller.
     *
     * @param edge_switch       The switch to be configured.
     * @param edgeNodeConnectors A list which will construct the node connector
     */
    public void deleteFlowsFromSwitches(String edge_switch, List<String> edgeNodeConnectors) {

        LOG.debug("Deleting flow from config datastore.");
        int table_num;

        WriteTransaction transaction = db.newWriteOnlyTransaction();
        for (String edgeNodeConnector : edgeNodeConnectors) {
            String nodeConnector = "";
            if (!edgeNodeConnector.contains("openflow")){
                String[] ports = edgeNodeConnector.split(":");
                for (int j = 1; j < ports.length; j++){
                    nodeConnector += ports[j];
                }
            }
            else{
                nodeConnector = edgeNodeConnector;
            }

            //Remove the flow which will be deleted from the Flows registry
            if (FlowRegistryUtils.getInstance().readFromFlowRegistry(edgeNodeConnector) != null) {
                table_num = FlowRegistryUtils.getInstance().readFromFlowRegistry(edgeNodeConnector).getTableNum();


                LOG.info("Will remove flow from table " + table_num + " for switch " + edge_switch + " and edge conn" + edgeNodeConnector);
                FlowRegistryUtils.getInstance().deleteFromFlowRegistry(edgeNodeConnector);

                // build instance identifier for flow
                InstanceIdentifier<Flow> flowPath = InstanceIdentifier
                        .builder(Nodes.class)
                        .child(Node.class, new NodeKey(new NodeId(edge_switch)))
                        .augmentation(FlowCapableNode.class)
                        .child(Table.class, new TableKey((short) table_num))
                        .child(Flow.class, new FlowKey(new FlowId(nodeConnector))).build();

                transaction.delete(LogicalDatastoreType.CONFIGURATION, flowPath);
                LOG.info("Deleted flow with id {} from config datastore.", new FlowId(nodeConnector));
            } else {
                LOG.info("Not found table num.");
            }
        }
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        LOG.info("Just deleted the flows.");
        Futures.addCallback(future,
                new LoggingFuturesCallBack<>("Failed to delete flow", LOG));
    }

    //create the flow to be deleted
    private Flow createDeletedFlow(String edgeNodeConnector, String srcAddress, String dstAddress) {

        FlowBuilder flowBuilder = new FlowBuilder() //
                .setTableId((short) 0) //
                .setFlowName(edgeNodeConnector);

        flowBuilder.setId(new FlowId(edgeNodeConnector));

        Match match = new MatchBuilder()
                .setEthernetMatch(new EthernetMatchBuilder()
                        .setEthernetType(new EthernetTypeBuilder()
                                .setType(new EtherType(0x86DDL))
                                .build())
                        .setEthernetDestination(new EthernetDestinationBuilder()
                                .setAddress(new MacAddress("00:00:00:00:00:00"))
                                .build())
                        .build())
                .setLayer3Match(new Ipv6MatchArbitraryBitMaskBuilder()
                        .setIpv6SourceAddressNoMask(new Ipv6Address(srcAddress))
                        .setIpv6SourceArbitraryBitmask(new Ipv6ArbitraryMask(srcAddress))
                        .setIpv6DestinationAddressNoMask(new Ipv6Address(dstAddress))
                        .setIpv6DestinationArbitraryBitmask(new Ipv6ArbitraryMask(dstAddress))
                        .build())
                .build();

        ActionBuilder actionBuilder = new ActionBuilder();
        List<Action> actions = new ArrayList<Action>();
        //Actions
        Action outputNodeConnectorAction = actionBuilder
                .setOrder(0).setAction(new OutputActionCaseBuilder()
                        .setOutputAction(new OutputActionBuilder()
                                .setOutputNodeConnector(new Uri(edgeNodeConnector.split(":")[2]))
                                .build())
                        .build())
                .build();
        actions.add(outputNodeConnectorAction);

        //ApplyActions
        ApplyActions applyActions = new ApplyActionsBuilder().setAction(actions).build();

        //Instruction
        Instruction applyActionsInstruction = new InstructionBuilder() //
                .setOrder(0).setInstruction(new ApplyActionsCaseBuilder()//
                        .setApplyActions(applyActions) //
                        .build()) //
                .build();

        Instructions applyInstructions = new InstructionsBuilder()
                .setInstruction(ImmutableList.of(applyActionsInstruction))
                .build();

        // Put our Instruction in a list of Instructions
        flowBuilder
                .setMatch(match)
                .setBufferId(OFConstants.OFP_NO_BUFFER)
                .setInstructions(applyInstructions)
                .setPriority(1000)
                .setCookie(new FlowCookie(BigInteger.valueOf(flowCookieInc.getAndIncrement())))
                .setFlags(new FlowModFlags(false, false, false, false, false));

        return flowBuilder.build();
    }


    /**
     * The method which installs a flow to the switch, through the configuration datastore of the controller,
     * in the specified switch table.
     *
     * @param edge_switch       The switch to be configured.
     * @param edgeNodeConnector The node connector to be configured.
     * @param srcAddress        The source IPv6 address to be added to the arbitrary bitmask rule.
     * @param dstAddress        The destination IPv6 address to be added to the arbitrary bitmask rule.
     * @param table_num         The table in which the flow will be installed.
     * @param tablesCount       The total number of tables in the specific switch.
     */
    public void addFlowConfigMultipleTables(String edge_switch, String dstNodeId, String edgeNodeConnector, String srcAddress, String dstAddress, Integer table_num, Integer tablesCount) {

        LOG.debug("Adding flow to config datastore.");
        Flow createdFlow;
        Flow defaultFlow;
        Group createdGroup;
        WriteTransaction transaction = db.newWriteOnlyTransaction();

        // create the flow
        if (BootstrappingServiceImpl.groupsFlag == true){
            if (!dstNodeId.contains("host")) {
                if (!groupsHashMap.containsKey(edgeNodeConnector)) {
                    Integer port = findAlternativePort(edgeNodeConnector);
                    if (port != -1) {
                        LOG.info("Adding to registry for " + edgeNodeConnector + " group id " + groupId);
                        GroupsRegistryUtils.getInstance().writeToGroupRegistry(
                                new GroupsRegistryEntryBuilder()
                                        .setEdgeNodeConnector(edgeNodeConnector)
                                        .setGroupId(groupId.intValue())
//                                    .setGroupId(Integer.parseInt(edgeNodeConnector.split(":")[1].concat(edgeNodeConnector.split(":")[2])))
                                        .build());
                    }
                    createdGroup = createGroup(edgeNodeConnector);

                    InstanceIdentifier<Group> groupPath = InstanceIdentifier
                            .builder(Nodes.class)
                            .child(Node.class, new NodeKey(new NodeId(edge_switch)))
                            .augmentation(FlowCapableNode.class)
                            .child(Group.class, new GroupKey(createdGroup.getGroupId())).build();
                    transaction.put(LogicalDatastoreType.CONFIGURATION, groupPath, createdGroup, true);
                }
            }
            createdFlow = createAddedFlowMultipleTablesGroups(edgeNodeConnector, srcAddress, dstAddress, table_num, tablesCount);
            defaultFlow = createDefaultFlowMultipleTables(edge_switch, edgeNodeConnector, table_num, tablesCount);

        }
        else {
            createdFlow = createAddedFlowMultipleTables(edgeNodeConnector, srcAddress, dstAddress, table_num, tablesCount);
            defaultFlow = createDefaultFlowMultipleTables(edge_switch, edgeNodeConnector, table_num, tablesCount);
        }

        // build instance identifier for flow
        InstanceIdentifier<Flow> flowPath = InstanceIdentifier
                .builder(Nodes.class)
                .child(Node.class, new NodeKey(new NodeId(edge_switch)))
                .augmentation(FlowCapableNode.class)
                .child(Table.class, new TableKey(createdFlow.getTableId()))
                .child(Flow.class, new FlowKey(createdFlow.getId())).build();

        InstanceIdentifier<Flow> defaultFlowPath = InstanceIdentifier
                .builder(Nodes.class)
                .child(Node.class, new NodeKey(new NodeId(edge_switch)))
                .augmentation(FlowCapableNode.class)
                .child(Table.class, new TableKey(defaultFlow.getTableId()))
                .child(Flow.class, new FlowKey(defaultFlow.getId())).build();

        transaction.put(LogicalDatastoreType.CONFIGURATION, flowPath, createdFlow, true);
        transaction.put(LogicalDatastoreType.CONFIGURATION, defaultFlowPath, defaultFlow, true);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<>("Failed to add flow.", LOG));
        LOG.debug("Added flow {} to config datastore.", createdFlow);
    }

    /**
     * The method which creates a group for fast failover, for a specific edge node connector.
     * The connector's output port will be the primary port, while a failover port will
     * be specified as alternative.
     *
     * @param edgeNodeConnector An edge node connector.
     * @return The group to be added.
     */
    private Group createGroup(String edgeNodeConnector){

   //     String groupId = edgeNodeConnector.split(":")[1].concat(edgeNodeConnector.split(":")[2]);
   //     GroupId id = new GroupId(Long.parseLong(groupId));

        GroupId id = new GroupId(groupId);
        String groupName = "group" + groupId;

        BucketId bId = new BucketId(bucketId);
        BucketKey bucketKey = new BucketKey(bId);
        List<Bucket> bucketList = new ArrayList<>();

        //Actions for Bucket
        ActionBuilder actionBuilder = new ActionBuilder();
        List<Action> actions = new ArrayList<>();
        Action outputNodeConnectorAction = actionBuilder
                .setOrder(0).setAction(new OutputActionCaseBuilder()
                        .setOutputAction(new OutputActionBuilder()
                                .setOutputNodeConnector(new Uri(edgeNodeConnector.split(":")[2]))
                                .build())
                        .build())
                .build();
        actions.add(outputNodeConnectorAction);

        //Bucket creation for current port
        Bucket bucket = new BucketBuilder()
                .setBucketId(bId)
                .setWatchPort(Long.parseLong(edgeNodeConnector.split(":")[2]))
                .setKey(bucketKey)
                .setAction(actions)
                .build();
        bucketList.add(bucket);
        bucketId++;

        Integer port = findAlternativePort(edgeNodeConnector);
        if (port != -1) {
            BucketId bIdFailover = new BucketId(bucketId);
            BucketKey bucketKeyFailover = new BucketKey(bIdFailover);

            //Actions for Failover Bucket
            ActionBuilder actionBuilderFailover = new ActionBuilder();
            List<Action> actionsFailover = new ArrayList<>();
            Action outputNodeConnectorActionFailover = actionBuilderFailover
                    .setOrder(1).setAction(new OutputActionCaseBuilder()
                            .setOutputAction(new OutputActionBuilder()
                                    .setOutputNodeConnector(new Uri(port.toString()))
                                    .build())
                            .build())
                    .build();
            actionsFailover.add(outputNodeConnectorActionFailover);

            //Bucket creation for failover port
            Long failoverPort = Long.parseLong(port.toString());
            Bucket bucketFailover = new BucketBuilder()
                    .setBucketId(bIdFailover)
                    .setWatchPort(failoverPort)
                    .setKey(bucketKeyFailover)
                    .setAction(actionsFailover)
                    .build();
            bucketList.add(bucketFailover);
            bucketId++;
        }

        Buckets buckets = new BucketsBuilder().setBucket(bucketList).build();

        Group group = new GroupBuilder().setGroupName(groupName)
                .setBarrier(false)
                .setGroupId(id)
                .setGroupType(GroupTypes.GroupFf)
                .setBuckets(buckets)
                .build();

        //Write group created to Group hash map
        if (!groupsHashMap.containsKey(edgeNodeConnector))
            groupsHashMap.put(edgeNodeConnector, groupId.intValue());

        groupId++;
        return group;
    }

    /**
     * The method which prepares the flow to be added to the switch in a specific table.
     *
     * @param edgeNodeConnector The node connector to be configured.
     * @param srcAddress        The source IPv6 address to be added to the arbitrary bitmask rule.
     * @param dstAddress        The destination IPv6 address to be added to the arbitrary bitmask rule.
     * @param table_num         The table in which the flow will be installed.
     * @param tablesCount       The total number of tables in the specific switch.
     * @return The flow to be added.
     */

    private Flow createAddedFlowMultipleTables(String edgeNodeConnector, String srcAddress, String dstAddress, Integer table_num, Integer tablesCount) {

        Short tableNum = Short.parseShort(table_num.toString());

        LOG.info("Test links for openflow:" + edgeNodeConnector.split(":")[1] + BootstrappingServiceImpl.tablesPerSwitch.get("openflow:".concat(edgeNodeConnector.split(":")[1])));
        Integer tables = BootstrappingServiceImpl.tablesPerSwitch.get("openflow:".concat(edgeNodeConnector.split(":")[1]));


        FlowBuilder flowBuilder = new FlowBuilder() //
                .setTableId((short) tableNum) //
                .setFlowName(edgeNodeConnector);

        flowBuilder.setId(new FlowId(edgeNodeConnector));

        Match match = new MatchBuilder()
                .setEthernetMatch(new EthernetMatchBuilder()
                        .setEthernetType(new EthernetTypeBuilder()
                                .setType(new EtherType(0x86DDL))
                                .build())
                        .setEthernetDestination(new EthernetDestinationBuilder()
                                .setAddress(new MacAddress("00:00:00:00:00:00"))
                                .build())
                        .build())
                .setLayer3Match(new Ipv6MatchArbitraryBitMaskBuilder()
                        .setIpv6SourceAddressNoMask(new Ipv6Address(srcAddress))
                        .setIpv6SourceArbitraryBitmask(new Ipv6ArbitraryMask(srcAddress))
                        .setIpv6DestinationAddressNoMask(new Ipv6Address(dstAddress))
                        .setIpv6DestinationArbitraryBitmask(new Ipv6ArbitraryMask(dstAddress))
                        .build())
                .build();

        ActionBuilder actionBuilder = new ActionBuilder();
        List<Action> actions = new ArrayList<Action>();
        List<Instruction> instructions = new ArrayList<>();

        //Actions
        Action outputNodeConnectorAction = actionBuilder
                .setOrder(0).setAction(new OutputActionCaseBuilder()
                        .setOutputAction(new OutputActionBuilder()
                                .setOutputNodeConnector(new Uri(edgeNodeConnector.split(":")[2]))
                                .build())
                        .build())
                .build();

        actions.add(outputNodeConnectorAction);

        //ApplyActions
        ApplyActions applyActions = new ApplyActionsBuilder().setAction(actions).build();

        //Instruction for Actions
        Instruction applyActionsInstruction = new InstructionBuilder()//
                .setOrder(0).setInstruction(new ApplyActionsCaseBuilder()//
                        .setApplyActions(applyActions) //
                        .build()) //
                .build();

        instructions.add(applyActionsInstruction);


        Integer tableId = table_num + 1;
        Short tableGoto = Short.parseShort(tableId.toString());

        if (tableGoto < tables){
//        if (tableGoto < tablesCount) {
            //Instruction for Goto
            GoToTableBuilder gttb = new GoToTableBuilder();
            gttb.setTableId(tableGoto);

            Instruction gotoInstruction = new InstructionBuilder()
                    .setOrder(1).setInstruction(new GoToTableCaseBuilder()
                            .setGoToTable(gttb.build())
                            .build())
                    .build();

            instructions.add(gotoInstruction);
        }

        //Build all the instructions together, based on the Instructions list
        Instructions allInstructions = new InstructionsBuilder()
                .setInstruction(instructions)
                .build();


        // Put our Instruction in a list of Instructions
        flowBuilder
                .setMatch(match)
                .setBufferId(OFConstants.OFP_NO_BUFFER)
                .setInstructions(allInstructions)
                .setPriority(1000)
                .setCookie(new FlowCookie(BigInteger.valueOf(flowCookieInc.getAndIncrement())))
                .setFlags(new FlowModFlags(false, false, false, false, false));

        return flowBuilder.build();
    }

    /**
     * The method which creates the default flow for a specific table of a switch.
     * The default flow contains only a goto (next table) statement.
     *
     * @param edge_switch The switch where the flow will be added.
     * @param edgeNodeConnector The edge node connector for which the flow will be added.
     * @param table_num The number of the table where the flow will be added.
     * @param tablesCount The total number of tables in the specific switch.
     * @return The flow to be added.
     */
    private Flow createDefaultFlowMultipleTables(String edge_switch, String edgeNodeConnector, Integer table_num, Integer tablesCount){
        Short tableNum = Short.parseShort(table_num.toString());

   //     LOG.info("Test links for openflow:" + edgeNodeConnector.split(":")[1] + BootstrappingServiceImpl.tablesPerSwitch.get("openflow:".concat(edgeNodeConnector.split(":")[1])));
        Integer tables = BootstrappingServiceImpl.tablesPerSwitch.get("openflow:".concat(edgeNodeConnector.split(":")[1]));


        String name = "Table" + num;
        num++;
        FlowBuilder flowBuilder = new FlowBuilder() //
                .setTableId((short) tableNum) //
                .setFlowName(name);

        flowBuilder.setId(new FlowId(name));

        Match match_default = new MatchBuilder()
                .setEthernetMatch(new EthernetMatchBuilder()
                        .setEthernetType(new EthernetTypeBuilder()
                                .setType(new EtherType(0x86DDL))
                                .build())
                        .setEthernetDestination(new EthernetDestinationBuilder()
                                .setAddress(new MacAddress("00:00:00:00:00:00"))
                                .build())
                        .build())
                .build();

        ActionBuilder actionBuilder = new ActionBuilder();
        List<Action> actions = new ArrayList<Action>();
        List<Instruction> instructions = new ArrayList<>();


        Integer tableId = table_num + 1;
        Short tableGoto = Short.parseShort(tableId.toString());

        if (tableGoto < tables){
       // if (tableGoto < tablesCount) {
            //Instruction for Goto
            GoToTableBuilder gttb = new GoToTableBuilder();
            gttb.setTableId(tableGoto);

            Instruction gotoInstruction = new InstructionBuilder()
                    .setOrder(1).setInstruction(new GoToTableCaseBuilder()
                            .setGoToTable(gttb.build())
                            .build())
                    .build();
            instructions.add(gotoInstruction);
        }
        else {
            Action outputNodeConnectorAction = actionBuilder
                    .setOrder(0).setAction(new OutputActionCaseBuilder()
                            .setOutputAction(new OutputActionBuilder()
                                    .setMaxLength(Integer.valueOf(0xffff))
                                    .setOutputNodeConnector(new Uri(OutputPortValues.CONTROLLER.toString()))
                                    .build())
                            .build())
                    .build();
            actions.add(outputNodeConnectorAction);
        }

        if ((actions != null) && (!actions.isEmpty())){
            //ApplyActions
            ApplyActions applyActions = new ApplyActionsBuilder().setAction(actions).build();

            //Instruction
            Instruction applyActionsInstruction = new InstructionBuilder() //
                    .setOrder(0).setInstruction(new ApplyActionsCaseBuilder()//
                            .setApplyActions(applyActions) //
                            .build()) //
                    .build();
            instructions.add(applyActionsInstruction);
        }

        //Build all the instructions together, based on the Instructions list
        Instructions allInstructions = new InstructionsBuilder()
                .setInstruction(instructions)
                .build();

        //Add the flow which will be inserted to the Flows registry
        FlowRegistryUtils.getInstance().writeToFlowRegistry(
                new FlowRegistryEntryBuilder()
                        .setEdgeNodeConnector(edgeNodeConnector+"default")
                        .setEdgeSwitch(edge_switch)
                        .setTableNum(table_num)
                        .build());

        // Put our Instruction in a list of Instructions
        flowBuilder
                .setMatch(match_default)
                .setBufferId(OFConstants.OFP_NO_BUFFER)
                .setInstructions(allInstructions)
                .setPriority(900)
                .setCookie(new FlowCookie(BigInteger.valueOf(flowCookieInc.getAndIncrement())))
                .setFlags(new FlowModFlags(false, false, false, false, false));

        return flowBuilder.build();
    }

    /**
     * The method which creates the flow for a specific table of a switch using groups option.
     *
     * @param edgeNodeConnector The edge node connector for which the flow will be added.
     * @param srcAddress The src IP address of the switch.
     * @param dstAddress The dst IP address of the switch.
     * @param table_num The number of the table where the flow will be added.
     * @param tablesCount The total number of tables in the specific switch.
     * @return The flow to be added.
     */
    private Flow createAddedFlowMultipleTablesGroups(String edgeNodeConnector, String srcAddress, String dstAddress, Integer table_num, Integer tablesCount){

        Short tableNum = Short.parseShort(table_num.toString());

        LOG.info("Test links for openflow:" + edgeNodeConnector.split(":")[1] + BootstrappingServiceImpl.tablesPerSwitch.get("openflow:".concat(edgeNodeConnector.split(":")[1])));
        Integer tables = BootstrappingServiceImpl.tablesPerSwitch.get("openflow:".concat(edgeNodeConnector.split(":")[1]));

        FlowBuilder flowBuilder = new FlowBuilder() //
                .setTableId((short) tableNum) //
                .setFlowName(edgeNodeConnector);

        flowBuilder.setId(new FlowId(edgeNodeConnector));

        Match match = new MatchBuilder()
                .setEthernetMatch(new EthernetMatchBuilder()
                        .setEthernetType(new EthernetTypeBuilder()
                                .setType(new EtherType(0x86DDL))
                                .build())
                        .setEthernetDestination(new EthernetDestinationBuilder()
                                .setAddress(new MacAddress("00:00:00:00:00:00"))
                                .build())
                        .build())
                .setLayer3Match(new Ipv6MatchArbitraryBitMaskBuilder()
                        .setIpv6SourceAddressNoMask(new Ipv6Address(srcAddress))
                        .setIpv6SourceArbitraryBitmask(new Ipv6ArbitraryMask(srcAddress))
                        .setIpv6DestinationAddressNoMask(new Ipv6Address(dstAddress))
                        .setIpv6DestinationArbitraryBitmask(new Ipv6ArbitraryMask(dstAddress))
                        .build())
                .build();

        ActionBuilder actionBuilder = new ActionBuilder();
        List<Action> actions = new ArrayList<Action>();
        List<Instruction> instructions = new ArrayList<>();

        //Actions
        Integer group = groupsHashMap.get(edgeNodeConnector);
        if (group != null) {
            LOG.info("Flow creation for " + edgeNodeConnector + " adding group " + group);
            Action groupAction = actionBuilder
                    .setOrder(0).setAction(new GroupActionCaseBuilder()
                            .setGroupAction(new GroupActionBuilder()
                                    .setGroupId(group.longValue())
                                    .build())
                            .build())
                    .build();
            actions.add(groupAction);
        }
        else{
            //means that we have a link connected to host, so it has no failover group
            LOG.info("Empty entry for " + edgeNodeConnector);
            Action outputNodeConnectorAction = actionBuilder
                    .setOrder(1).setAction(new OutputActionCaseBuilder()
                            .setOutputAction(new OutputActionBuilder()
                                    .setOutputNodeConnector(new Uri(edgeNodeConnector.split(":")[2]))
                                    .build())
                            .build())
                    .build();
            actions.add(outputNodeConnectorAction);
        }


/*        GroupsRegistryEntry groupsRegistryEntry = GroupsRegistryUtils.getInstance().readFromGroupRegistry(edgeNodeConnector);
        if (groupsRegistryEntry != null) {
            LOG.info("installed group for " + edgeNodeConnector);
            Long groupId = groupsRegistryEntry.getGroupId().longValue();
            Action groupAction = actionBuilder
                    .setOrder(0).setAction(new GroupActionCaseBuilder()
                            .setGroupAction(new GroupActionBuilder()
                                    .setGroupId(groupId)
                                    .build())
                            .build())
                    .build();
            actions.add(groupAction);
        }
        else{
            //means that we have a link connected to host, so it has no failover group
            LOG.info("did not install group for " + edgeNodeConnector);
            Action outputNodeConnectorAction = actionBuilder
                    .setOrder(1).setAction(new OutputActionCaseBuilder()
                            .setOutputAction(new OutputActionBuilder()
                                    .setOutputNodeConnector(new Uri(edgeNodeConnector.split(":")[2]))
                                    .build())
                            .build())
                    .build();
            actions.add(outputNodeConnectorAction);
        }*/

        //ApplyActions
        ApplyActions applyActions = new ApplyActionsBuilder().setAction(actions).build();

        //Instruction for Actions
        Instruction applyActionsInstruction = new InstructionBuilder()//
                .setOrder(0).setInstruction(new ApplyActionsCaseBuilder()//
                        .setApplyActions(applyActions) //
                        .build()) //
                .build();

        instructions.add(applyActionsInstruction);

        Integer tableId = table_num + 1;
        Short tableGoto = Short.parseShort(tableId.toString());

        if (tableGoto < tables){
            LOG.info("Not controller for openflow:" + edgeNodeConnector.split(":")[1] + BootstrappingServiceImpl.tablesPerSwitch.get("openflow:".concat(edgeNodeConnector.split(":")[1])));
            //if (tableGoto < tablesCount) {
            //Instruction for Goto
            GoToTableBuilder gttb = new GoToTableBuilder();
            gttb.setTableId(tableGoto);

            Instruction gotoInstruction = new InstructionBuilder()
                    .setOrder(1).setInstruction(new GoToTableCaseBuilder()
                            .setGoToTable(gttb.build())
                            .build())
                    .build();

            instructions.add(gotoInstruction);
        }

        //Build all the instructions together, based on the Instructions list
        Instructions allInstructions = new InstructionsBuilder()
                .setInstruction(instructions)
                .build();


        // Put our Instruction in a list of Instructions
        flowBuilder
                .setMatch(match)
                .setBufferId(OFConstants.OFP_NO_BUFFER)
                .setInstructions(allInstructions)
                .setPriority(1000)
                .setCookie(new FlowCookie(BigInteger.valueOf(flowCookieInc.getAndIncrement())))
                .setFlags(new FlowModFlags(false, false, false, false, false));
        return flowBuilder.build();
    }

    /**
     * The method which finds a failover port for a node connector.
     * It does not assign as failover port a node connector which connects
     * to a host. It only uses the internal node connectors.
     *
     * @param edgeNodeConnector An edge node connector.
     * @return The alternative port.
     */
    private Integer findAlternativePort(String edgeNodeConnector){

        Integer failoverPort = -1;

        for (Link l : BootstrappingServiceImpl.unconfiguredLinks){
            if (!l.getLinkId().getValue().contains("host")) {
                String sourceNode = l.getSource().getSourceTp().getValue();
                if (sourceNode.split(":")[1].equals(edgeNodeConnector.split(":")[1]) && !sourceNode.equals(edgeNodeConnector)) {
                    failoverPort = Integer.parseInt(sourceNode.split(":")[2]);
                    break;
                }
            }
        }

        return failoverPort;
    }

    /**
     * The method which adds the flow for an output combination to the switch.
     *
     * @param edgeNodeConnectors A list of the outputs of a combination.
     * @param srcAddress The src IP address of the switch.
     * @param dstAddress The dst IP address of the switch.
     * @param priority The priority which the flow will be inserted with.
     */
    public void addFlowForCombination(String edge_switch, List<String> edgeNodeConnectors, String srcAddress, String dstAddress, int priority) {

        LOG.debug("Adding flow to config datastore.");

        // create the flow
        Flow createdFlow = createAddedFlowForCombination(edge_switch, edgeNodeConnectors, srcAddress, dstAddress, priority);

        // build instance identifier for flow
        InstanceIdentifier<Flow> flowPath = InstanceIdentifier
                .builder(Nodes.class)
                .child(Node.class, new NodeKey(new NodeId(edge_switch)))
                .augmentation(FlowCapableNode.class)
                .child(Table.class, new TableKey(createdFlow.getTableId()))
                .child(Flow.class, new FlowKey(createdFlow.getId())).build();


        WriteTransaction transaction = db.newWriteOnlyTransaction();
        transaction.put(LogicalDatastoreType.CONFIGURATION, flowPath, createdFlow, true);
        CheckedFuture<Void, TransactionCommitFailedException> future = transaction.submit();
        Futures.addCallback(future,
                new LoggingFuturesCallBack<>("Failed to add flow.", LOG));
        LOG.debug("Added flow {} to config datastore.", createdFlow);
    }

    /**
     * The method which creates the flow for an output combination to the switch.
     *
     * @param edgeNodeConnectors A list of the outputs of a combination.
     * @param srcAddress The src IP address of the switch.
     * @param dstAddress The dst IP address of the switch.
     * @param priority The priority which the flow will be inserted with.
     * @return The created flow.
     */
    private Flow createAddedFlowForCombination(String edge_switch, List<String> edgeNodeConnectors, String srcAddress, String dstAddress, int priority){

        String name = "";
        for (String edgeNodeConnector : edgeNodeConnectors)
            name += edgeNodeConnector;

        FlowBuilder flowBuilder = new FlowBuilder() //
                .setTableId((short) 0) //
                .setFlowName(name);

        flowBuilder.setId(new FlowId(name));

        Match match = new MatchBuilder()
                .setEthernetMatch(new EthernetMatchBuilder()
                        .setEthernetType(new EthernetTypeBuilder()
                                .setType(new EtherType(0x86DDL))
                                .build())
                        .setEthernetDestination(new EthernetDestinationBuilder()
                                .setAddress(new MacAddress("00:00:00:00:00:00"))
                                .build())
                        .build())
                .setLayer3Match(new Ipv6MatchArbitraryBitMaskBuilder()
                        .setIpv6SourceAddressNoMask(new Ipv6Address(srcAddress))
                        .setIpv6SourceArbitraryBitmask(new Ipv6ArbitraryMask(srcAddress))
                        .setIpv6DestinationAddressNoMask(new Ipv6Address(dstAddress))
                        .setIpv6DestinationArbitraryBitmask(new Ipv6ArbitraryMask(dstAddress))
                        .build())
                .build();


        ActionBuilder actionBuilder = new ActionBuilder();
        List<Action> actions = new ArrayList<Action>();
        List<Instruction> instructions = new ArrayList<>();

        if (BootstrappingServiceImpl.groupsFlag){
            int j = 0;
            for (String edgeNodeConnector : edgeNodeConnectors) {
                String nodeConnector = edge_switch.concat(":").concat(edgeNodeConnector);
                Integer group = groupsHashMap.get(nodeConnector);
                if (group != null) {
                    LOG.info("Flow creation for " + nodeConnector + " adding group " + group);
                    Action groupAction = actionBuilder
                            .setOrder(j).setAction(new GroupActionCaseBuilder()
                                    .setGroupAction(new GroupActionBuilder()
                                            .setGroupId(group.longValue())
                                            .build())
                                    .build())
                            .build();
                    j++;
                    actions.add(groupAction);
                }
                else{
                    LOG.info("Empty entry for " + nodeConnector);
                    Action outputNodeConnectorAction = actionBuilder
                            .setOrder(j).setAction(new OutputActionCaseBuilder()
                                    .setOutputAction(new OutputActionBuilder()
                                            .setOutputNodeConnector(new Uri(nodeConnector.split(":")[2]))
                                            .build())
                                    .build())
                            .build();
                    j++;
                    actions.add(outputNodeConnectorAction);
                }
            }

        }
        else {
            int i = 0;
            for (String edgeNodeConnector : edgeNodeConnectors) {
                Action outputNodeConnectorAction = actionBuilder
                        .setOrder(i).setAction(new OutputActionCaseBuilder()
                                .setOutputAction(new OutputActionBuilder()
                                        .setOutputNodeConnector(new Uri(edgeNodeConnector))
                                        .build())
                                .build())
                        .build();
                i++;
                actions.add(outputNodeConnectorAction);
            }
        }

        //ApplyActions
        ApplyActions applyActions = new ApplyActionsBuilder().setAction(actions).build();

        //Instruction for Actions
        Instruction applyActionsInstruction = new InstructionBuilder()//
                .setOrder(0).setInstruction(new ApplyActionsCaseBuilder()//
                        .setApplyActions(applyActions) //
                        .build()) //
                .build();

        instructions.add(applyActionsInstruction);

        //Build all the instructions together, based on the Instructions list
        Instructions allInstructions = new InstructionsBuilder()
                .setInstruction(instructions)
                .build();


        // Put our Instruction in a list of Instructions
        flowBuilder
                .setMatch(match)
                .setBufferId(OFConstants.OFP_NO_BUFFER)
                .setInstructions(allInstructions)
                .setPriority(priority)
                .setCookie(new FlowCookie(BigInteger.valueOf(flowCookieInc.getAndIncrement())))
                .setFlags(new FlowModFlags(false, false, false, false, false));

        return flowBuilder.build();
    }
}