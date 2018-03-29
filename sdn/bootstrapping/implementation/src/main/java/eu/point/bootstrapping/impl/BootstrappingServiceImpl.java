/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.bootstrapping.impl;

import com.google.common.base.Optional;
import com.google.common.util.concurrent.CheckedFuture;
import com.google.common.util.concurrent.Futures;
import eu.point.bootstrapping.impl.icn.IcnIdConfigurator;
import eu.point.bootstrapping.impl.topology.NetworkGraphImpl;
import eu.point.client.Client;
import eu.point.registry.impl.LinkRegistryUtils;
import eu.point.registry.impl.NodeConnectorRegistryUtils;
import eu.point.registry.impl.NodeRegistryUtils;
import eu.point.tmsdn.impl.TmSdnMessages;
import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.md.sal.binding.api.ReadOnlyTransaction;
import org.opendaylight.controller.md.sal.common.api.data.LogicalDatastoreType;
import org.opendaylight.controller.md.sal.common.api.data.ReadFailedException;
import org.opendaylight.yang.gen.v1.urn.eu.point.bootstrapping.rev150722.*;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.flow.registry.FlowRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.connector.registry.NodeConnectorRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.registry.NodeRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.connector.registry.NodeConnectorRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.Nodes;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.node.NodeConnector;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.Node;
import org.opendaylight.yang.gen.v1.urn.opendaylight.port.statistics.rev131214.FlowCapableNodeConnectorStatisticsData;
import org.opendaylight.yang.gen.v1.urn.opendaylight.port.statistics.rev131214.flow.capable.node.connector.statistics.FlowCapableNodeConnectorStatistics;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.topology.Link;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.opendaylight.yangtools.yang.common.RpcResult;
import org.opendaylight.yangtools.yang.common.RpcResultBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Future;

/**
 * The bootstrapping service class.
 * It includes all the REST APIs for the bootstrapping module.
 *
 * @author George Petropoulos, Marievi Xezonaki
 * @version cycle-2
 */
public class BootstrappingServiceImpl implements BootstrappingService {

    private static final Logger LOG = LoggerFactory.getLogger(BootstrappingServiceImpl.class);

    public static Map<String, String> assignedNodeIds = new HashMap<>();
    public static Map<String, String> assignedLinkIds = new HashMap<>();
    public static List<Link> unconfiguredLinks = new ArrayList<>();
    public static List<Link> allLinks = new ArrayList<>();
    public static boolean applicationActivated = false;
    public static List<List<Link>> groupedLinks = new ArrayList<>();
    public static boolean multipleTablesFlag = false;
    public static boolean groupsFlag = false;
    public static boolean deploymentToolBootstrapping = true;
    public static HashMap<String, Integer> tablesPerSwitch = new HashMap<>();
    DataBroker dataBroker;
    /**
     * The constructor method.
     */
    public BootstrappingServiceImpl(DataBroker db) {
        dataBroker = db;
    }

