#include <unistd.h> // sleep()
#include "finstrument.h"

int
main(int argc, char *argv[])
{
    uuid_t id1;
    tracer_event("started program.");
    tracer_event_in("block1 started.");
    sleep(1);
    tracer_event_out("block1 ended.");
    tracer_event_in_r(id1, "block2 started.");
    sleep(2);
    tracer_event_out_r(id1, "block2 ended.");
    return 0;
}