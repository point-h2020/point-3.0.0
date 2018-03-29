/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.bootstrapping.impl.topology;

import eu.point.bootstrapping.impl.BootstrappingServiceImpl;
import eu.point.bootstrapping.impl.icn.IcnIdConfigurator;
import eu.point.bootstrapping.impl.switches.SwitchConfigurator;
import eu.point.client.Client;
import eu.point.registry.impl.FlowRegistryUtils;
import eu.point.registry.impl.LinkRegistryUtils;
import eu.point.registry.impl.NodeConnectorRegistryUtils;
import eu.point.registry.impl.NodeRegistryUtils;
import eu.point.tmsdn.impl.TmSdnMessages;
import org.opendaylight.controller.md.sal.binding.api.DataChangeListener;
import org.opendaylight.controller.md.sal.common.api.data.AsyncDataChangeEvent;
import org.opendaylight.yang.gen.v1.urn.eu.point.bootstrapping.rev150722.BootstrappingService;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.NodeRegistry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.flow.registry.FlowRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.link.registry.LinkRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.connector.registry.NodeConnectorRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.registry.NodeRegistryEntry;
import org.opendaylight.yang.gen.v1.urn.eu.point.registry.rev150722.node.registry.NodeRegistryEntryBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.flow.service.rev130819.SalFlowService;
import org.opendaylight.yang.gen.v1.urn.opendaylight.inventory.rev130819.nodes.Node;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.topology.Link;
import org.opendaylight.yangtools.yang.binding.DataObject;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;

/**
 * The class which listens for topology changes.
 *
 * @author George Petropoulos
 * @version cycle-2
 */
public class TopologyListener implements DataChangeListener {

    private static final Logger LOG = LoggerFactory.getLogger(TopologyListener.class);
    private SalFlowService salFlowService;
    public static List<List<Link>> groupedLinks = new ArrayList<>();
    int i = 0;
    /**
     * The constructor class.
     */
    public TopologyListener() {
        LOG.info("Initializing topology listener.");
    }

