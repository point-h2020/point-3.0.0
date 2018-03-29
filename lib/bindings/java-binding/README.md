# Prerequisites

In order to build and run the java samples:

- Install JDK and Maven
- Configure $JAVA_HOME pointing to your Java installation

# Build
Execute:

- ```$ mvn clean install```
- eu_pursuit_client_BlackadderWrapper.so will be placed in /tmp/ folder.

# Usage
- Add to your pom.xml the java-binding dependency:
```
<dependency>
  <groupId>eu.pursuit</groupId>
  <artifactId>java-binding</artifactId>
  <version>1.0</version>
</dependency>
```
- Instantiate BlackadderWrapper using the following code:
```
BlackadderWrapper.configureObjectFile("/tmp/eu_pursuit_client_BlackadderWrapper.so");
```
