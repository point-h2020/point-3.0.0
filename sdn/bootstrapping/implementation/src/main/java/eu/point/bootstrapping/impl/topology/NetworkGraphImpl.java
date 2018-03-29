/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.bootstrapping.impl.topology;

import com.google.common.base.Optional;

import edu.uci.ics.jung.algorithms.shortestpath.DijkstraShortestPath;
import edu.uci.ics.jung.graph.Graph;
import edu.uci.ics.jung.graph.SparseMultigraph;
import edu.uci.ics.jung.graph.util.EdgeType;

import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.controller.md.sal.binding.api.ReadOnlyTransaction;
import org.opendaylight.controller.md.sal.common.api.data.LogicalDatastoreType;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.NetworkTopology;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.NodeId;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.TopologyId;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.link.attributes.DestinationBuilder;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.link.attributes.SourceBuilder;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.Topology;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.TopologyKey;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.topology.Link;
import org.opendaylight.yang.gen.v1.urn.tbd.params.xml.ns.yang.network.topology.rev131021.network.topology.topology.LinkBuilder;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * The class which maintains the network graph.
 *
 * @author George Petropoulos
 * @version cycle-2
 */
public class NetworkGraphImpl {

    private static final Logger LOG = LoggerFactory.getLogger(NetworkGraphImpl.class);
    private Graph<NodeId, Link> networkGraph = null;
    private Set<String> linkAdded = new HashSet<>();
    private DijkstraShortestPath<NodeId, Link> shortestPath = null;
    private static NetworkGraphImpl instance = null;
    private DataBroker db;

    private List<String> linksAlreadyAdded = new ArrayList<String>();
    private List<String> linksAlreadyRemoved = new ArrayList<String>();
    private Set<String> linkRemoved = new HashSet<>();
    private  Topology topology = null;


    /**
     * The constructor class.
     */
    public NetworkGraphImpl() {

    }

    /**
     * The method which returns the instance of the class.
     *
     * @return The singleton instance of the class.
     */
    public static NetworkGraphImpl getInstance() {
        if (instance == null) {
            instance = new NetworkGraphImpl();
        }
        return instance;
    }

    public void setDb(DataBroker db) {
        this.db = db;
    }

    public DijkstraShortestPath<NodeId, Link> getShortestPath() {
        return shortestPath;
    }

    /**
     * The method which initializes the network topology graph.
     */
    public void init() {
        LOG.info("Initializing network topology graph.");
        clearGraph();
        List<Link> links = getLinksFromTopology();
        if (links == null || links.isEmpty()) {
            return;
        }
        addLinks(links);

    }

    /**
     * The method which reads the links from the topology.
     *
     * @return The list of links of the topology.
     * @see Link
     */
    public List<Link> getLinksFromTopology() {
        LOG.info("Reading links from topology datastore.");
        InstanceIdentifier<Topology> topologyInstanceIdentifier = InstanceIdentifier.builder(NetworkTopology.class)
                .child(Topology.class, new TopologyKey(new TopologyId("flow:1")))
                .build();
        Topology topology = null;
        ReadOnlyTransaction readOnlyTransaction = db.newReadOnlyTransaction();
        try {
            Optional<Topology> topologyOptional = readOnlyTransaction.read(LogicalDatastoreType.OPERATIONAL,
                    topologyInstanceIdentifier).get();
            if (topologyOptional.isPresent()) {
                topology = topologyOptional.get();
            }
        } catch (Exception e) {
            LOG.error("Error reading topology {}", topologyInstanceIdentifier);
            readOnlyTransaction.close();
            throw new RuntimeException("Error reading from operational store, topology : "
                    + topologyInstanceIdentifier, e);
        }
        readOnlyTransaction.close();
        if (topology == null) {
            return null;
        }
        List<Link> links = topology.getLink();
        if (links == null || links.isEmpty()) {
            return null;
        }
        List<Link> internalLinks = new ArrayList<>();
        for (Link link : links) {
            //if (!(link.getLinkId().getValue().contains("host"))) {
                internalLinks.add(link);
            //}
        }

        return internalLinks;
    }

    /**
     * The method which adds links to the graph.
     *
     * @param links The list of links to be added.
     * @see Link
     */
    public synchronized void addLinks(List<Link> links) {
        LOG.info("Inside graphimpl");
        LOG.debug("Adding links {}", links);
        if (links == null || links.isEmpty()) {
            LOG.warn("Links list empty or null.");
            return;
        }

        if (networkGraph == null) {
            networkGraph = new SparseMultigraph<>();
        }

        for (Link link : links) {
            if (linkAlreadyAdded(link)) {
                continue;
            }
            NodeId sourceNodeId = link.getSource().getSourceNode();
            NodeId destinationNodeId = link.getDestination().getDestNode();
            networkGraph.addVertex(sourceNodeId);
            networkGraph.addVertex(destinationNodeId);
            networkGraph.addEdge(link, sourceNodeId, destinationNodeId, EdgeType.UNDIRECTED);
        }

        LOG.info("Updated topology graph {} ", new ArrayList<>(networkGraph.getEdges()));

        LOG.info("EDGES : " + networkGraph.getEdgeCount() + networkGraph.getEdges().toString());
        LOG.info("VERTICES : " + networkGraph.getVertexCount() + networkGraph.getVertices().toString());

        if (shortestPath == null) {
            shortestPath = new DijkstraShortestPath<NodeId, Link>(networkGraph);
        }
        else {
            shortestPath.reset();
        }
        LOG.debug("Updated shortest path.");
    }

