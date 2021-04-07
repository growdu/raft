#include "../lib/cluster.h"
#include "../lib/runner.h"

/******************************************************************************
 *
 * Fixture
 *
 *****************************************************************************/

struct fixture
{
    FIXTURE_CLUSTER;
};

static void *setUp(const MunitParameter params[], MUNIT_UNUSED void *user_data)
{
    struct fixture *f = munit_malloc(sizeof *f);
    SETUP_CLUSTER(3);
    CLUSTER_BOOTSTRAP;
    CLUSTER_START;
    CLUSTER_ELECT(0);
    return f;
}

static void tearDown(void *data)
{
    struct fixture *f = data;
    TEAR_DOWN_CLUSTER;
    free(f);
}

/******************************************************************************
 *
 * Helper macros
 *
 *****************************************************************************/

/* Set the snapshot threshold on all servers of the cluster */
#define SET_SNAPSHOT_THRESHOLD(VALUE)                            \
    {                                                            \
        unsigned i;                                              \
        for (i = 0; i < CLUSTER_N; i++) {                        \
            raft_set_snapshot_threshold(CLUSTER_RAFT(i), VALUE); \
        }                                                        \
    }

/* Set the snapshot trailing logs number on all servers of the cluster */
#define SET_SNAPSHOT_TRAILING(VALUE)                            \
    {                                                           \
        unsigned i;                                             \
        for (i = 0; i < CLUSTER_N; i++) {                       \
            raft_set_snapshot_trailing(CLUSTER_RAFT(i), VALUE); \
        }                                                       \
    }

/* Set the snapshot timeout on all servers of the cluster */
#define SET_SNAPSHOT_TIMEOUT(VALUE)                                   \
    {                                                                 \
        unsigned i;                                                   \
        for (i = 0; i < CLUSTER_N; i++) {                             \
            raft_set_install_snapshot_timeout(CLUSTER_RAFT(i), VALUE);\
        }                                                             \
    }

/******************************************************************************
 *
 * Successfully install a snapshot
 *
 *****************************************************************************/

SUITE(snapshot)

/* Install a snapshot on a follower that has fallen behind. */
TEST(snapshot, installOne, setUp, tearDown, 0, NULL)
{
    struct fixture *f = data;
    (void)params;

    /* Set very low threshold and trailing entries number */
    SET_SNAPSHOT_THRESHOLD(3);
    SET_SNAPSHOT_TRAILING(1);
    CLUSTER_SATURATE_BOTHWAYS(0, 2);

    /* Apply a few of entries, to force a snapshot to be taken. */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;

    /* Reconnect the follower and wait for it to catch up */
    CLUSTER_DESATURATE_BOTHWAYS(0, 2);
    CLUSTER_STEP_UNTIL_APPLIED(2, 4, 5000);

    return MUNIT_OK;
}

/* Install snapshot times out and leader retries */
TEST(snapshot, installOneTimeOut, setUp, tearDown, 0, NULL)
{
    struct fixture *f = data;
    (void)params;

    /* Set very low threshold and trailing entries number */
    SET_SNAPSHOT_THRESHOLD(3);
    SET_SNAPSHOT_TRAILING(1);
    SET_SNAPSHOT_TIMEOUT(200);
    CLUSTER_SATURATE_BOTHWAYS(0, 2);

    /* Apply a few of entries, to force a snapshot to be taken. */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;

    /* Reconnect the leader, but follower cannot reply */
    CLUSTER_DESATURATE(0, 2);

    /* InstallSnaphot RPC times out */
    CLUSTER_STEP_UNTIL_ELAPSED(400);

    /* Reconnect the follower */
    CLUSTER_DESATURATE(2, 0);
    CLUSTER_STEP_UNTIL_APPLIED(2, 4, 5000);

    /* Assert that the leader has retried the InstallSnapshot RPC */
    munit_assert_int(CLUSTER_N_SEND(0, RAFT_IO_INSTALL_SNAPSHOT), ==, 2);

    return MUNIT_OK;
}

