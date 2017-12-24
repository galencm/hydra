/*  =========================================================================
    hydra_client - Hydra Client

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of zbroker, the ZeroMQ broker project.           
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

/*
@header
    This is a client for the Hydra protocol.
@discuss
    Generated by zproto and implements a background actor that manages the
    connection to the Hydra server.
@end
*/

#include "hydra_classes.h"

//  Forward reference to method arguments structure
typedef struct _client_args_t client_args_t;

//  This structure defines the context for a client connection
typedef struct {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine. The cmdpipe gets
    //  messages sent to the actor; the msgpipe may be used for
    //  faster asynchronous message flows.
    zsock_t *cmdpipe;           //  Command pipe to/from caller API
    zsock_t *msgpipe;           //  Message pipe to/from caller API
    zsock_t *dealer;            //  Socket to talk to server
    hydra_proto_t *message;     //  Message from and to server
    client_args_t *args;        //  Arguments from methods

    int heartbeat_timer;        //  Timeout for heartbeats to server
    int retries;                //  How many heartbeats we've tried
    zconfig_t *config;          //  Own configuration data
    char *identity;             //  Own identity to send to server
    char *nickname;             //  Own nickname to send to server
    zconfig_t *peer_config;     //  Peer configuration data
    hydra_post_t *post;         //  Current post we're receiving
    const char *oldest;         //  Oldest post from peer
    const char *newest;         //  Newest post from peer
    size_t chunk_offset;        //  For fetching post content in chunks
    zsock_t *sink;              //  Where we send posts to be stored
    size_t received;            //  Number of posts received
} client_t;

//  Include the generated client engine
#include "hydra_client_engine.inc"

//  Maximum size of chunks we fetch, 1MB seems fair over WiFi
#define CHUNK_SIZE 1024 * 1024 * 10

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
    //  Load configuration data
    self->config = zconfig_load ("hydra.cfg");
    assert (self->config);          //  Server must already have started
    self->identity = zconfig_resolve (self->config, "/hydra/identity", NULL);
    assert (self->identity);        //  Server must already have started
    self->nickname = zconfig_resolve (self->config, "/hydra/nickname", "");

    //  Create and connect sink socket; use identity as unique endpoint
    self->sink = zsock_new (ZMQ_PUSH);
    int rc = zsock_connect (self->sink, "inproc://%s", self->identity);
    assert (rc == 0);

    //  We'll ping the server once per second
    self->heartbeat_timer = 1000;
    
    return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
    zconfig_destroy (&self->config);
    zconfig_destroy (&self->peer_config);
    hydra_post_destroy (&self->post);
    zsock_destroy (&self->sink);
}


//  ---------------------------------------------------------------------------
//  use_connect_timeout
//

static void
use_connect_timeout (client_t *self)
{
    engine_set_timeout (self, self->args->timeout);
}


//  ---------------------------------------------------------------------------
//  connect_to_server_endpoint
//

static void
connect_to_server_endpoint (client_t *self)
{
    if (zsock_connect (self->dealer, "%s", self->args->endpoint)) {
        engine_set_exception (self, bad_endpoint_event);
        zsys_warning ("could not connect to %s", self->args->endpoint);
    }
}


//  ---------------------------------------------------------------------------
//  set_client_identity
//

static void
set_client_identity (client_t *self)
{
    hydra_proto_set_identity (self->message, self->identity);
    hydra_proto_set_nickname (self->message, self->nickname);
}


//  ---------------------------------------------------------------------------
//  client_is_connected
//

static void
client_is_connected (client_t *self)
{
    self->retries = 0;
    engine_set_connected (self, true);
    engine_set_timeout (self, self->heartbeat_timer);
}


//  ---------------------------------------------------------------------------
//  check_if_connection_is_dead
//

static void
check_if_connection_is_dead (client_t *self)
{
    //  We send at most 3 heartbeats before expiring the server
    if (++self->retries >= 3) {
        engine_set_timeout (self, 0);
        engine_set_connected (self, false);
        engine_set_exception (self, exception_event);
    }
}


//  ---------------------------------------------------------------------------
//  signal_connected
//

