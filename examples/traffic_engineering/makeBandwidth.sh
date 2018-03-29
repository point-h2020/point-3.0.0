#!/bin/bash

rm -f bandwidth_subscriber.o bandwidth_subscriber 
rm -f bandwidth_publisher.o bandwidth_publisher

echo "g++ -std=c++11 -g -Wall -O2 -c bandwidth_subscriber.cc"
g++ -std=c++11 -g -Wall -O2 -c bandwidth_subscriber.cc &
echo "g++ -std=c++11 -g -Wall -O2 -c bandwidth_publisher.cc"
g++ -std=c++11 -g -Wall -O2 -c bandwidth_publisher.cc &
wait
echo "g++ -o bandwidth_subscriber bandwidth_subscriber.o -lblackadder -lpthread -lbampers -lboost_system"
g++ -o bandwidth_subscriber bandwidth_subscriber.o -lblackadder -lpthread -lbampers -lboost_system &
echo "g++ -o bandwidth_publisher bandwidth_publisher.o -lblackadder -lpthread -lbampers -lboost_system"
g++ -o bandwidth_publisher bandwidth_publisher.o -lblackadder -lpthread -lbampers -lboost_system &
wait
