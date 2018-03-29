# Prerequisites

In order to build and run the java samples:
-Install Java oracle and Maven
-Configure $JAVA_HOME pointing to your Java installation
-Build the java-bindings located in /lib/bindings/java-binding

# Build and Run
Execute:
$ mvn clean install

While all relevant Blackadder processes are running, to run the publisher execute:
$ sudo java -jar target/publisher-jar-with-dependencies.jar

To run the subscriber:
$ sudo java -jar target/subscriber-jar-with-dependencies.jar

# Usage
-Add to your pom.xml the java-binding dependency:
```
<dependency>
  <groupId>eu.pursuit</groupId>
  <artifactId>java-binding</artifactId>
  <version>1.0</version>
</dependency>
```
-Instantiate BlackadderWrapper using the following code:
```
BlackadderWrapper.configureObjectFile("/tmp/eu_pursuit_client_BlackadderWrapper.so");
```
