## Synopsis

rv_to_moly pulls information about the rendezvous system state, and reports the state consumption
periodically through MOLY. The blackadder deployment has to be done including "--rvinfo", which
enables access to the RV data structures.

For parsing the JSON data from the RV, a copy of the rapidjson header library is needed in
include/rapidjson (or other location after modifying the Makefile). The code has been tested with
library version 1.1.0.

Compiling with -DRV_MOLY_DEBUG enables further debugging information to be printed to stdout,
including full JSON dump of the RV information structures.