    /**
     * The method which removes links from the graph.
     *
     * @param links The list of links to be removed.
     * @see Link
     */
    public synchronized void removeLinks(List<Link> links) {
        LOG.debug("Removing links {}", links);
        if (links == null || links.isEmpty()) {
            LOG.warn("Links list empty or null.");
            return;
        }

        if (networkGraph == null) {
            LOG.info("Network graph already empty, nothing to delete.");
            return;
        }

        for (Link link : links) {
            if (linkAlreadyDeleted(link)) {
                continue;
            }
            networkGraph.removeEdge(link);
        }

        LOG.info("Updated topology graph {} ", new ArrayList<>(networkGraph.getEdges()));

        LOG.info("EDGES : " + networkGraph.getEdgeCount() + networkGraph.getEdges().toString());
        LOG.info("VERTICES : " + networkGraph.getVertexCount() + networkGraph.getVertices().toString());

        if (shortestPath == null) {
            shortestPath = new DijkstraShortestPath<NodeId, Link>(networkGraph);
        }
        else {
            shortestPath.reset();
        }
        LOG.debug("Updated shortest path.");
    }

    /**
     * The method which checks whether a link was already deleted from the graph.
     *
     * @param link The link to be checked whether it was deleted.
     * @return True or false, depending on whether it is deleted or not.
     * @see Link
     */
    private boolean linkAlreadyDeleted(Link link) {
        String linkAddedKey = null;
        if (link.getDestination().getDestTp().hashCode() > link.getSource().getSourceTp().hashCode()) {
            linkAddedKey = link.getSource().getSourceTp().getValue()
                    + link.getDestination().getDestTp().getValue();
        } else {
            linkAddedKey = link.getDestination().getDestTp().getValue()
                    + link.getSource().getSourceTp().getValue();
        }
        if (!linkAdded.contains(linkAddedKey)) {
            return true;
        } else {
            linkAdded.remove(linkAddedKey);
            return false;
        }

    }

    /**
     * The method which checks whether a link was already added to the graph.
     *
     * @param link The link to be checked whether it was deleted.
     * @return True or false, depending on whether it is deleted or not.
     * @see Link
     */
    private boolean linkAlreadyAdded(Link link) {
        String linkAddedKey = null;
        if (link.getDestination().getDestTp().hashCode() > link.getSource().getSourceTp().hashCode()) {
            linkAddedKey = link.getSource().getSourceTp().getValue()
                    + link.getDestination().getDestTp().getValue();
        } else {
            linkAddedKey = link.getDestination().getDestTp().getValue()
                    + link.getSource().getSourceTp().getValue();
        }
        if (linkAdded.contains(linkAddedKey)) {
            return true;
        } else {
            linkAdded.add(linkAddedKey);
            return false;
        }
    }

    /**
     * The method which sorts the shortest path in terms of links order.
     *
     * @param links The list of links.
     * @param src The source node of the path to be constructed.
     * @return The shortest path in terms of list of links.
     * @see Link
     */
    public List<Link> sortShortestPathLinkList(List<Link> links, String src) {
        List<Link> sortedList = new ArrayList<>();
        LOG.debug("Received shortest path {}", links);

        if (links != null && links.size() > 0) {
            String previous = src;
            for (int i = 0; i < links.size(); i++) {

                String srcNode = links.get(i).getSource().getSourceNode().getValue();
                if (srcNode.equals(previous)) {
                    LOG.debug("Src node {} equals previous {}", srcNode, previous);
                    sortedList.add(links.get(i));
                    previous = links.get(i).getDestination().getDestNode().getValue();
                }
                else {
                    Link l = new LinkBuilder()
                            .setLinkId(links.get(i).getLinkId())
                            .setSource(new SourceBuilder()
                                    .setSourceNode(links.get(i).getDestination().getDestNode())
                                    .setSourceTp(links.get(i).getDestination().getDestTp())
                                    .build())
                            .setDestination(new DestinationBuilder()
                                    .setDestNode(links.get(i).getSource().getSourceNode())
                                    .setDestTp(links.get(i).getSource().getSourceTp())
                                    .build())
                            .build();
                    sortedList.add(l);
                    previous = l.getDestination().getDestNode().getValue();
                }
            }
        }
        LOG.debug("Sorted shortest path {}", links);
        return sortedList;
    }

    /**
     * The method which clears the graph.
     */
    public void clearGraph() {
        networkGraph = new SparseMultigraph<>();
    }

    /**
     * The method which returns the shortest path based on Dijkstra algorithm.
     *
     * @param srcNodeId The source node of the path to be constructed.
     * @param dstNodeId The destination node of the path to be constructed.
     * @return The shortest path in terms of list of links.
     * @see Link
     */
    public List<Link> getShortestPath(String srcNodeId, String dstNodeId) {
        List<Link> randomPath = shortestPath.getPath(new NodeId(srcNodeId), new NodeId(dstNodeId));
        LOG.debug("Random path between {} and {} is {}.", srcNodeId, dstNodeId, randomPath);
        List<Link> sortedPath = sortShortestPathLinkList(randomPath, srcNodeId);
        LOG.info("Shortest path is {}.", randomPath);
        return sortedPath;
    }

    public Graph<NodeId, Link> getNetworkGraph(){
        return networkGraph;
    }

}
