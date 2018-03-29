/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.bootstrapping.impl.icn;

import com.google.common.util.concurrent.CheckedFuture;
import eu.point.bootstrapping.impl.BootstrappingServiceImpl;
import eu.point.bootstrapping.impl.switches.SwitchConfigurator;
import eu.point.bootstrapping.impl.topology.NetworkGraphImpl;
import eu.point.client.Client;
import eu.point.registry.impl.*;
import eu.point.tmsdn.impl.TmSdnMessages;
import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.md.sal.binding.api.ReadOnlyTransaction;
import org.opendaylight.controller.md.sal.common.api.data.LogicalDatastoreType;
import org.opendaylight.controller.md.sal.common.api.data.ReadFailedException;
import org.opendaylight.yang.gen.v1.urn.eu.point.bootstrapping.rev150722.BootstrappingService;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.LinkInfoRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.LinkRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.NodeConnectorRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.flow.registry.FlowRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.flow.registry.FlowRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.groups.registry.GroupsRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.info.registry.LinkInfoRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.info.registry.LinkInfoRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.connector.registry.NodeConnectorRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.connector.registry.NodeConnectorRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.registry.NodeRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.address.tracker.rev140617.AddressCapableNodeConnector;
import org.opendaylight.yang.gen.v1.urn.opendaylight.address.tracker.rev140617.address.node.connector.Addresses;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.Nodes;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.node.NodeConnector;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.Node;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.topology.Link;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.net.Inet6Address;
import java.net.UnknownHostException;
import java.util.*;

/**
 * The ICN configurator class.
 * It includes all the required methods to map opendaylight/sdn information to ICN ones.
 *
 * @author George Petropoulos, Marievi Xezonaki
 * @version cycle-2
 */
public class IcnIdConfigurator {

    private static final Logger LOG = LoggerFactory.getLogger(IcnIdConfigurator.class);
    public static String tmOpenflowId;
    public static String tmNodeId;
    public static String tmAttachedOpenflowSwitchId;
    public static int tmLidPosition;
    public static int tmInternalLidPosition;
    public static String tmSourceNodeConnectorId;
    private static IcnIdConfigurator instance = null;
    private DataBroker db;
    private static List<String> lidsList = new ArrayList<>();
    private static List<String> NodeConnectorsList = new ArrayList<>();
    private static int max = 1;
    private static List<String> connectorsToRemove = new ArrayList<>();
    private  List<List<String>> lidsListAll = new ArrayList<>();
    private  List<List<String>> NodeConnectorsListAll = new ArrayList<>();
    private static List<Integer> filledTables = new ArrayList<>();
    public static List<String> tablesFreed = new ArrayList<>();

    /**
     * The constructor class.
     */
    private IcnIdConfigurator() {

    }

    /**
     * The method which sets the databroker.
     *
     * @param db The data broker object.
     * @see IcnIdConfigurator
     */
    public void setDb(DataBroker db) {
        this.db = db;
    }

    /**
     * The method which creates a single instance of the ICN Configurator class.
     * It also writes to the registry some initial required parameters,
     * e.g which host acts as TM, with which node id, and link id.
     *
     * @return It returns the icn id configurator instance.
     * @see IcnIdConfigurator
     */
    public static IcnIdConfigurator getInstance() {
        if (instance == null) {
            instance = new IcnIdConfigurator();
        }
        return instance;
    }

    /**
     * The method which adds links to the ICN topology.
     * If bootstrapping application is active, then for each link in the list,
     * it calls the relevant method.
     * Otherwise it adds them to the to-be-configured list.
     *
     * @param links The list of links to be added.
     * @see Link
     */
    public void addLinks(List<Link> links) {
        if (BootstrappingServiceImpl.applicationActivated) {
            LOG.info("The unconf are " + BootstrappingServiceImpl.unconfiguredLinks.size());
            LOG.info("Application active, hence generating LIDs for {}", links);
            for (Iterator<Link> iter = links.listIterator(); iter.hasNext(); ) {
                Link l = iter.next();
                if (!l.getLinkId().getValue().startsWith("host")) {
                    String sourceNodeId = l.getSource().getSourceNode().getValue();
                    String sourceNodeConnectorId = l.getSource().getSourceTp().getValue();
                    String dstNodeId = l.getDestination().getDestNode().getValue();


                    if (FlowRegistryUtils.getInstance().readFromFlowRegistry(sourceNodeConnectorId) == null) {
                        LOG.info("Writing to FlowRegistry connector " + sourceNodeConnectorId + " for switch " + sourceNodeId);
                        //Add the flow which will be inserted to the Flows registry
                        FlowRegistryUtils.getInstance().writeToFlowRegistry(
                                new FlowRegistryEntryBuilder()
                                        .setEdgeNodeConnector(sourceNodeConnectorId)
                                        .setEdgeSwitch(sourceNodeId)
                                        .setTableNum(0)
                                        .build());
                    }

                    LOG.info("About to add link " + l.getLinkId().getValue());
                    boolean linkAdded = addLink(sourceNodeId, dstNodeId, sourceNodeConnectorId, l.getLinkId().getValue());
                    if (linkAdded)
                        iter.remove();
                }
            }

            //Up to here all the seperate flows have been added. Now we will add the combinations
            //Check if every lid corresponds to a node connector in the registry
            addFlowsForCombinations();
        }
        else {
            LOG.info("Application not active, hence adding {} to unconfigured.", links);
            for (Link link : links){
                if (!BootstrappingServiceImpl.unconfiguredLinks.contains(link)){
                    LOG.info("First time will become unconfigured " + link.getLinkId().getValue());
                    BootstrappingServiceImpl.unconfiguredLinks.add(link);
                }
            }
 //           BootstrappingServiceImpl.unconfiguredLinks.addAll(links);
        }
    }