    /**
     * The method which monitors changes in data store for topology.
     * @param dataChangeEvent The data change event, including the new, updated or deleted data.
     */
    @Override
    public void onDataChanged(AsyncDataChangeEvent<InstanceIdentifier<?>, DataObject> dataChangeEvent) {
        List<Link> linkList;
        if (dataChangeEvent == null)
            return;

        Map<InstanceIdentifier<?>, DataObject> createdData = dataChangeEvent.getCreatedData();
        Set<InstanceIdentifier<?>> removedPaths = dataChangeEvent.getRemovedPaths();
        Map<InstanceIdentifier<?>, DataObject> originalData = dataChangeEvent.getOriginalData();

        if (createdData != null && !createdData.isEmpty()) {
            Set<InstanceIdentifier<?>> linksIds = createdData.keySet();
            linkList = new ArrayList<>();

            for (InstanceIdentifier<?> linkId : linksIds) {
                if (Link.class.isAssignableFrom(linkId.getTargetType())) {
                    Link link = (Link) createdData.get(linkId);
                 /*   if (BootstrappingServiceImpl.deploymentToolBootstrapping) {
                        NodeRegistryEntry sourceNodeRegistryEntry = NodeRegistryUtils.getInstance().readFromNodeRegistry(link.getSource().getSourceNode().getValue());
                        NodeRegistryEntry destNodeRegistryEntry = NodeRegistryUtils.getInstance().readFromNodeRegistry(link.getSource().getSourceNode().getValue());
                        if (sourceNodeRegistryEntry == null){
                            NodeRegistryUtils.getInstance().writeToNodeRegistry(new NodeRegistryEntryBuilder()
                                    .setNoneId(null)
                                    .setNoneName(link.getSource().getSourceNode().getValue())
                            .build());
                        }
                        if (destNodeRegistryEntry == null){
                            NodeRegistryUtils.getInstance().writeToNodeRegistry(new NodeRegistryEntryBuilder()
                                    .setNoneId(null)
                                    .setNoneName(link.getDestination().getDestNode().getValue())
                                    .build());
                        }
                        LinkRegistryEntry linkRegistryEntry = LinkRegistryUtils.getInstance().readFromLinkRegistry(link.getSource().getSourceNode().getValue() + "," + link.getDestination().getDestNode().getValue() + "," + link.getSource().getSourceTp().getValue());
                        if (linkRegistryEntry == null) {
                            LinkRegistryUtils.getInstance().writeToLinkRegistry(new LinkRegistryEntryBuilder()
                                    .setLinkName(link.getSource().getSourceNode().getValue() + "," + link.getDestination().getDestNode().getValue() + "," + link.getSource().getSourceTp().getValue())
                                    .setLinkId(null)
                                    .setOdlLinkID(link.getLinkId().getValue())
                                    .build());
                        }
                    }*/
                    LOG.info("Graph is updated! Added Link {}", link.getLinkId().getValue());
                    linkList.add(link);
                }
            }
            NetworkGraphImpl.getInstance().addLinks(linkList);

            // if bootstrapping through deployment tool, then ignore functions below, as they are not needed
            if (BootstrappingServiceImpl.deploymentToolBootstrapping) {
                return;
            }

            if (BootstrappingServiceImpl.getMultipleTablesFlag()) {
                //we have multiple tables
                if (BootstrappingServiceImpl.applicationActivated) {
                    //means that it is not the first time
                    for (Link link : linkList) {
                        if (!BootstrappingServiceImpl.unconfiguredLinks.contains(link)) {
                            BootstrappingServiceImpl.unconfiguredLinks.add(link);
                        }
                    }
                    groupLinks(BootstrappingServiceImpl.unconfiguredLinks);
                    for (List<Link> links : groupedLinks) {
                        IcnIdConfigurator.getInstance().addLinksMultipleTables(links);
                    }
                } else {
                    IcnIdConfigurator.getInstance().addLinksMultipleTables(linkList);
                }
            } else {
                //we have single tables
                if (BootstrappingServiceImpl.applicationActivated) {
                    //means that it is not the first time;
                    for (Link link : linkList) {
                        if (!BootstrappingServiceImpl.unconfiguredLinks.contains(link)) {
                            BootstrappingServiceImpl.unconfiguredLinks.add(link);
                        }
                    }

                    groupLinks(BootstrappingServiceImpl.unconfiguredLinks);
                    int i = 1;
                    for (List<Link> links : groupedLinks) {
                        IcnIdConfigurator.getInstance().addLinks(links);
                        i++;
                    }
                } else {
                    IcnIdConfigurator.getInstance().addLinks(linkList);
                }
            }
        }

        if (removedPaths != null && !removedPaths.isEmpty() && originalData != null && !originalData.isEmpty()) {
            linkList = new ArrayList<>();
            for (InstanceIdentifier<?> instanceId : removedPaths) {
                if (Link.class.isAssignableFrom(instanceId.getTargetType())) {
                    Link link = (Link) originalData.get(instanceId);
                    LOG.info("Graph is updated! Removed Link {}", link.getLinkId().getValue());
                    linkList.add(link);
                }
            }

            List<Link> tempList = new ArrayList<>();
            for (Link link : linkList)
                tempList.add(link);

            NetworkGraphImpl.getInstance().removeLinks(linkList);

            if (!BootstrappingServiceImpl.deploymentToolBootstrapping) {
                BootstrappingServiceImpl.unconfiguredLinks.removeAll(linkList);
                IcnIdConfigurator.getInstance().removeLinks(linkList);
            }


        }
        groupedLinks.clear();
        i++;
    }


    /**
     * The method which groups a list of links to smaller lists according to
     * the switch which they belong to.
     *
     * @param linksToGroup The list of all the links to be grouped.
     */
    public void groupLinks(List<Link> linksToGroup){
        //group the links according to source switch (which will be configured for this link)
        LOG.info("Will group " + linksToGroup.size());
        for (Link link : linksToGroup){

            if (!link.getLinkId().getValue().startsWith("host")) {

                if (groupedLinks.size() > 0) {
                    boolean flag = false;
                    for (List<Link> list : groupedLinks){
                        if ( (list != null) && (!list.isEmpty()) && (list.size() > 0)){
                            if (list.get(0).getLinkId().getValue().split(":")[1].equals(link.getLinkId().getValue().split(":")[1])){
                                list.add(link);
                                flag = true;
                            }
                        }

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

        LOG.info("Grouped " + groupedLinks.size());
    }

}