### 
###  Created on: 09 July 2015
###  Author: Mohammed Al-Khalidi <mshawk@essex.ac.uk> 
### 

sink("ns3_topology.cfg")

library(graph)
library(igraph)

g <- barabasi.game(100, directed = FALSE)
g <- as_graphnel(g)

ed <- list()
XX <- list()
YY <- list()
edgenum=0;
for(i in nodes(g)) {
    for(el in graph::edges(g)[[i]]) {
        ed <- c(ed,list(c(i,el)))
        XX <-  c(XX,list(i))
        YY <- c(YY,list(el))
edgenum <- edgenum + 1 ;
    }
}
X <- as.numeric(XX)
Y <- as.numeric(YY)


n=edgenum;

z <- nchar(X)
p <- nchar(Y)

h=1;

for(i in seq(1:h)) 
 
{ 
cat("BLACKADDER_ID_LENGTH = 8;")
cat("\nLIPSIN_ID_LENGTH = 32;")
cat("\nWRITE_CONF = ")
cat("\"/tmp/\";")
cat("\n\nnetwork = {")
cat("\nnodes = (")

}
v=1;
for(i in seq(1:n)) 
 
{ 

 if ((X[i] == v) && ( v==1 ))
{

	


cat("\n{")

cat("\nrole = [];")


b=z[i];
w=p[i];

if(b == 1) {
cat("\nlabel =","\"0000000")
cat(X[i])
cat("\";")

cat("\nconnections = (")

}
if (b == 2) {
cat("\nlabel =","\"000000")
cat(X[i])
cat("\";")

cat("\nconnections = (")

}
if (b == 3) {
cat("\nlabel =","\"00000")
cat(X[i])
cat("\";")

cat("\nconnections = (")

}		
cat("\n{")
 
if(w == 1) {               
cat("\nto = ","\"0000000")
cat(Y[i])
cat("\";")
cat("\nMtu = 1500;")

}
if (w == 2) {
cat("\nto = ","\"000000")
cat(Y[i])
cat("\";")
cat("\nMtu = 1500;")

}
if (w == 3) {
cat("\nto = ","\"00000")
cat(Y[i])
cat("\";")
cat("\nMtu = 1500;")

}
cat("\nDataRate =")
cat("\"100Mbps\"")
cat(";")

cat("\nDelay = \"10ms\";")

if (X[i+1]== v)

{
cat("\n},")
 }
else 
{
cat("\n}")
} 
      
i <- i+1
	v <- v+1	
}


		
else if (X[i] == 1)

{
  w=p[i];
  cat("\n{")
 
if(w == 1) {    
cat("\nto = ","\"0000000")
cat(Y[i])
cat("\";")
}
if (w == 2) {
cat("\nto = ","\"000000")
cat(Y[i])
cat("\";")
}
if (w == 3) {
cat("\nto = ","\"00000")
cat(Y[i])
cat("\";")
}
cat("\nMtu = 1500;")
cat("\nDataRate =")

cat("\"100Mbps\"")
cat(";")
cat("\nDelay = \"10ms\";")
if(X[i + 1] == 1)


{
cat("\n},")
 
}
else if(X[i + 1] != 1) 
{
cat("\n}")

}     

i <- i+1

}


}


m =2;

for(i in seq(1:n)) 
 
{ 


 if (X[i] == m)
{

	
cat("\n);")
	
cat("\n},")

cat("\n{")

cat("\nrole = [];")


b=z[i];
w=p[i];
{
if(b == 1) {
cat("\nlabel =","\"0000000")
cat(X[i])
cat("\";")

cat("\nconnections = (")

}
if (b == 2) {
cat("\nlabel =","\"000000")
cat(X[i])
cat("\";")

cat("\nconnections = (")

}
if (b == 3) {
cat("\nlabel =","\"00000")
cat(X[i])
cat("\";")

cat("\nconnections = (")
	
}		
cat("\n{")
 
if(w == 1) {               
cat("\nto = ","\"0000000")
cat(Y[i])
cat("\";")
}
if (w == 2) {
cat("\nto = ","\"000000")
cat(Y[i])
cat("\";")

}
if (w == 3) {
cat("\nto = ","\"00000")
cat(Y[i])
cat("\";")
}

cat("\nMtu = 1500;")
cat("\nDataRate =")
cat("\"100Mbps\"")
cat(";")

cat("\nDelay = \"10ms\";")


if ((i+1) <= n)
{

if (X[i + 1] == m)  


{
cat("\n},")
 
}
else if (X[i + 1] != m )
{
cat("\n}")

} 
}
 else if ((i+1) > n)
{
cat("\n}")

}

      
i <- i+1
	m <- m+1	
}


}		
else if ((X[i] == (m -1)) && (X[i] != 1))

{
  w=p[i];
  cat("\n{")
 
if(w == 1) {    
cat("\nto = ","\"0000000")
cat(Y[i])
cat("\";")
}
if (w == 2) {
cat("\nto = ","\"000000")
cat(Y[i])
cat("\";")
}
if (w == 3) {
cat("\nto = ","\"00000")
cat(Y[i])
cat("\";")

}
cat("\nMtu = 1500;")
cat("\nDataRate =")

cat("\"100Mbps\"")
cat(";")
cat("\nDelay = \"10ms\";")


if ((i+1) <= n)
{
if(X[i + 1] == (m -1))


{
cat("\n},")
 
}
else if(X[i + 1] != (m -1)) 
{
cat("\n}")

}
}
 else if ((i+1) > n)
{
cat("\n}")

}
 
       

i <- i+1

}
}


cat("\n);")
	
cat("\n}")
cat("\n);")
	
cat("\n};")