    /**
     * The method which initiates the procedure for combining all possible outputs of a link.
     * Each time a link is added, it gathers all the edge connectors of its source switch together
     * and combines them again from scratch.
     */
    private void addFlowsForCombinations(){
        LOG.info("About to find combinations.");

        if (NodeConnectorsList.size() == 0){
            return;
        }
        else{
            List<LinkInfoRegistryEntry> allEntries = LinkInfoRegistryUtils.getInstance().readAllFromLinkInfoRegistry();
            if (allEntries != null) {
                for (LinkInfoRegistryEntry entry : allEntries){
                    if (entry.getEdgeNodeConnector() != null){
                        if (entry.getEdgeNodeConnector().split(":")[1].equals(NodeConnectorsList.get(0).split(":")[1])){
                            if (!lidsList.contains(entry.getLid()) && (!NodeConnectorsList.contains(entry.getEdgeNodeConnector()))){
                                lidsList.add(entry.getLid());
                                NodeConnectorsList.add((entry.getEdgeNodeConnector()));
                            }
                        }
                    }

                }
            }
        }

        String [] source = NodeConnectorsList.get(0).split(":");
        String sourceNodeId = source[0] + ":" + source[1];

        List<String> allCombinations = new ArrayList<>();
        for (int i = 2; i <= lidsList.size(); i++) {
            Combinations.combination(lidsList, i);
            String allCombs = Combinations.output;
            allCombinations.add(allCombs);
        }

        LOG.info("We will configure the combinations of switch " + sourceNodeId + " which are " + lidsList.size() + " " + NodeConnectorsList.size());
        configureCombinations(allCombinations, sourceNodeId);

        lidsList.clear();
        NodeConnectorsList.clear();
        lidsList = new ArrayList<>();
        NodeConnectorsList = new ArrayList<>();
    }

    /**
     * The method which configures all output combinations of a switch. It initiates the procedure
     * of adding the appropriate flow for each output combination.
     *
     * @param allCombinations The list of all the output combinations to be configured.
     * @param sourceNodeId The switch whose output combinations are being cofigured.
     */
    private void configureCombinations(List<String> allCombinations, String sourceNodeId){
        for (String combination : allCombinations){
            //Combination is for example all the pairs or triples

            String [] one_combination = combination.split("#");
            for (String one_comb : one_combination){
                configureCombination(one_comb, sourceNodeId);
            }
        }
    }

    /**
     * The method which configures each output combination of a switch.
     * @param one_comb The combination to be configured.
     * @param sourceNodeId The switch whose output combinations are being cofigured.
     */
    private void configureCombination(String one_comb, String sourceNodeId){
        int pos;
        List<Integer> posList = new ArrayList<>();
        List<String> connectorsForFlow = new ArrayList<>();

        String [] combination_members = one_comb.split(",");
        //ex. String [] combination_members = [1][2] , [2][3], [1][3], [1][2][3]
        int combinationSize = combination_members.length;
        int priority;
        if (combinationSize > max){
            priority = 1000 + combinationSize*10;
            max = combinationSize;
        }
        else{
            priority = 1000 + combinationSize*10;
        }

        for (String member : combination_members) {
            pos = lidBitPosition(member);
            LOG.info("Pos " + pos + " " + member);
            posList.add(pos);

            //find corresponding edge connector for each member
            String nodeConnector = correspondingNodeConnector(member);
            connectorsForFlow.add(nodeConnector.split(":")[2]);
        }

        //find combined lid for the combination
        String combinedLid = combineLids(posList);
        posList.clear();

        //find combined ip src and dst addresses
        String addresses = findAddresses(combinedLid);
        String srcAddress = addresses.split(",")[0];
        String dstAddress = addresses.split(",")[1];

        //Add the flow which will be inserted to the Flows registry
        String edgeNodeConnector = sourceNodeId.split(":")[1] + ":";
        for (String connectorForFlow : connectorsForFlow) {
            edgeNodeConnector += connectorForFlow + ":";
        }

        LOG.info("Writing to FlowRegistry connector " + edgeNodeConnector + " for switch " + sourceNodeId);
        FlowRegistryUtils.getInstance().writeToFlowRegistry(
                new FlowRegistryEntryBuilder()
                        .setEdgeNodeConnector(edgeNodeConnector)
                        .setEdgeSwitch(sourceNodeId)
                        .setTableNum(0)
                        .build());


        //configure switch with the ABM rule
        new SwitchConfigurator(db).addFlowForCombination(sourceNodeId, connectorsForFlow,
                srcAddress, dstAddress, priority);


        connectorsForFlow.clear();

    }