    /**
     * The method which configures the client of the TM-SDN interface.
     * It receives as input the TM IP address and port.
     * @param input The TM configuration input, including parameters such as server ip, port, node id, lid and ilid.
     * @return It returns a void future result.
     * @see ConfigureTmInput
     */
    @Override
    public Future<RpcResult<Void>> configureTm(ConfigureTmInput input) {

        LOG.info("Received input {}.", input);
        Client.serverIP = input.getTmIp();
        Client.tcpPort = input.getTmPort();
        IcnIdConfigurator.tmOpenflowId = input.getTmOpenflowId();
        IcnIdConfigurator.tmAttachedOpenflowSwitchId = input.getTmAttachedSwitchId();
        IcnIdConfigurator.tmNodeId = input.getTmNodeId();
        IcnIdConfigurator.tmLidPosition = input.getTmLid();
        IcnIdConfigurator.tmInternalLidPosition = input.getTmInternalLid();
        IcnIdConfigurator.tmSourceNodeConnectorId = input.getTmSourceNodeConnector();
        if (input.isDeploymentToolOn() != null)
            deploymentToolBootstrapping = input.isDeploymentToolOn();
        if (input.isGroupsOptionOn() != null)
            groupsFlag = input.isGroupsOptionOn();
        if (input.isTablesOptionOn() != null)
            multipleTablesFlag = input.isTablesOptionOn();

        BootstrappingServiceImpl.assignedNodeIds.put(IcnIdConfigurator.tmOpenflowId,
                IcnIdConfigurator.tmNodeId);

        BootstrappingServiceImpl.assignedLinkIds.put(IcnIdConfigurator.tmOpenflowId + ","
                + IcnIdConfigurator.tmAttachedOpenflowSwitchId + ","
                + IcnIdConfigurator.tmSourceNodeConnectorId,
                IcnIdConfigurator.generateLid(IcnIdConfigurator.tmLidPosition));

        //Adding input to the registries
        NodeRegistryUtils.getInstance().writeToNodeRegistry(new NodeRegistryEntryBuilder()
                .setNoneName(IcnIdConfigurator.tmOpenflowId)
                .setNoneId(IcnIdConfigurator.tmNodeId)
                .build());

        //find the ODL Link ID to store to the registry
        String odlLinkID = "";
        NetworkGraphImpl networkGraph = NetworkGraphImpl.getInstance();
        List<Link> links = networkGraph.getLinksFromTopology();
        for (Link link : links){
            if (link.getSource().getSourceNode().equals(IcnIdConfigurator.tmOpenflowId) && (link.getDestination().getDestNode().equals(IcnIdConfigurator.tmAttachedOpenflowSwitchId))){
                odlLinkID = link.getLinkId().getValue();
                break;
            }
        }
        LinkRegistryUtils.getInstance().writeToLinkRegistry(new LinkRegistryEntryBuilder()
                .setLinkName(IcnIdConfigurator.tmOpenflowId + ","
                        + IcnIdConfigurator.tmAttachedOpenflowSwitchId + ","
                        + IcnIdConfigurator.tmSourceNodeConnectorId)
                .setLinkId(IcnIdConfigurator.generateLid(IcnIdConfigurator.tmLidPosition))
                .setOdlLinkID(odlLinkID)
                .build());

        NodeConnectorRegistryUtils.getInstance().writeToNodeConnectorRegistry(
                new NodeConnectorRegistryEntryBuilder()
                .setSrcNode(IcnIdConfigurator.tmAttachedOpenflowSwitchId)
                .setDstNode(IcnIdConfigurator.tmOpenflowId)
                .setNoneConnectorName(IcnIdConfigurator.tmSourceNodeConnectorId)
                .build());

        return Futures.immediateFuture(RpcResultBuilder.<Void>success().build());
    }

    /**
     * The method which activates the bootstrapping application.
     * @param input The input to activate the bootstrapping application, boolean or false.
     * @return It returns a void future result.
     * @see ActivateApplicationInput
     */
    @Override
    public Future<RpcResult<Void>> activateApplication(ActivateApplicationInput input) {
        applicationActivated = input.isStatus();
        LOG.info("Received input {}.", input);
        // if true then activate.

        if (applicationActivated) {
            LOG.info("Activating bootstrapping application.");

            if (multipleTablesFlag == true){
                //each flow of one switch will be put in different table
                LOG.info("Multiple tables chosen.");
                if (groupsFlag)
                    LOG.info("Groups chosen.");
                else
                    LOG.info("Groups not chosen.");
                groupLinks(unconfiguredLinks);
                for (List<Link> links : groupedLinks){
                    tablesPerSwitch.put("openflow:".concat(links.get((0)).getLinkId().getValue().split(":")[1]), links.size());
                    IcnIdConfigurator.getInstance().addLinksMultipleTables(links);
                }
            }
            else{
                //all flows of one switch will be put in the same table
                LOG.info("Single tables chosen.");
                groupLinks(unconfiguredLinks);
                for (List<Link> links : groupedLinks){
                    IcnIdConfigurator.getInstance().addLinks(links);
                }
//                IcnIdConfigurator.getInstance().addLinks(unconfiguredLinks);
            }

  //          IcnIdConfigurator.getInstance().addLinks(unconfiguredLinks);
            //IcnIdConfigurator.getInstance().addMultipleLinks(unconfiguredLinks);
        }
        // else de-activate.
        else {
            LOG.info("De-activating bootstrapping application.");
        }
        return Futures.immediateFuture(RpcResultBuilder.<Void>success().build());
    }

