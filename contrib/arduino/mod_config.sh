#!/bin/bash

cp template.yaml config.yaml

sed -i 's|^\(  data: \).*$|\1'"${PWD}"'|' config.yaml
sed -i 's|^\(  user: \).*$|\1'"${PWD}/Arduino"'|' config.yaml
# sed -i 's|^\(  downloads: \).*$|\1'"/tmp/arduino"'|' config.yaml