    /**
     * The method which finds the corresponding node connector for
     * an output number.
     *
     * @param member The output number for which the corresponding node connector will be found.
     * @return The corresponding node connector.
     */
    private String correspondingNodeConnector(String member){

        int position = -1;
        int i = 0;
        for (String lid : lidsList){
            if (lid.equals(member)){
                position = i;
                break;
            }
            i++;
        }

        String nodeConnector = NodeConnectorsList.get(position);
        return nodeConnector;
    }

    /**
     * The method which finds the src and dst IP addresses for a (combined) lid.
     *
     * @param combinedLid The lid for which the src and dst IP addresses
     *                    will be found.
     * @return The src and dst IP addresses, concatenated.
     */
    private String findAddresses(String combinedLid){
        String downLid = combinedLid.substring(0, 128);
        String upLid = combinedLid.substring(128, 256);
        String srcAddress = "";
        String dstAddress = "";
        String addresses = "";
        try {
            srcAddress = lidToIpv6(downLid);
            dstAddress = lidToIpv6(upLid);
            addresses = srcAddress + "," + dstAddress;
        } catch (Exception e) {
            LOG.warn(e.getMessage());
        }
        return addresses;
    }

    /**
     * The method which adds multiple links to the ICN topology at the same time.
     * If bootstrapping application is active, then it sends a single request
     * to the resource manager for all these links.
     * Otherwise it adds them to the to-be-configured list.
     *
     * @param links The list of links to be added.
     * @see Link
     */
    public void addMultipleLinks(List<Link> links) {
        if (BootstrappingServiceImpl.applicationActivated) {
            LOG.info("Application active, hence generating LIDs for {}", links);
            List<TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest> requests =
                    new ArrayList<>();

            for (Iterator<Link> iter = links.listIterator(); iter.hasNext(); ) {
                Link l = iter.next();
                if (!l.getLinkId().getValue().startsWith("host")) {
                    String sourceNodeId = l.getSource().getSourceNode().getValue();
                    String sourceNodeConnectorId = l.getSource().getSourceTp().getValue();
                    String dstNodeId = l.getDestination().getDestNode().getValue();
                    TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest req =
                            prepareRequest(sourceNodeId, dstNodeId, sourceNodeConnectorId);
                    if (req != null) {
                        requests.add(req);
                        iter.remove();
                    }
                }
            }
            TmSdnMessages.TmSdnMessage.ResourceRequestMessage.Builder rrBuilder =
                    TmSdnMessages.TmSdnMessage.ResourceRequestMessage.newBuilder();
            rrBuilder.addAllRequests(requests);
            TmSdnMessages.TmSdnMessage message = TmSdnMessages.TmSdnMessage.newBuilder()
                    .setResourceRequestMessage(rrBuilder.build())
                    .setType(TmSdnMessages.TmSdnMessage.TmSdnMessageType.RR)
                    .build();
            TmSdnMessages.TmSdnMessage.ResourceOfferMessage resourceOffer =
                    new Client().sendMessage(message);
            LOG.info("Received resource offer message {}", resourceOffer);

            for (int i = 0; i < resourceOffer.getOffersCount(); i++) {
                TmSdnMessages.TmSdnMessage.ResourceOfferMessage.RecourceOffer ro =
                        resourceOffer.getOffers(i);

                String sourceNodeId = requests.get(i).getSrcNode();
                String dstNodeId = requests.get(i).getDstNode();
                int nodeConnectorId = requests.get(i).getNodeConnector();
                String sourceNodeConnectorId = sourceNodeId + ":" + nodeConnectorId;

                LOG.debug("Received the node link information {}: ", ro);
                String nodeid = ro.getNid();
                String genlid = ro.getLid();

                BootstrappingServiceImpl.assignedNodeIds.put(sourceNodeId, nodeid);
                BootstrappingServiceImpl.assignedLinkIds.put(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId, genlid);

                //add given resources to the registries for future need
                NodeRegistryUtils.getInstance().writeToNodeRegistry(new NodeRegistryEntryBuilder()
                        .setNoneName(sourceNodeId)
                        .setNoneId(nodeid)
                        .build());
                LinkRegistryEntry linkRegistryEntry = LinkRegistryUtils.getInstance().readFromLinkRegistry(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId);
                if (linkRegistryEntry != null) {
                    LinkRegistryUtils.getInstance().deleteFromLinkRegistry(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId);
                }
                LinkRegistryUtils.getInstance().writeToLinkRegistry(new LinkRegistryEntryBuilder()
                        .setLinkName(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId)
                        .setLinkId(genlid)
                        .build());
                NodeConnectorRegistryUtils.getInstance().writeToNodeConnectorRegistry(
                        new NodeConnectorRegistryEntryBuilder()
                                .setNoneConnectorName(sourceNodeConnectorId)
                                .setSrcNode(sourceNodeId)
                                .setDstNode(dstNodeId)
                                .build());

                String lid = new StringBuilder(genlid).reverse().toString();
                String downLid = lid.substring(0, 128);
                String upLid = lid.substring(128, 256);
                String srcAddress = "";
                String dstAddress = "";
                try {
                    srcAddress = lidToIpv6(downLid);
                    dstAddress = lidToIpv6(upLid);
                    LOG.info("Src and dst ipv6 addresses {} and {}", srcAddress, dstAddress);
                } catch (Exception e) {
                    LOG.warn(e.getMessage());
                    return;
                }

                //configure switch with the ABM rule
                new SwitchConfigurator(db).addFlowConfig(sourceNodeId, dstNodeId, sourceNodeConnectorId,
                        srcAddress, dstAddress);
            }
        } else {
            LOG.info("Application not active, hence adding {} to unconfigured.", links);
            BootstrappingServiceImpl.unconfiguredLinks.addAll(links);
            BootstrappingServiceImpl.allLinks.addAll(links);
        }
    }

