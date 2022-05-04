

aux_source_directory(./ NET_LIST)
aux_source_directory(./poller/ NET_LIST)

add_library(net ${NET_LIST})
