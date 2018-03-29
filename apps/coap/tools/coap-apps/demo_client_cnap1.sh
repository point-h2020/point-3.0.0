#!/bin/bash
#GET a measurment for a specific sensor
coap-client -P 172.16.191.2 -T abcd coap://sensor1.floor1.west.building6/a/A0
coap-client -P 172.16.191.2 -T abcd coap://sensor1.floor1.east.building6/a/A0
coap-client -P 172.16.191.2 -T abcd coap://sensor1.floor2.west.building6/a/A0
coap-client -P 172.16.191.2 -T abcd coap://sensor1.floor2.east.building6/a/A0
coap-client -P 172.16.191.2 -T abcd coap://sensor1.floor3.west.building6/a/A0
coap-client -P 172.16.191.2 -T abcd coap://sensor1.floor3.east.building6/a/A0

#Group communication
coap-client -P 172.16.191.2 -T abcd -m PUT coap://building6/a/D2 -e 1
coap-client -P 172.16.191.2 -T abcd -m PUT coap://floor1.building6/a/D2 -e 1
coap-client -P 172.16.191.2 -T abcd -m PUT coap://floor2.building6/a/D2 -e 1
coap-client -P 172.16.191.2 -T abcd -m PUT coap://floor3.building6/a/D2 -e 1
coap-client -P 172.16.191.2 -T abcd -m PUT coap://west.building6/a/D2 -e 1
coap-client -P 172.16.191.2 -T abcd -m PUT coap://east.building6/a/D2 -e 1

#Coap Observe
coap-client -s 5 -P 172.16.191.2 -T abcd coap://sensor1.floor1.east.building6/a/A1

