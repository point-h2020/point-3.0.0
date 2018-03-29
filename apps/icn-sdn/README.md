# The ICN-SDN Application

## Prerequisites

This application assumes that Blackadder is installed in the machine following the instructions in the root folder. 

Additionally, the google protobuf library needs to be installed: 
`sudo apt-get install libprotobuf-dev protobuf-compiler`.

Finally, all the processes require the execution of Click process.

## Build & execution instructions

 - To build the code: `make all`
 - To execute the ICN-SDN Application server process: `./server`. To execute the ICN-SDN Application server with arguments, run: `./server node_id host:mac_address openflow:X`, where node-id is the node identifier of the TM, mac_address the mac address of the TM and openflow:X the openflow identifier of the attached SDN switch to the TM, e.g. `./server 00000019 host:af:dd:45:44:22:3e openflow:156786896342474`
 - To delete the generated artifacts: `make clean`. 
 
 In case of protobuf compilation errors, delete the *messages/messages.pb.cc* and *messages/messages.pb.h* files and execute at *messages/* folder: `protoc -I=. --cpp_out=. messages.proto`
