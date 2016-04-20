
include_directories("./include")

set(SOURCES 
    src/mqtt.c 
    src/mqtt_msg.c 
    src/proto.c 
    src/queue.c src/ringbuf.c 
    src/utils.c)

add_library(mqtt ${SOURCES})