    /**
     * The method which prepares a resource request for a single link.
     *
     * @param sourceNodeId The source node id of the link.
     * @param dstNodeId The destination node id of the link.
     * @param sourceNodeConnectorId The source node connector of the link.
     * @return The resource request.
     */
    private TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest prepareRequest(
            String sourceNodeId, String dstNodeId, String sourceNodeConnectorId) {
        TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest request = null;
        LOG.info("Adding link {}, {}, {}", sourceNodeId, dstNodeId, sourceNodeConnectorId);
        if (!BootstrappingServiceImpl.assignedLinkIds.containsKey(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId)) {
            int nodeConnectorId = Integer.parseInt(sourceNodeConnectorId.split(":")[2]);
            request = TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest.newBuilder()
                    .setSrcNode(sourceNodeId)
                    .setDstNode(dstNodeId)
                    .setNodeConnector(nodeConnectorId)
                    .build();
        }
        return request;
    }


    /**
     * The method which adds a single link to the ICN topology.
     * If it is not configured already, then it prepares a resource request,
     * sends it to the resource manager and prepares the arbitrary bitmask
     * rule and configures it to the switch.
     * It finally adds the assigned ids to the registry.
     *
     * @param sourceNodeId The source node id of the link.
     * @param dstNodeId The destination node id of the link.
     * @param sourceNodeConnectorId The source node connector of the link.
     * @return True or false, depending on the outcome of operation.
     */
    private boolean addLink(String sourceNodeId, String dstNodeId, String sourceNodeConnectorId, String odlLinkID) {
        LOG.info("Adding link {}, {}, {}", sourceNodeId, dstNodeId, sourceNodeConnectorId);
        if (!BootstrappingServiceImpl.assignedLinkIds.containsKey(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId)) {
            LOG.info("Link " + sourceNodeConnectorId + " not configured, generating parameters.");

            //if link is not configured, then request resources from resource manager
            List<TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest> requests =
                    new ArrayList<>();
            int nodeConnectorId = Integer.parseInt(sourceNodeConnectorId.split(":")[2]);
            requests.add(TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest.newBuilder()
                    .setSrcNode(sourceNodeId)
                    .setDstNode(dstNodeId)
                    .setNodeConnector(nodeConnectorId)
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

            LOG.debug("Received resource offer {}", resourceOffer);
            if (resourceOffer == null || resourceOffer.getOffersList().size() == 0)
                return false;
            LOG.debug("Received the node link information {}: ", resourceOffer.getOffersList().get(0));
            String nodeid = resourceOffer.getOffersList().get(0).getNid();
            String genlid = resourceOffer.getOffersList().get(0).getLid();

            BootstrappingServiceImpl.assignedNodeIds.put(sourceNodeId, nodeid);
            BootstrappingServiceImpl.assignedLinkIds.put(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId, genlid);
            LOG.info("Generated lid for source {} and dst {} is {} ", sourceNodeId, dstNodeId, genlid);

            //add given resources to the registries for future need
            NodeRegistryUtils.getInstance().writeToNodeRegistry(new NodeRegistryEntryBuilder()
                    .setNoneName(sourceNodeId)
                    .setNoneId(nodeid)
                    .build());

            LinkRegistryEntry linkRegistryEntry = LinkRegistryUtils.getInstance().readFromLinkRegistry(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId);
            if (linkRegistryEntry != null) {
                LinkRegistryUtils.getInstance().deleteFromLinkRegistry(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId);
            }
            LinkRegistryUtils.getInstance().writeToLinkRegistry(new LinkRegistryEntryBuilder()
                    .setLinkName(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId)
                    .setLinkId(genlid)
                    .setOdlLinkID(odlLinkID)
                    .build());

            NodeConnectorRegistryUtils.getInstance().writeToNodeConnectorRegistry(
                    new NodeConnectorRegistryEntryBuilder()
                            .setNoneConnectorName(sourceNodeConnectorId)
                            .setSrcNode(sourceNodeId)
                            .setDstNode(dstNodeId)
                            .build());

            String lid = new StringBuilder(genlid).reverse().toString();
            String downLid = lid.substring(0, 128);
            String upLid = lid.substring(128, 256);
            String srcAddress = "";
            String dstAddress = "";
            try {
                srcAddress = lidToIpv6(downLid);
                dstAddress = lidToIpv6(upLid);
                LOG.info("Src and dst ipv6 addresses {} and {}", srcAddress, dstAddress);
            } catch (Exception e) {
                LOG.warn(e.getMessage());
                return false;
            }

            lidsList.add(lid);
            NodeConnectorsList.add(sourceNodeConnectorId);

            //Add the links' info to the LinkInfo registry
            LinkInfoRegistryUtils.getInstance().writeToLinkInfoRegistry(
                    new LinkInfoRegistryEntryBuilder()
                            .setEdgeNodeConnector(sourceNodeConnectorId)
                            .setLid(lid)
                            .build());

            //Write the group which will be created to Group Registry
   /*         if (BootstrappingServiceImpl.groupsFlag == true){
                if (!dstNodeId.contains("host")) {
                    Integer port = findAlternativePort(sourceNodeConnectorId);
                    if (port != -1) {
                        GroupsRegistryUtils.getInstance().writeToGroupRegistry(
                                new GroupsRegistryEntryBuilder()
                                        .setEdgeNodeConnector(sourceNodeConnectorId)
                                        .setGroupId(Integer.parseInt(sourceNodeConnectorId.split(":")[1].concat(sourceNodeConnectorId.split(":")[2])))
                                        .build());
                    }
                }

            }*/

            //configure switch with the ABM rule
            new SwitchConfigurator(db).addFlowConfig(sourceNodeId, dstNodeId, sourceNodeConnectorId,
                    srcAddress, dstAddress);
        }
        return true;
    }

    /**
     * The method which removes links from the ICN topology.
     *
     * @param links The list of links to be added.
     * @see Link
     */
    public void removeLinks(List<Link> links) {
        LOG.info("Removing LIDs for {}", links);
        for (Link l : links) {
            if (!l.getLinkId().getValue().startsWith("host")) {
                String sourceNodeId = l.getSource().getSourceNode().getValue();
                String dstNodeId = l.getDestination().getDestNode().getValue();
                String sourceNodeConnectorId = l.getSource().getSourceTp().getValue();
                LOG.info("Calling remove for " + sourceNodeId + " " + sourceNodeConnectorId);
                removeLink(sourceNodeId, dstNodeId, sourceNodeConnectorId);
            }
        }
    }

    /**
     * The method which removes a single link from the ICN topology.
     *
     * @param sourceNodeId The source node id of the link.
     * @param dstNodeId The destination node id of the link.
     * @param sourceNodeConnectorId The source node connector of the link.
     */
    private void removeLink(String sourceNodeId, String dstNodeId, String sourceNodeConnectorId) {
        LOG.info("Removing link {}, {}, {}", sourceNodeId, dstNodeId, sourceNodeConnectorId);
        if (BootstrappingServiceImpl.assignedLinkIds.containsKey(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId)) {
            LOG.info("Configured, removing parameters.");

            BootstrappingServiceImpl.assignedLinkIds.remove(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId);

            //TODO check if node has more links, otherwise delete
            //NodeRegistryUtils.getInstance().deleteFromNodeRegistry(sourceNodeId);
            LinkRegistryUtils.getInstance().deleteFromLinkRegistry(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId);
            NodeConnectorRegistryUtils.getInstance().deleteFromNodeConnectorRegistry(sourceNodeConnectorId);

            List<LinkInfoRegistryEntry> allEntries = LinkInfoRegistryUtils.getInstance().readAllFromLinkInfoRegistry();
            if (allEntries != null) {
                for (LinkInfoRegistryEntry entry : allEntries){
                    if ((entry.getEdgeNodeConnector() != null) && (entry.getLid() != null)){
                        if (entry.getEdgeNodeConnector().equals(sourceNodeConnectorId)){
                            LinkInfoRegistryUtils.getInstance().deleteFromLinkInfoRegistry(entry.getLid());
                        }
                    }

                }
            }

            if (BootstrappingServiceImpl.getMultipleTablesFlag()){
                LOG.info("Just before deleting " + sourceNodeId + " " + sourceNodeConnectorId);
                new SwitchConfigurator(db).deleteFlowConfig(sourceNodeId, sourceNodeConnectorId);
            }
            else{
                flowsToRemove(sourceNodeId, sourceNodeConnectorId);
                LOG.info("Just before deleting " + sourceNodeId + " " + connectorsToRemove.size() + " connectors ");
                new SwitchConfigurator(db).deleteFlowsFromSwitches(sourceNodeId, connectorsToRemove);
                connectorsToRemove.clear();
            }
        }
    }

    /**
     * The method which specifies all the flows (including flows with combined outputs)
     * which must be removed for a specific edgeNodeConnector being deleted.
     *
     * @param sourceNodeId The source node id of the link.
     * @param sourceNodeConnectorId The source node connector of the link.
     */
    private void flowsToRemove (String sourceNodeId, String sourceNodeConnectorId){
        List<FlowRegistryEntry> allFlowEntries = FlowRegistryUtils.getInstance().readAllFromFlowRegistry();
        for (FlowRegistryEntry entry : allFlowEntries) {
            String[] splitNodeConnector = entry.getEdgeNodeConnector().split("openflow:");
            String[] elements;
            if (splitNodeConnector.length > 1)
                elements = splitNodeConnector[1].split(":");
            else
                elements = splitNodeConnector[0].split(":");
            //specify for removal any output that contains the output port of the sourceNodeConnectorId
            if (elements[0].equals(sourceNodeId.split(":")[1])){
                for (int i = 1; i < elements.length; i++){
                    if (elements[i].equals(sourceNodeConnectorId.split(":")[2])){
                        LOG.info("About to delete " +  entry.getEdgeNodeConnector());
                        connectorsToRemove.add(entry.getEdgeNodeConnector());
                        break;
                    }
                }
            }

        }
    }

    /**
     * The method which creates a 256-bit lid from position of '1'.
     *
     * @param pos The position of '1'.
     * @return The generated link id.
     */
    public static String generateLid(int pos) {
        String lid = "";
        for (int i = 0; i < 256; i++) {
            if (i == pos)
                lid += "1";
            else
                lid += "0";
        }
        LOG.info("Generated LID {}", lid);
        return lid;
    }

    /**
     * The method which generates a full-form IPv6 address from a LID.
     *
     * @param lid The link id to be translated to IPv6 address.
     * @return The IPv6 address to be configured as arbitrary bitmask rule.
     */
    private String lidToIpv6(String lid) throws UnknownHostException {
        byte[] ipv6Bytes = new byte[16];
        for (int i = 0; i < 16; i++) {
            String byteString = lid.substring(i * 8, i * 8 + 8);
            String bs = new StringBuilder(byteString).reverse().toString();
            ipv6Bytes[i] = (byte) Integer.parseInt(bs, 2);
        }
        String ipv6_hex = Inet6Address.getByAddress(ipv6Bytes).getHostAddress();
        return fullFormIpv6(ipv6_hex);
    }

    /**
     * The method which generates a full form IPv6 address from a given short-form one,
     * e.g from ::4 is mapped to 0000:0000:0000:...:0004
     *
     * @param shortIpv6 The short IPv6 address.
     * @return The long IPv6 address.
     */
    private static String fullFormIpv6(String shortIpv6) {
        String fullIpv6 = "";
        String[] strings = shortIpv6.split(":");
        for (int i = 0; i < strings.length; i++) {
            String full = String.format("%4s", strings[i]).replace(' ', '0');
            fullIpv6 += full;
            if (i != 7) {
                fullIpv6 += ":";
            }
        }
        return fullIpv6;
    }

    /**
     * The method which generates a LID from a full-form IPv6 address.
     *
     * @param ipv6 The IPv6 address to be translated to LID.
     * @return The LID corresponding to an IPv6.
     */
    public static String ipv6ToLid(String ipv6) {
        String lid = "";
        String[] ipv6Bytes = ipv6.split(":");
        for (int i = 0; i < ipv6Bytes.length; i++){
            int j = Integer.parseInt(ipv6Bytes[i], 16);
            String bin = Integer.toBinaryString(j);
            String temp = "";
            int a = bin.length();
            while (a < 16){
                temp += "0";
                a++;
            }
            String finalByte = temp+bin;
            lid += finalByte;
        }
        return lid;
    }

    /**
     * The method which calculates the TMFID for a host with given MAC address
     * to reach the TM.
     * It assumes that TM is host:00:00:00:00:00:01. To be changed to not-hardcoded value.
     *
     * @param hostMac The mac address of the node which is added to the network and requires the FID to the TM.
     * @return The calculated TMFID.
     */
    public String calculateTMFID(String hostMac) {
        String tmfid = "";
        List<Link> shortestPath = NetworkGraphImpl.getInstance().getShortestPath(hostMac,
                tmOpenflowId);
        List<Integer> pathLids = new ArrayList<>();
        //Add TM internal LID always
        pathLids.add(tmInternalLidPosition);
        for (Link l : shortestPath) {
            String srcNode = l.getSource().getSourceNode().getValue();
            String dstNode = l.getDestination().getDestNode().getValue();
            String sourceNodeConnectorId = l.getSource().getSourceTp().getValue();
            int pos;
            if (BootstrappingServiceImpl.assignedLinkIds.containsKey(srcNode + "," + dstNode + "," + sourceNodeConnectorId)) {
                String lid = BootstrappingServiceImpl.assignedLinkIds.get(srcNode + "," + dstNode + "," + sourceNodeConnectorId);
                pos = lidBitPosition(lid);
                if (pos != -1)
                    pathLids.add(pos);
                LOG.info("Node connector {}, pos {}", srcNode + "," + dstNode, pos);
            }
        }

        LOG.info("TMFID lids {}", pathLids);

        for (int i = 0; i < 256; i++) {
            if (pathLids.contains(i))
                tmfid += "1";
            else
                tmfid += "0";
        }
        LOG.info("Generated TMFID {}", tmfid);
        return tmfid;
    }

    /**
     * The method which finds the position of '1' bit from a given LID.
     * @param lid The provided link id.
     * @return The position of '1' bit.
     */
    private int lidBitPosition(String lid) {
        char[] chararray = lid.toCharArray();
        for (int i = 0; i < chararray.length; i++) {
            if (chararray[i] == '1')
                return i;
        }
        return -1;
    }

    /**
     * The method which configures a single link manually.
     *
     * @param switchId The switch id
     * @param portId The port id
     * @param lidPosition The position of the link id which is configured.
     */
    public void addLinkManually(String switchId, String portId, int lidPosition) {
        LOG.info("Adding link manually for {}, {}, {}", switchId, portId, lidPosition);
        String genlid = generateLid(lidPosition);

        String lid = new StringBuilder(genlid).reverse().toString();
        String downLid = lid.substring(0, 128);
        String upLid = lid.substring(128, 256);
        String srcAddress = "";
        String dstAddress = "";
        try {
            srcAddress = lidToIpv6(downLid);
            dstAddress = lidToIpv6(upLid);
            LOG.info("Src and dst ipv6 addresses {} and {}", srcAddress, dstAddress);
        } catch (Exception e) {
            LOG.warn(e.getMessage());
        }
        //configure switch with the ABM rule
        Node dstNode = getNodeFromIpAddress(dstAddress);
        String dstNodeId = dstNode.getId().getValue();
        new SwitchConfigurator(db).addFlowConfig(switchId, dstNodeId, portId, srcAddress, dstAddress);
    }

    /**
     * The method which finds the switch which corresponds to an IP address.
     *
     * @param ipAddress An IP address.
     * @return The corresponding node.
     */
    public Node getNodeFromIpAddress(String ipAddress) {
        LOG.info("Finding node with IP address {}. ", ipAddress);
        try {
            List<Node> nodeList = new ArrayList<>();
            InstanceIdentifier<Nodes> nodesIid = InstanceIdentifier.builder(
                    Nodes.class).build();
            ReadOnlyTransaction nodesTransaction = db.newReadOnlyTransaction();
            CheckedFuture<com.google.common.base.Optional<Nodes>, ReadFailedException> nodesFuture = nodesTransaction
                    .read(LogicalDatastoreType.OPERATIONAL, nodesIid);
            com.google.common.base.Optional<Nodes> nodesOptional = com.google.common.base.Optional.absent();
            nodesOptional = nodesFuture.checkedGet();

            if (nodesOptional != null && nodesOptional.isPresent()) {
                nodeList = nodesOptional.get().getNode();
            }

            if (nodeList != null) {
                for (Node n : nodeList) {
                    List<NodeConnector> nodeConnectors = n.getNodeConnector();
                    for (NodeConnector nc : nodeConnectors) {

                        AddressCapableNodeConnector acnc = nc
                                .getAugmentation(AddressCapableNodeConnector.class);
                        if (acnc != null && acnc.getAddresses() != null) {
                            // get address list from augmentation.
                            List<Addresses> addresses = acnc.getAddresses();
                            for (Addresses address : addresses) {
                                LOG.info(
                                        "Checking address {} for connector {}",
                                        address.getIp().getIpv4Address()
                                                .getValue(), nc.getId()
                                                .getValue());
                                if (address.getIp().getIpv4Address().getValue()
                                        .equals(ipAddress))
                                    return n;
                            }
                        }

                    }
                }
            }
        } catch (Exception e) {
            LOG.info("IP address reading failed");
        }
        return null;
    }

    /**
     * The method which adds links to the ICN topology, using the multiple tables approach
     * (for each switch, one flow per table).
     * If bootstrapping application is active, then for each link in the list,
     * it calls the relevant method.
     * Otherwise it adds them to the to-be-configured list.
     *
     * @param links The list of links to be added.
     * @see Link
     */

    public void addLinksMultipleTables(List<Link> links) {
        if (BootstrappingServiceImpl.applicationActivated) {
            LOG.info("Application active, hence generating LIDs for {}", links);
            Integer tableNum = 0;
            Integer tablesCount = links.size();
            for (Iterator<Link> iter = links.listIterator(); iter.hasNext(); ) {
                Link l = iter.next();
                if (!l.getLinkId().getValue().startsWith("host")) {
                    String sourceNodeId = l.getSource().getSourceNode().getValue();
                    String sourceNodeConnectorId = l.getSource().getSourceTp().getValue();
                    String dstNodeId = l.getDestination().getDestNode().getValue();

                    LOG.info("Just call add link for " + sourceNodeId + " " + sourceNodeConnectorId);
                    LOG.info("About to add link " + l.getLinkId().getValue());
                    boolean linkAdded = addLinkMultipleTables(sourceNodeId, dstNodeId, sourceNodeConnectorId, tableNum, tablesCount, l.getLinkId().getValue());
                    tableNum++;

                    if (linkAdded)
                        iter.remove();
                }
            }
        }
        else {
            LOG.info("Application not active, hence adding {} to unconfigured.", links);
            BootstrappingServiceImpl.unconfiguredLinks.addAll(links);
            BootstrappingServiceImpl.allLinks.addAll(links);
        }
    }

    /**
     * The method which adds a single link to the ICN topology.
     * If it is not configured already, then it prepares a resource request,
     * sends it to the resource manager and prepares the arbitrary bitmask
     * rule and configures it to the switch, in a specific table
     * (incrementing by one for each link of the same switch).
     * It finally adds the assigned ids to the registry.
     *
     * @param sourceNodeId The source node id of the link.
     * @param dstNodeId The destination node id of the link.
     * @param sourceNodeConnectorId The source node connector of the link.
     * @return True or false, depending on the outcome of operation.
     */
    private boolean addLinkMultipleTables(String sourceNodeId, String dstNodeId, String sourceNodeConnectorId, Integer tableNum, Integer tablesCount, String odlLinkID) {
        LOG.info("Adding link {}, {}, {}", sourceNodeId, dstNodeId, sourceNodeConnectorId);

        if (!BootstrappingServiceImpl.assignedLinkIds.containsKey(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId)) {
            for (String tableFreed : tablesFreed) {
                String[] tableFreedParts = tableFreed.split(",");
                if (tableFreedParts[0].equals(sourceNodeId.split(":")[1]) && tableFreedParts[2].equals(sourceNodeConnectorId.split(":")[2])) {
                    tableNum = Integer.parseInt(tableFreedParts[1]);
                }
            }
        }

        if (FlowRegistryUtils.getInstance().readFromFlowRegistry(sourceNodeConnectorId) == null) {
            LOG.info("Writing to FlowRegistry connector " + sourceNodeConnectorId + " for switch " + sourceNodeId + " in table " + tableNum);
            //Add the flow which will be inserted to the Flows registry
            FlowRegistryUtils.getInstance().writeToFlowRegistry(
                    new FlowRegistryEntryBuilder()
                            .setEdgeNodeConnector(sourceNodeConnectorId)
                            .setEdgeSwitch(sourceNodeId)
                            .setTableNum(tableNum)
                            .build());
        }

        if (!BootstrappingServiceImpl.assignedLinkIds.containsKey(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId)) {

            LOG.info("Link " + sourceNodeConnectorId + " not configured, generating parameters.");

            //if link is not configured, then request resources from resource manager
            List<TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest> requests =
                    new ArrayList<>();
            int nodeConnectorId = Integer.parseInt(sourceNodeConnectorId.split(":")[2]);
            requests.add(TmSdnMessages.TmSdnMessage.ResourceRequestMessage.RecourceRequest.newBuilder()
                    .setSrcNode(sourceNodeId)
                    .setDstNode(dstNodeId)
                    .setNodeConnector(nodeConnectorId)
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

            LOG.debug("Received resource offer {}", resourceOffer);
            if (resourceOffer == null || resourceOffer.getOffersList().size() == 0)
                return false;
            LOG.debug("Received the node link information {}: ", resourceOffer.getOffersList().get(0));
            String nodeid = resourceOffer.getOffersList().get(0).getNid();
            String genlid = resourceOffer.getOffersList().get(0).getLid();

            BootstrappingServiceImpl.assignedNodeIds.put(sourceNodeId, nodeid);
            BootstrappingServiceImpl.assignedLinkIds.put(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId, genlid);
            LOG.info("Generated lid for source {} and dst {} is {} ", sourceNodeId, dstNodeId, genlid);

            //add given resources to the registries for future need
            NodeRegistryUtils.getInstance().writeToNodeRegistry(new NodeRegistryEntryBuilder()
                    .setNoneName(sourceNodeId)
                    .setNoneId(nodeid)
                    .build());
            LinkRegistryEntry linkRegistryEntry = LinkRegistryUtils.getInstance().readFromLinkRegistry(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId);
            if (linkRegistryEntry != null) {
                LinkRegistryUtils.getInstance().deleteFromLinkRegistry(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId);
            }
            LinkRegistryUtils.getInstance().writeToLinkRegistry(new LinkRegistryEntryBuilder()
                    .setLinkName(sourceNodeId + "," + dstNodeId + "," + sourceNodeConnectorId)
                    .setLinkId(genlid)
                    .setOdlLinkID(odlLinkID)
                    .build());
            NodeConnectorRegistryUtils.getInstance().writeToNodeConnectorRegistry(
                    new NodeConnectorRegistryEntryBuilder()
                            .setNoneConnectorName(sourceNodeConnectorId)
                            .setSrcNode(sourceNodeId)
                            .setDstNode(dstNodeId)
                            .build());

            String lid = new StringBuilder(genlid).reverse().toString();
            String downLid = lid.substring(0, 128);
            String upLid = lid.substring(128, 256);
            String srcAddress = "";
            String dstAddress = "";
            try {
                srcAddress = lidToIpv6(downLid);
                dstAddress = lidToIpv6(upLid);
                LOG.info("Src and dst ipv6 addresses {} and {}", srcAddress, dstAddress);
            } catch (Exception e) {
                LOG.warn(e.getMessage());
                return false;
            }

            //Write the group which will be created to Group Registry
/*            if (BootstrappingServiceImpl.groupsFlag == true){
                if (!dstNodeId.contains("host")) {
                    Integer port = findAlternativePort(sourceNodeConnectorId);
                    if (port != -1) {
                        GroupsRegistryUtils.getInstance().writeToGroupRegistry(
                                new GroupsRegistryEntryBuilder()
                                        .setEdgeNodeConnector(sourceNodeConnectorId)
                                        .setGroupId(Integer.parseInt(sourceNodeConnectorId.split(":")[1].concat(sourceNodeConnectorId.split(":")[2])))
                                        .build());
                    }
                }

            }*/

            new SwitchConfigurator(db).addFlowConfigMultipleTables(sourceNodeId, dstNodeId, sourceNodeConnectorId,
                    srcAddress, dstAddress, tableNum, tablesCount);
        }
        return true;
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
     * The method which combines two or more lids into one.
     *
     * @param posList A list of indicators for lids to be combined.
     * @return The combined lid.
     */
    public static String combineLids(List<Integer> posList) {
        String lid = "";

        for (Integer pos: posList){
            LOG.info("POS " + pos);
        }
        for (int i = 0; i < 256; i++) {
            boolean flag = false;
            for (Integer pos : posList){
                if (i == pos){
                    lid += "1";
                    flag = true;
                }
            }
            if (flag == false)
                lid += "0";
        }

        LOG.info("Generated LID {}", lid);
        return lid;
    }

}
