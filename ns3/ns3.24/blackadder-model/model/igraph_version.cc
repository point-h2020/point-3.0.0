/*
 * Copyright (C) 2010-2011  Martin Reed
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 3 as published by the Free Software Foundation.
 *
 * See LICENSE and COPYING for more details.
 */
#include <iostream>
#include <igraph/igraph.h>
#include <igraph/igraph_version.h>

using namespace std;
int main(int argc, char* argv[]) {
  // versions of igraph 0.5 and earlier did not specify IGRAPH_VERSION
  int major=0;
  int minor=5;
  int sub=0;
  printf("//THIS FILE WAS AUTOMATICALLY GENERATED DO NOT EDIT, EDIT igraph_version.cpp INSTEAD\n");
  printf("#ifndef IGRAPH_VERSION_HH\n #define IGRAPH_VERSION_HH\n");
#ifdef IGRAPH_VERSION
  sscanf(IGRAPH_VERSION,"%d.%d.%d",&major,&minor,&sub);
  printf("// Detected igraph Major=%d, minor=%d, sub=%d\n",major,minor,sub);
#endif
  // Need to add more lines with new versions
  printf("#define IGRAPH_V_0_8 80\n");
  printf("#define IGRAPH_V_0_7 70\n");
  printf("#define IGRAPH_V_0_6 60\n");
  if(major==0 && minor==8 ) {
    printf("#define IGRAPH_V IGRAPH_V_0_8\n");
  } else if (major==0 && minor==7 ) {
    printf("#define IGRAPH_V IGRAPH_V_0_7\n");
  } else if (major==0 && minor==6 ) {
    printf("#define IGRAPH_V IGRAPH_V_0_6\n");
  } else {
    printf("#define IGRAPH_V IGRAPH_V_0_5\n");
  }
  printf("#endif");
}