/* Install snapshot times out and leader retries, afterwards AppendEntries resume */
TEST(snapshot, installOneTimeOutAppendAfter, setUp, tearDown, 0, NULL)
{
    struct fixture *f = data;
    (void)params;

    /* Set very low threshold and trailing entries number */
    SET_SNAPSHOT_THRESHOLD(3);
    SET_SNAPSHOT_TRAILING(1);
    SET_SNAPSHOT_TIMEOUT(200);
    CLUSTER_SATURATE_BOTHWAYS(0, 2);

    /* Apply a few of entries, to force a snapshot to be taken. */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;

    /* Reconnect the leader, but follower cannot reply */
    CLUSTER_DESATURATE(0, 2);

    /* InstallSnaphot RPC times out */
    CLUSTER_STEP_UNTIL_ELAPSED(400);

    /* Reconnect the follower */
    CLUSTER_DESATURATE(2, 0);

    /* Append a few entries and check if they are replicated */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_STEP_UNTIL_APPLIED(2, 5, 5000);

    /* Assert that the leader has retried the InstallSnapshot RPC */
    munit_assert_int(CLUSTER_N_SEND(0, RAFT_IO_INSTALL_SNAPSHOT), ==, 2);

    return MUNIT_OK;
}

/* Install 2 snapshots that both time out and assure the follower catches up */
TEST(snapshot, installMultipleTimeOut, setUp, tearDown, 0, NULL)
{
    struct fixture *f = data;
    (void)params;

    /* Set very low threshold and trailing entries number */
    SET_SNAPSHOT_THRESHOLD(3);
    SET_SNAPSHOT_TRAILING(1);
    SET_SNAPSHOT_TIMEOUT(200);
    CLUSTER_SATURATE_BOTHWAYS(0, 2);

    /* Apply a few of entries, to force a snapshot to be taken. */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;

    /* Reconnect the leader, but follower cannot reply */
    CLUSTER_DESATURATE(0, 2);

    /* InstallSnaphot RPC times out */
    CLUSTER_STEP_UNTIL_ELAPSED(400);

    /* Apply another few of entries, to force a new snapshot to be taken. */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;

    /* InstallSnaphot RPC times out */
    CLUSTER_STEP_UNTIL_ELAPSED(400);

    /* Reconnect the follower */
    CLUSTER_DESATURATE(2, 0);
    CLUSTER_STEP_UNTIL_APPLIED(2, 7, 5000);

    /* Assert that the leader has sent multiple InstallSnapshot RPCs */
    munit_assert_int(CLUSTER_N_SEND(0, RAFT_IO_INSTALL_SNAPSHOT), >=, 2);

    return MUNIT_OK;
}

/* Install 2 snapshots that both time out, launch a few regular AppendEntries
 * and assure the follower catches up */
TEST(snapshot, installMultipleTimeOutAppendAfter, setUp, tearDown, 0, NULL)
{
    struct fixture *f = data;
    (void)params;

    /* Set very low threshold and trailing entries number */
    SET_SNAPSHOT_THRESHOLD(3);
    SET_SNAPSHOT_TRAILING(1);
    SET_SNAPSHOT_TIMEOUT(200);
    CLUSTER_SATURATE_BOTHWAYS(0, 2);

    /* Apply a few of entries, to force a snapshot to be taken. */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;

    /* Reconnect the leader, but follower cannot reply */
    CLUSTER_DESATURATE(0, 2);

    /* InstallSnaphot RPC times out */
    CLUSTER_STEP_UNTIL_ELAPSED(400);

    /* Apply another few of entries, to force a new snapshot to be taken. */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;

    /* InstallSnaphot RPC times out */
    CLUSTER_STEP_UNTIL_ELAPSED(400);

    /* Reconnect the follower */
    CLUSTER_DESATURATE(2, 0);

    /* Append a few entries and make sure the follower catches up */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_STEP_UNTIL_APPLIED(2, 9, 5000);

    /* Assert that the leader has sent multiple InstallSnapshot RPCs */
    munit_assert_int(CLUSTER_N_SEND(0, RAFT_IO_INSTALL_SNAPSHOT), >=, 2);

    return MUNIT_OK;
}