static void
signal_connected (client_t *self)
{
    const char *identity = hydra_proto_identity (self->message);
    const char *nickname = hydra_proto_nickname (self->message);
    
    //  Load the peer's config file, or create a new config tree
    self->peer_config = zconfig_loadf ("peers/%s.cfg", identity);
    if (!self->peer_config)
        self->peer_config = zconfig_new ("root", NULL);

    //  Store the id and nickname we got from the server
    zconfig_put (self->peer_config, "/peer/identity", identity);
    zconfig_put (self->peer_config, "/peer/nickname", nickname);

    //  Signal to calling API that we're successfully connected
    zsock_send (self->cmdpipe, "sis", "CONNECTED", 0, nickname);
}


//  ---------------------------------------------------------------------------
//  check_peer_status
//

static void
check_peer_status (client_t *self)
{
    self->received = 0;
    self->oldest = zconfig_resolve (self->peer_config, "/peer/oldest", NULL);
    self->newest = zconfig_resolve (self->peer_config, "/peer/newest", NULL);

    if (self->oldest && self->newest)
        engine_set_next_event (self, known_peer_event);
    else
        engine_set_next_event (self, new_peer_event);
}


//  ---------------------------------------------------------------------------
//  prepare_to_fetch_newest_post
//

static void
prepare_to_fetch_newest_post (client_t *self)
{
    hydra_proto_set_ident (self->message, "HEAD");
}


//  ---------------------------------------------------------------------------
//  prepare_to_fetch_newer_post
//

static void
prepare_to_fetch_newer_post (client_t *self)
{
    //  We'll ask for next newer post before newest
    if (self->newest)
        hydra_proto_set_ident (self->message, self->newest);
}


//  ---------------------------------------------------------------------------
//  prepare_to_fetch_older_post
//

static void
prepare_to_fetch_older_post (client_t *self)
{
    //  We'll ask for next older post before oldest
    if (self->oldest)
        hydra_proto_set_ident (self->message, self->oldest);
}


//  ---------------------------------------------------------------------------
//  use_this_post_as_oldest
//

static void
use_this_post_as_oldest (client_t *self)
{
    //  Store the oldest post ID in peer config, then grab a reference to it
    zconfig_put (self->peer_config, "/peer/oldest", hydra_proto_ident (self->message));
    self->oldest = zconfig_resolve (self->peer_config, "/peer/oldest", NULL);
    
    //  If we don't have a newest post, use this post
    if (!self->newest) {
        zconfig_put (self->peer_config, "/peer/newest", self->oldest);
        self->oldest = zconfig_resolve (self->peer_config, "/peer/newest", NULL);
    }
}


//  ---------------------------------------------------------------------------
//  use_this_post_as_newest
//

static void
use_this_post_as_newest (client_t *self)
{
    //  Store the newest post ID in peer config, then grab a reference to it
    zconfig_put (self->peer_config, "/peer/newest", hydra_proto_ident (self->message));
    self->newest = zconfig_resolve (self->peer_config, "/peer/newest", NULL);
}


//  ---------------------------------------------------------------------------
//  skip_post_if_duplicate
//

static void
skip_post_if_duplicate (client_t *self)
{
    //  What's the resolution here? Could we use a ledger actor and talk to
    //  it directly via dealer-router?
    //  This is the most brute-force solution I could think of
    hydra_ledger_t *ledger = hydra_ledger_new ();
    hydra_ledger_load (ledger);
    if (hydra_ledger_index (ledger, hydra_proto_ident (self->message)) >= 0)
        engine_set_exception (self, duplicate_event);
    hydra_ledger_destroy (&ledger);
}


//  ---------------------------------------------------------------------------
//  store_post_metadata
//

static void
store_post_metadata (client_t *self)
{
    hydra_post_destroy (&self->post);
    self->post = hydra_post_decode (self->message);
}


//  ---------------------------------------------------------------------------
//  prepare_to_get_first_chunk
//

static void
prepare_to_get_first_chunk (client_t *self)
{
    //  TODO: stage post data and restart failed transfers
    //  - implement this in hydra_post
    //  - create new post, set_remote
    //  - desired content size, actual content size
    //  - hydra_post tells us what offset & octets to fetch
    //  - hydra_post_store to accept a chunk
    //  - calculate digest on the fly, check it matches at the end
    //  For now, assume 1 post = 1 chunk
    self->chunk_offset = 0;
    hydra_proto_set_offset (self->message, self->chunk_offset);
    hydra_proto_set_octets (self->message, CHUNK_SIZE);
}


