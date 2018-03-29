define(['app/icnsdnui/icnsdnui.module'], function(topology) {

  topology.factory('TopologyRestangular', function(Restangular, ENV) {
    return Restangular.withConfig(function(RestangularConfig) {
      RestangularConfig.setBaseUrl(ENV.getBaseURL("MD_SAL"));
    });
  });

  topology.factory('NetworkTopologySvc', function(TopologyRestangular) {
      var svc = {
          base: function() {
              return TopologyRestangular.one('restconf').one('operational').one('network-topology:network-topology');
          },

          full: function() {
              return TopologyRestangular.one('restconf').one('operational').one('opendaylight-inventory:nodes');
          },

          bootstrapping: function() {
              return TopologyRestangular.one('restconf');
          },

          initDone: "false",

          monitoringStarted: "false",

          nodeBootstrapped: "false",
          
          LidToIpv6: function(lid) {
            hexNumbers = [];
            //Convert binary to hex array
            for (i=0; i<32; i++) {
              hexBits = lid.substring(i * 4, i * 4 + 4);
              hexNum = parseInt(hexBits, 2).toString(16);
              hexNumbers.push(hexNum);
            }
            //Revert bytes
            revertedHexNumbers = [];
            for (i=16; i > 0; i--) {
              revertedHexNumbers.push(hexNumbers[i * 2 - 2], hexNumbers[i * 2 - 1]);
            }
  
            //Group and create extended ipv6
            extendedIpv6 = "";
            for (i=0; i < 8; i++) {
              ipv6Part = revertedHexNumbers.slice(i*4, i*4 + 4).join("");
              extendedIpv6 += ipv6Part + ":";
            }
            extendedIpv6 = extendedIpv6.substring(0, extendedIpv6.length - 1);
            //Ugly but works...
            ipv6Parts = extendedIpv6.split(":");
            for (i = 0; i < ipv6Parts.length; i++) {
              if (ipv6Parts[i] === "0000") {
                ipv6Parts[i] = "0";
              } else {
                ipv6Parts[i] = ipv6Parts[i].replace(/^0*/, "");
              }
            }
            semiCompressedIpv6 = ipv6Parts.join(":");
            compressedIpv6 = semiCompressedIpv6.replaceLargest(/(^(0:)+)|(((:0)+)$)/, "::");
            return compressedIpv6;
          },

          ipv6ToLid: function(ipv6) {
            lidByteArr = [];
            splitted = ipv6.split(':').join('');
            for (i=16; i > 0; i --) {
              lidByteArr.push(parseInt(splitted.substring(2*(i-1), 2*i), 16));
            }
            result = "";
            for (i=0; i < 16; i++) {
              toConvert = lidByteArr[i];
              currByte = ['0', '0', '0', '0', '0', '0', '0', '0'];
              if (toConvert >= 128) {
                currByte[0] = '1';
                toConvert -= 128;
              }
              if (toConvert >= 64) {
                currByte[1] = '1';
                toConvert -= 64;
              }
              if (toConvert >= 32) {
                currByte[2] = '1';
                toConvert -= 32;
              }
              if (toConvert >= 16) {
                currByte[3] = '1';
                toConvert -= 16;
              }
              if (toConvert >= 8) {
                currByte[4] = '1';
                toConvert -= 8;
              }
              if (toConvert >= 4) {
                currByte[5] = '1';
                toConvert -= 4;
              }
              if (toConvert >= 2) {
                currByte[6] = '1';
                toConvert -= 2;
              }
              if (toConvert >= 1) {
                currByte[7] = '1';
              }
              result += currByte.join('');
            }
    
          return result;
          },

          data: null,
          TOPOLOGY_CONST: {
              HT_SERVICE_ID:"host-tracker-service:id",
              IP:"ip",
              HT_SERVICE_ATTPOINTS:"host-tracker-service:attachment-points",
              HT_SERVICE_TPID:"host-tracker-service:tp-id",
              NODE_ID:"node-id",
              SOURCE_NODE:"source-node",
              DEST_NODE:"dest-node",
              SOURCE_TP:"source-tp",
              DEST_TP:"dest-tp",
              ADDRESSES:"addresses",
              HT_SERVICE_ADDS:"host-tracker-service:addresses",
              HT_SERVICE_IP:"host-tracker-service:ip"
          }
      };
      svc.getCurrentData = function() {
          return svc.data;
      };
      svc.getAllNodes = function() {
          svc.data = svc.base().getList();
          return svc.data;
      };

      svc.getNodePositionFromId = function(nodeList, id) {
          for (i = 0; i < nodeList.length; i++) {
            if (nodeList[i].id === id) {
              return i;
            }
          }
          return -1;
      };

      // the method which returns the graph and all the shown information
      svc.getNode = function(node, cb) {

          // Determines the node id from the nodes array corresponding to the text passed
          var getNodeIdByText = function getNodeIdByText(inNodes, text) {
              var nodes = inNodes.filter(function(item, index) {
                      return item.label === text;
                  }),
                  nodeId;

              if(nodes && nodes[0]) {
                  nodeId = nodes[0].id;
              }else{
                  return null;
              }

              return nodeId;
          };
          // Checks if the edge is present in the links map or not so we show single edge link between switches
          var isEdgePresent = function(inLinks,srcId,dstId, srcPort, dstPort){
              if( inLinks[srcId+":"+dstId+":"+srcPort] === undefined && inLinks[dstId+":"+srcId+":"+dstPort] === undefined) {
                  return false;
              }
              else {
                  return true;
              }
          };
              var odlInvRest = TopologyRestangular.one('restconf').one('operational').one('opendaylight-inventory:nodes').get();
              var odlInv = odlInvRest["$object"];

          return svc.base().one("topology", node).get().then(function(ntData){
              var nodes = [];
              var links = [];
              var linksMap = {};

              if(ntData.topology && ntData.topology[0]){
                  //Loop over the nodes
                  angular.forEach(ntData.topology[0].node, function(nodeData) {
                    var groupType = "", nodeTitle = "", nodeLabel = "";
                    var nodeId = nodeData[svc.TOPOLOGY_CONST.NODE_ID];
                      if(nodeId!==undefined && nodeId.indexOf("host")>=0){
                        groupType = "host";
                        var ht_serviceadd = nodeData[svc.TOPOLOGY_CONST.ADDRESSES];
                          if(ht_serviceadd===undefined){
                              ht_serviceadd = nodeData[svc.TOPOLOGY_CONST.HT_SERVICE_ADDS];
                          }
                          if(ht_serviceadd!==undefined){
                              var ip;
                            //get title info
                            for(var i=0;i<ht_serviceadd.length;i++){
                                ip = ht_serviceadd[i][svc.TOPOLOGY_CONST.IP];
                                if(ip===undefined){
                                    ip = ht_serviceadd[i][svc.TOPOLOGY_CONST.HT_SERVICE_IP];
                                }
                                nodeTitle += 'IP: <b>' + ip + '</b><br>';
                            }
                          }
                        nodeTitle += 'Type: <b>Host</b>';
                      }
                      else {
                        groupType = "switch";
                        nodeTitle = 'Name: <b>' + nodeId + '</b><br>Type: <b>Switch</b>';
                      }

                    nodeLabel = nodeData[svc.TOPOLOGY_CONST.NODE_ID];
                    nodes.push({'id': nodes.length.toString(), 'label': nodeLabel, group: groupType,value:20,title:nodeTitle});
                  });

                  // Loops over the links
                  angular.forEach(ntData.topology[0].link, function(linkData) {
                      var srcId = getNodeIdByText(nodes, linkData.source[svc.TOPOLOGY_CONST.SOURCE_NODE]),
                          dstId = getNodeIdByText(nodes, linkData.destination[svc.TOPOLOGY_CONST.DEST_NODE]),
                          srcPort = linkData.source[svc.TOPOLOGY_CONST.SOURCE_TP],
                          dstPort = linkData.destination[svc.TOPOLOGY_CONST.DEST_TP],
                          linkId = links.length.toString();
                      if(srcId!=null && dstId!=null && !isEdgePresent(linksMap,srcId,dstId,srcPort,dstPort)){
                          links.push({id: linkId, 'from' : srcId, 'to': dstId, title:'Source Port: <b>' + srcPort + '</b><br>Dest Port: <b>'+dstPort+'</b>'});
                          linksMap[srcId+":"+dstId+":"+srcPort]=linkId;
                      }
                  });

              }
              // the topology data which creates the graph
              var data = {
                  "nodes" : nodes,
                  "links" : links
              };

            // This piece of code attempts to receive information from the opendaylight inventory using the REST API.
            // If this fails, node/link data remains as if this contribution did not exist
            // and the topology diagram is created.
            // If information about the link status is received, the functionality of the links
            // in the "data" object is checked using the opendaylight-inventory information.
            // In case a link is blocked or down, that link is removed from the "data.links" array.
            // Traffic information is added to the link label (currently number of bytes transmitted/received).

              var myObj = svc.full().get().then(function(ntData) {
                  console.log("All data successfully received from opendaylight inventory.");
                  console.log(ntData);

                  var flowTraffic = {};
                  console.log("Flow traffic extracted from JSON.");

                // Get bootstrapped node IDs and fill nodeIDs object accordingly.
                // if bootstrapping is completed, then update nodes and links with
                // assigned identifiers.
                  var nodeIDs = {};
                  if (svc.initDone == "true") {
                      svc.bootstrapping().one("operational").one("registry:node-registry").get().then(function(nodeRegistry) {
                          console.log("Successfully received nodes from node registry."); 
                          angular.forEach(nodeRegistry["node-registry"]["node-registry-entry"], function(entry) {
                              nodeIDs[entry.noneName] = entry.noneId;
                          });
                        // Return with (modified if necessary) data.
                          if (data.nodes) {
                              angular.forEach(data.nodes, function(n, key) {
                                  if (nodeIDs[n.label]) {
                                      n.title += "<br/><b>Node ID: " + nodeIDs[n.label] + "</b>";
                                  }
                              });

                             // Remove openflow:4 if existing
                             // added for demo scenario
                             // todo to be removed
                              angular.forEach(data.nodes, function(n, key) {
                                  if (n.label == "openflow:4") {
                                      data.nodes.splice(key, 1);
                                  }
                              });                      
                          }

                        // Get link registry information then
                          svc.bootstrapping().one("operational").one("registry:link-registry").get().then(function(linkData) {
                              var linkInfo = [];
                              angular.forEach(linkData["link-registry"]["link-registry-entry"], function(link) {
                                  endpoints = link.linkName.split(",");
                                  linkId = link.linkId;
                                  linkInfo.push({endpoints: endpoints, linkID: linkId});
                              });

                            // Compare with data and update appropriately
                              angular.forEach(data.links, function(link) {
                                  linkEndpoints = [data.nodes[parseInt(svc.getNodePositionFromId(data.nodes, link.from))].label,
                                  data.nodes[parseInt(svc.getNodePositionFromId(data.nodes, link.to))].label];

                                // Find link registry information to update data link
                                  var linkICNBytes = 0;
                                  angular.forEach(linkInfo, function(linfo) {
                                      if (((linfo.endpoints[0] == linkEndpoints[0]) && (linfo.endpoints[1] == linkEndpoints[1]))
                                      || ((linfo.endpoints[1] == linkEndpoints[0]) && (linfo.endpoints[0] == linkEndpoints[1]))) {
                                          link.title += "<br/>Link ID: <b>" + linfo.linkID.replaceLargest(/00000+/g, "00...0") + "</b>";
                                          if ((ipv6Repr = svc.LidToIpv6(linfo.linkID.substring(0, 128))) != "::0") {
                                            link.title += "<br/>ABM rule: <b>" + ipv6Repr + "/" + ipv6Repr + " (dst)</b>";
                                          } else if ((ipv6Repr = svc.LidToIpv6(linfo.linkID.substring(128, 256))) != "::0") {
                                            link.title += "<br/>ABM rule: <b>" + ipv6Repr + "/" + ipv6Repr + " (src)</b>";
                                          }
                                      }
                                  });
                            });
                            cb(data); 
                          }, function(err) {
                              cb(data);
                              console.log("Failed to get data from link-registry using REST API. Response: " + err.status);
                          });                     
                      }, function(err) {
                          console.log("Failed to get data from node-registry using REST API. Response: " + err.status);
                      });
                  } else {
                      console.log("Bootstrapping not initiated. Proceeding as usually.");
                      //Remove openflow:4 (management switch)
                      if (data.nodes) {
                          angular.forEach(data.nodes, function(n, key) {
                              if (n.label == "openflow:4") {
                                data.nodes.splice(key, 1);
                              }
                          });
                      }                
                      cb(data);                
                  }
              }, 
              function(err) {
                  console.log("Failed to get full data from opendaylight inventory using REST API. Response: " + err.status);

                //Return without modifying any data
                  if (data.nodes) {
                      angular.forEach(data.nodes, function(n, key) {
                          if (n.label == "openflow:4") {
                              data.nodes.splice(key, 1);
                          }
                      });
                  }                
                  cb(data);
              });
          },function(response) {
              console.log("Error with status code", response.status);
          });
      };
      return svc;
  });

  topology.factory('BootstrappingSvc', function(TopologyRestangular) {
    // default object
      var svc = {
            bootstrappingStarted: "false",
            hostBootstrapped: "false",
            subscriberStarted: "false",
            // base REST API function
            bootstrapping: function() {
                return TopologyRestangular.one('restconf');
            }
      };

    // function which activates bootstrapping
      svc.activateBootstrapping = function(cb) {
          svc.bootstrapping().one("operations").post("bootstrapping:activateApplication",
              {
                  input: {
                      status: "true"
                  }
              }
              ).then(function(posted) {
                  console.log("Successfully sent bootstrapping application activation message to REST API.");
                  svc.bootstrappingStarted = "true";
                  cb();
              }, function(err) {
                  console.log("Error sending bootstrapping application activation message to REST API.");
              }
          );
      };

    // function which starts monitoring
      svc.startMonitoring = function(cb) {
          svc.bootstrapping().one("operations").post("monitoring:init",
              {
                  input: {
                      "trafficmonitor-enabled": true,
                      "trafficmonitor-period": 30000,
                      "linkmonitor-enabled": true
                  }
              }
              ).then(function(posted) {
                  console.log("Successfully sent monitoring application activation message to REST API.");
                  cb();
              }, function(err) {
                  console.log("Error sending monitoring application activation message to REST API.");
              }
          );
      };

    // function which stops monitoring
      svc.stopMonitoring = function(cb) {
          svc.bootstrapping().one("operations").post("monitoring:init",
              {
                  input: {
                      "trafficmonitor-enabled": false,
                      "trafficmonitor-period": 0,
                      "linkmonitor-enabled": false
                  }
              }
              ).then(function(posted) {
                  console.log("Successfully sent monitoring application de-activation message to REST API.");
                  cb();
              }, function(err) {
                  console.log("Error sending monitoring application de-activation message to REST API.");
              }
          );
      };

    // test function to update the sdn controller registry with a test node
      svc.updateRegistry = function(cb) {
          svc.bootstrapping().one("operations").post("bootstrapping:nodeLinkInformation",
              {
                  input: {
                  srcNode: "host:00:00:00:00:00:02",
                  dstNode: "openflow:3"
                  }
              }).then(function (response) {
                  console.log("Registry (should be) updated.");
                  cb();
              }, function(err) {
                  console.log("Failed to update registry.");
              });
      };   

    // test function to bootstrap a host
      svc.bootstrapHost = function(cb) {
          svc.bootstrapping().one("operations").post("bootstrapping:bootstrapNode",
              {
                  input: {
                      username: "mininet",
                      hostAddresses: "192.168.1.2,192.168.1.3"
                  }
              }
          ).then(function(posted) {
              console.log("Successfully sent bootstrap host message to REST API.");
              svc.hostBootstrapped = "true";
              svc.updateRegistry(cb);
          }, function(err) {
              console.log("Error sending bootstrap host message to REST API.");	
          });
      };

      return svc; 
  });
});