static bool server_installing_snapshot(struct raft_fixture *f, void* data) {
    (void) f;
    const struct raft *r = data;
    return r->snapshot.put.data != NULL;
}

static bool server_snapshot_done(struct raft_fixture *f, void *data) {
    (void) f;
    const struct raft *r = data;
    return r->snapshot.put.data == NULL;
}

/* Follower receives HeartBeats during the installation of a snapshot */
TEST(snapshot, installSnapshotHeartBeats, setUp, tearDown, 0, NULL)
{
    struct fixture *f = data;
    (void)params;

    /* Set very low threshold and trailing entries number */
    SET_SNAPSHOT_THRESHOLD(3);
    SET_SNAPSHOT_TRAILING(1);
    CLUSTER_SATURATE_BOTHWAYS(0, 1);

    /* Apply a few of entries, to force a snapshot to be taken. */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;

    /* Set a large disk latency on the follower, this will allow some
     * heartbeats to be sent during the snapshot installation */
    CLUSTER_SET_DISK_LATENCY(1, 2000);

    munit_assert_uint(CLUSTER_N_RECV(1, RAFT_IO_INSTALL_SNAPSHOT), ==, 0);

    /* Step the cluster until server 1 installs a snapshot */
    const struct raft *r = CLUSTER_RAFT(1);
    CLUSTER_DESATURATE_BOTHWAYS(0, 1);
    CLUSTER_STEP_UNTIL(server_installing_snapshot, (void*) r, 2000);
    munit_assert_uint(CLUSTER_N_RECV(1, RAFT_IO_INSTALL_SNAPSHOT), ==, 1);

    /* Count the number of AppendEntries RPCs received during the snapshot
     * install*/
    unsigned before = CLUSTER_N_RECV(1, RAFT_IO_APPEND_ENTRIES);
    CLUSTER_STEP_UNTIL(server_snapshot_done, (void*) r, 5000);
    unsigned after = CLUSTER_N_RECV(1, RAFT_IO_APPEND_ENTRIES);
    munit_assert_uint(before, < , after);

    /* Check that the InstallSnapshot RPC was not resent */
    munit_assert_uint(CLUSTER_N_RECV(1, RAFT_IO_INSTALL_SNAPSHOT), ==, 1);

    /* Check that the snapshot was applied and we can still make progress */
    CLUSTER_STEP_UNTIL_APPLIED(1, 4, 5000);
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_STEP_UNTIL_APPLIED(1, 6, 5000);

    return MUNIT_OK;
}

/* InstallSnapshot RPC arrives while persisting Entries */
TEST(snapshot, installSnapshotDuringEntriesWrite, setUp, tearDown, 0, NULL)
{
    struct fixture *f = data;
    (void)params;

    /* Set a large disk latency on the follower, this will allow a
     * InstallSnapshot RPC to arrive while the entries are being persisted. */
    CLUSTER_SET_DISK_LATENCY(1, 2000);
    SET_SNAPSHOT_THRESHOLD(3);
    SET_SNAPSHOT_TRAILING(1);

    /* Replicate some entries, these will take a while to persist */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS;

    /* Make sure leader can't succesfully send any more entries */
    CLUSTER_DISCONNECT(0,1);
    CLUSTER_MAKE_PROGRESS; /* Snapshot taken here */
    CLUSTER_MAKE_PROGRESS;
    CLUSTER_MAKE_PROGRESS; /* Snapshot taken here */
    CLUSTER_MAKE_PROGRESS;

    /* Snapshot with index 6 is sent while follower is still writing the entries
     * to disk that arrived before the disconnect. */
    CLUSTER_RECONNECT(0,1);

    /* Make sure follower is up to date */
    CLUSTER_STEP_UNTIL_APPLIED(1, 7, 5000);
    return MUNIT_OK;
}