//  ---------------------------------------------------------------------------
//  store_post_content_chunk
//

static void
store_post_content_chunk (client_t *self)
{
    //  Only allow one chunk per post for now
    assert (hydra_proto_offset (self->message) == 0);
    zchunk_t *chunk = hydra_proto_content (self->message);
    hydra_post_set_data (self->post, zchunk_data (chunk), zchunk_size (chunk));
}


//  ---------------------------------------------------------------------------
//  store_complete_post
//

static void
store_complete_post (client_t *self)
{
    //  Send post off to sink and to API for caller; since both recipients
    //  will own the post, we duplicate it as we send it
    zsock_send (self->sink, "p", hydra_post_dup (self->post));
    zsock_send (self->msgpipe, "sp", "POST", self->post);
    self->post = NULL;
    self->received++;
}


//  ---------------------------------------------------------------------------
//  save_peer_configuration
//

static void
save_peer_configuration (client_t *self)
{
    //  We store peer configuration data in the "peers" subdirectory
    //  If this doesn't already exist, now is a good time to create it
    char *identity = zconfig_resolve (self->peer_config, "/peer/identity", NULL);
    assert (identity);
    zsys_dir_create ("peers");
    zconfig_savef (self->peer_config, "peers/%s.cfg", identity);
}


//  ---------------------------------------------------------------------------
//  signal_success
//

static void
signal_success (client_t *self)
{
    zsock_send (self->cmdpipe, "si", "SUCCESS", 0);
}


//  ---------------------------------------------------------------------------
//  signal_sync_success
//  We send SUCCESS + count to msgpipe, which caller is monitoring for new
//  posts and this completion status.
//

static void
signal_sync_success (client_t *self)
{
    zsock_send (self->msgpipe, "si", "SUCCESS", self->received);
}


//  ---------------------------------------------------------------------------
//  check_status_code
//

static void
check_status_code (client_t *self)
{
    if (hydra_proto_status (self->message) == HYDRA_PROTO_COMMAND_INVALID)
        engine_set_next_event (self, command_invalid_event);
    else
        engine_set_next_event (self, other_event);
}


//  ---------------------------------------------------------------------------
//  signal_bad_endpoint
//

static void
signal_bad_endpoint (client_t *self)
{
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, "Bad server endpoint");
}


//  ---------------------------------------------------------------------------
//  signal_unhandled_error
//

static void
signal_unhandled_error (client_t *self)
{
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, "Unhandled error");
    zsock_send (self->msgpipe, "sis", "FAILURE", -1, "Unhandled error");
}


//  ---------------------------------------------------------------------------
//  signal_internal_error
//

static void
signal_internal_error (client_t *self)
{
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, "Internal server error");
    zsock_send (self->msgpipe, "sis", "FAILURE", -1, "Internal server error");
}


//  ---------------------------------------------------------------------------
//  Selftest

void
hydra_client_test (bool verbose)
{
    printf (" * hydra_client: ");
    if (verbose)
        printf ("\n");
    
    //  @selftest
    //  Start a server to test against, and bind to endpoint
    zsys_dir_create (".hydra_test");
    zsys_dir_change (".hydra_test");
    zactor_t *server = zactor_new (hydra_server, "hydra_client_test");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "LOAD", "hydra.cfg", NULL);
    zstr_sendx (server, "BIND", "ipc://@/hydra", NULL);
    
    hydra_client_verbose = verbose;
    hydra_client_t *client = hydra_client_new ();
    assert (client);
    int rc = hydra_client_connect (client, "ipc://@/hydra", 500);
    assert (rc == 0);
//     int rc = hydra_client_fetch (client, HYDRA_PROTO_FETCH_RESET);
//     assert (rc == -1);
    hydra_client_destroy (&client);
    
    zactor_destroy (&server);
    
    //  Delete the test directory
    zsys_dir_change ("..");
    zdir_t *dir = zdir_new (".hydra_test", NULL);
    assert (dir);
    zdir_remove (dir, true);
    zdir_destroy (&dir);
    //  @end
    printf ("OK\n");
}
