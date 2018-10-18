netsh interface ipv4 set dynamicportrange protocol=tcp startport=1025 numberofports=64511
netsh interface ipv4 show dynamicportrange protocol=tcp
pause