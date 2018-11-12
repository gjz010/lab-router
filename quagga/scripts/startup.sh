#!/bin/bash
echo "Router Experiment"
echo "Starting Zebra"
zebra -d
echo "Starting Ripd"
ripd -d
echo "You've Done! Go into the container to start your development..."
tail -f /var/log/quagga/zebra.log
