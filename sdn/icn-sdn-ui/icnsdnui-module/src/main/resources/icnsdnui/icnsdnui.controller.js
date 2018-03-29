define(['app/icnsdnui/icnsdnui.module','app/icnsdnui/icnsdnui.services', 'app/icnsdnui/icnsdnui.directives'], function(topology, service) {

  topology.controller('IcnCtrl', ['$scope', '$rootScope', 'NetworkTopologySvc', 'BootstrappingSvc', '$timeout' ,  function ($scope, $rootScope, NetworkTopologySvc, BootstrappingSvc, $timeout) {
    $rootScope['section_logo'] = '/src/app/icnsdnui/logo_icnsdnui.gif';
    var graphRenderer = null;
    var subscriberStarted = false;

    var bootstrappingStarted = false;
    var serviceStopped = false;
    $scope.receivedMessages = "";
    $scope.bootstrappingComplete = false;

    $scope.createTopology = function() {

        NetworkTopologySvc.getNode("flow:1", function(data) {
          $scope.networkData = data;
      });
    };

    $scope.initBootstrapping = function() {
        if (NetworkTopologySvc.initDone == "true") {
            console.log("Bootstrapping application already initiated. Doing nothing.");
            return;
        }
        BootstrappingSvc.activateBootstrapping(function () {
            NetworkTopologySvc.initDone = "true";
            $scope.createTopology();
        });
    };

    $scope.initMonitoring = function() {
        BootstrappingSvc.startMonitoring(function () {
            console.log("Monitoring application activated.");
        });
    };

    $scope.stopMonitoring = function() {
        BootstrappingSvc.stopMonitoring(function () {
            console.log("Monitoring application de-activated.");
        });
    };

    $scope.createTopology();

  }]);
});
