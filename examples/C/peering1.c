//
//  Broker peering simulation (part 1)
//  Prototypes the state flow
//
#include "zmsg.class"

int main (int argc, char *argv [])
{
    //  First argument is this broker's name
    //  Other arguments are our peers' names
    //
    if (argc < 2) {
        printf ("syntax: peering1 me {you}...\n");
        exit (EXIT_FAILURE);
    }
    char *self = argv [1];
    printf ("I: preparing broker at %s...\n", self);
    srandom ((unsigned) time (NULL));

    //  Prepare our context and sockets
    void *context = zmq_init (1);
    char endpoint [256];

    //  Bind statebe to endpoint
    void *statebe = zmq_socket (context, ZMQ_PUB);
    snprintf (endpoint, 255, "ipc://%s-state.ipc", self);
    int rc = zmq_bind (statebe, endpoint);
    assert (rc == 0);

    //  Connect statefe to all peers
    void *statefe = zmq_socket (context, ZMQ_SUB);
    zmq_setsockopt (statefe, ZMQ_SUBSCRIBE, "", 0);

    int argn;
    for (argn = 2; argn < argc; argn++) {
        char *peer = argv [argn];
        printf ("I: connecting to state backend at '%s'\n", peer);
        snprintf (endpoint, 255, "ipc://%s-state.ipc", peer);
        rc = zmq_connect (statefe, endpoint);
        assert (rc == 0);
    }
    //  Send out status messages to peers, and collect from peers
    //  The zmq_poll timeout defines our own heartbeating
    //
    while (1) {
        //  Initialize poll set
        zmq_pollitem_t items [] = {
            { statefe, 0, ZMQ_POLLIN, 0 }
        };
        //  Poll for activity, or 1 second timeout
        rc = zmq_poll (items, 1, 1000000);
        assert (rc >= 0);

        //  Handle incoming status message
        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *zmsg = zmsg_recv (statefe);
            printf ("%s - %s workers free\n",
                zmsg_address (zmsg), zmsg_body (zmsg));
            zmsg_destroy (&zmsg);
        }
        else {
            //  Send random value for worker availability
            zmsg_t *zmsg = zmsg_new ();
            zmsg_body_fmt (zmsg, "%d", randof (10));
            //  We stick our own address onto the envelope
            zmsg_wrap (zmsg, self, NULL);
            zmsg_send (&zmsg, statebe);
        }
    }
    //  We never get here but clean up anyhow
    zmq_close (statefe);
    zmq_term (context);
    return EXIT_SUCCESS;
}