    /**
     * The method which configures a switch manually, without any resource management function.
     *
     * @param input The input to configure a switch manually, including the switch id, port id, and link id.
     * @return It returns a void future result.
     * @see ConfigureSwitchInput
     */
    @Override
    public Future<RpcResult<Void>> configureSwitch(ConfigureSwitchInput input) {
        IcnIdConfigurator.getInstance().addLinkManually(input.getSwitchId(),
                input.getPortId(), input.getLinkId());
        return Futures.immediateFuture(RpcResultBuilder.<Void>success().build());
    }


    /**
     * The method which requests the node id and link id of a certain link.
     * It either finds it in the registry, or requests it from the resource manager directly
     * and updates the registry itself.
     *
     * @param input The link, including source and destination nodes, requiring unique id.
     * @return It returns a response including the assigned node and link ids information.
     * @see NodeLinkInformationInput
     * @see NodeLinkInformationOutput
     */
    @Override
    public Future<RpcResult<NodeLinkInformationOutput>> nodeLinkInformation(
            NodeLinkInformationInput input) {
        NodeLinkInformationOutput output = null;
        // if information exist in the registry then send it in the response.
        if (assignedNodeIds.containsKey(input.getSrcNode())
                && assignedLinkIds.containsKey(input.getSrcNode() + "," + input.getDstNode())) {
            output = new NodeLinkInformationOutputBuilder()
                    .setSrcNodeId(assignedNodeIds.get(input.getSrcNode()))
                    .setLinkId(assignedLinkIds.get(input.getSrcNode() + "," + input.getDstNode()))
                    .build();
            LOG.info("Return node id {} for request node {}.", assignedNodeIds.get(input.getSrcNode()),
                    input.getSrcNode());
        }
        else {
            // otherwise request the information from the resource manager
            List<TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest> requests =
                    new ArrayList<>();
            requests.add(TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest.newBuilder()
                    .setSrcNode(input.getSrcNode())
                    .setDstNode(input.getDstNode())
                    .build());
            TmSdnMessages.TmSdnMessage.ResourceRequestMessage.Builder rrBuilder =
                    TmSdnMessages.TmSdnMessage.ResourceRequestMessage.newBuilder();
            rrBuilder.addAllRequests(requests);
            TmSdnMessages.TmSdnMessage message = TmSdnMessages.TmSdnMessage.newBuilder()
                    .setResourceRequestMessage(rrBuilder.build())
                    .setType(TmSdnMessages.TmSdnMessage.TmSdnMessageType.RR)
                    .build();
            TmSdnMessages.TmSdnMessage.ResourceOfferMessage resourceOffer =
                    new Client().sendMessage(message);

            // update the registries with the received information
            if (resourceOffer != null && resourceOffer.getOffersList().size() != 0) {
                LOG.info("Received the node link information {}: ", resourceOffer.getOffersList().get(0));
                String nodeid = resourceOffer.getOffersList().get(0).getNid();
                String lid = resourceOffer.getOffersList().get(0).getLid();

                BootstrappingServiceImpl.assignedNodeIds.put(input.getSrcNode(), nodeid);
                BootstrappingServiceImpl.assignedLinkIds.put(
                        input.getSrcNode() + "," + input.getDstNode(), lid);

                NodeRegistryUtils.getInstance().writeToNodeRegistry(new NodeRegistryEntryBuilder()
                        .setNoneName(input.getSrcNode())
                        .setNoneId(nodeid)
                        .build());
                LinkRegistryUtils.getInstance().writeToLinkRegistry(new LinkRegistryEntryBuilder()
                        .setLinkName(input.getSrcNode() + "," + input.getDstNode())
                        .setLinkId(lid)
                        .build());
                output = new NodeLinkInformationOutputBuilder()
                        .setSrcNodeId(assignedNodeIds.get(input.getSrcNode()))
                        .setLinkId(assignedLinkIds.get(input.getSrcNode() + "," + input.getDstNode()))
                        .build();
            }
        }
        return RpcResultBuilder.success(output).buildFuture();
    }

