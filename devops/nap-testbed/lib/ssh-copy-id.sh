#!/bin/sh

for node in `cat ~/nap_nodes`; do
  ssh-copy-id point@"${node}"
done

