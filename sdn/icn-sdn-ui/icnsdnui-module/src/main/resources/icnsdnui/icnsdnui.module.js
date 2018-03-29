define(['angularAMD', 'app/routingConfig', 'app/core/core.services','Restangular', 'common/config/env.module'], function(ng) {

  /*
   *Code added by elkoks. Prototype functions for Array and String objects to find largest match of a regular expression and replace it
   */
  Array.prototype.largestMatchIndex = function() {
     max = 0;
     maxIndex = 0;
     for (i = 0; i < this.length; i++) {
        if (this[i].matchSize >= max) {
          max = this[i].matchSize;
          maxIndex = this[i].matchIndex;
        }
     }
     return maxIndex;
  };

  String.prototype.replaceLargest = function(regexp, alt) {
    var indices = [];
    var currIndex = 0;
    var currString = this;

     while ((match = regexp.exec(currString)) !== null) {
        indices.push(
           {
            fullMatch: match[0],
            matchIndex: currIndex + currString.indexOf(match[0]),
            matchSize: match[0].toString().length 
          }
        );
        currIndex += currString.indexOf(match[0]) + match[0].length;
        currString = currString.substring(currString.indexOf(match[0]) + match[0].length);
      }
 
   //indices contains position of regexps
   mostImportantIndex = indices.largestMatchIndex();
  
   return this.substr(0, mostImportantIndex) + this.substr(mostImportantIndex, this.length).replace(regexp, alt);
  };

  var topology = angular.module('app.icnsdnui', ['ui.router.state','app.core','restangular', 'config']);

  topology.config(function($stateProvider, $translateProvider, NavHelperProvider) {

    NavHelperProvider.addControllerUrl('app/icnsdnui/icnsdnui.controller');
    NavHelperProvider.addToMenu('icnsdnui', {
      "link": "#/icnsdnui",
      "title": "ICN-SDN",
      "active": "main.icnsdnui",
      "icon": "icon-spinner",
      "page": {
        "title": "ICN-SDN",
        "description": "ICN-SDN"
      }
    });

    var access = routingConfig.accessLevels;
    $stateProvider.state('main.icnsdnui', {
      url: 'icnsdnui',
      access: access.public,
      views : {
        'content' : {
          templateUrl: 'src/app/icnsdnui/icnsdnui.tpl.html',
          controller: 'IcnCtrl'
        }
      }
    });

  });

  return topology;
});