    /**
     * The method which configures the Node Registry.
     * It receives as input the TM IP address and port.
     * @param input The Node Registry configuration input, including parameters
     *              which are the node name and the node id.
     * @return It returns a void future result.
     * @see ConfigureNodeRegistryInput
     */
    @Override
    public Future<RpcResult<Void>> configureNodeRegistry(ConfigureNodeRegistryInput input) {
        String noneName = input.getNoneName();
        String noneId = input.getNoneId();
        LOG.info("Got from rpc " + noneName + " " + noneId);
        NodeRegistryUtils.getInstance().writeToNodeRegistry(new NodeRegistryEntryBuilder()
                .setNoneName(noneName)
                .setNoneId(noneId)
                .build());
        return Futures.immediateFuture(RpcResultBuilder.<Void>success().build());
    }

    /**
     * The method which configures the Node Connector Registry.
     * @param input The Node Connector Registry configuration input, including parameters
     *              which are the node connector name and its src and dst nodes.
     * @return It returns a void future result.
     * @see ConfigureNodeConnectorRegistryInput
     */
    @Override
    public Future<RpcResult<Void>> configureNodeConnectorRegistry(ConfigureNodeConnectorRegistryInput input) {
        String srcNode = input.getSrcNode();
        String dstNode = input.getDstNode();
        String noneConnectorName = input.getNoneConnectorName();
        LOG.info("Got from rpc " + srcNode + " " + dstNode + " " + noneConnectorName);
        NodeConnectorRegistryUtils.getInstance().writeToNodeConnectorRegistry(new NodeConnectorRegistryEntryBuilder()
                .setSrcNode(srcNode)
                .setDstNode(dstNode)
                .setNoneConnectorName(noneConnectorName)
                .build());
        return Futures.immediateFuture(RpcResultBuilder.<Void>success().build());
    }

    /**
     * The method which groups a list of links to smaller lists according to
     * the switch which they belong to.
     *
     * @param linksToGroup The list of all the links to be grouped.
     */
    public void groupLinks(List<Link> linksToGroup){
        //group the links according to source switch (which will be configured for this link)
        for (Link link : linksToGroup){

            if (!link.getLinkId().getValue().startsWith("host")) {
                //String Switch = link.getSource().getSourceNode().getValue();
                //String parts[] = Switch.split(":");
                //Integer switchNum = Integer.parseInt(parts[1]);

                if (groupedLinks.size() > 0) {
                    int j = 1;
                    boolean flag = false;
                    for (List<Link> list : groupedLinks){
                        if ( (list != null) && (!list.isEmpty()) && (list.size() > 0)){
                            if (list.get(0).getLinkId().getValue().split(":")[1].equals(link.getLinkId().getValue().split(":")[1])){
                                list.add(link);
                                flag = true;
                            }
                        }
                        j++;
                    }
                    if (flag == false){
                        ArrayList<Link> links = new ArrayList<>();
                        links.add(link);
                        groupedLinks.add(links);
                    }
                }
                else {
                    ArrayList<Link> links = new ArrayList<>();
                    links.add(link);
                    groupedLinks.add(links);
                }
            }
            else{
                continue;
            }

        }
    }

    /**
     * The method which returns the value of multipleTablesFlag variable.
     */
    public static boolean getMultipleTablesFlag(){
        return multipleTablesFlag;
    }

}