#!/bin/bash
#FLUX: -N 2
#FLUX: --exclusive

{{ source_lib_path_script }}

server_ip=$(flux exec -r 0 hostname -i)

flux submit -N 1 --exclusive --tasks-per-node=1 --requires=rank:0 \
    --env=CALI_SERVICES_ENABLE="event,trace,recorder,timer" --env=CALI_RECORDER_FILENAME="server.cali" \
    --output=server.out --error=server.err \
    {{ server_exe }} tag $server_ip {{ data_size }} {{ num_clients }} -p 8888

flux submit -N 1 --exclusive --tasks-per-node={{ num_clients }} --label-io \
    --env=CALI_SERVICES_ENABLE="event,trace,recorder,timer" \
    --output=clients.out --error=clients.err \
    {{ client_exe }} tag $server_ip {{ data_size }} -n {{ num_iters }} -p 8888
        
flux queue drain
