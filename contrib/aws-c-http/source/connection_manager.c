/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

#include <aws/http/connection_manager.h>

#include <aws/http/connection.h>
#include <aws/http/private/connection_manager_system_vtable.h>
#include <aws/http/private/connection_monitor.h>
#include <aws/http/private/http_impl.h>
#include <aws/http/private/proxy_impl.h>

#include <aws/io/channel_bootstrap.h>
#include <aws/io/event_loop.h>
#include <aws/io/logging.h>
#include <aws/io/socket.h>
#include <aws/io/tls_channel_handler.h>
#include <aws/io/uri.h>

#include <aws/common/clock.h>
#include <aws/common/hash_table.h>
#include <aws/common/linked_list.h>
#include <aws/common/mutex.h>
#include <aws/common/string.h>

#if _MSC_VER
#    pragma warning(disable : 4232) /* function pointer to dll symbol */
#endif

/*
 * Established connections not currently in use are tracked via this structure.
 */
struct aws_idle_connection {
    struct aws_allocator *allocator;
    struct aws_linked_list_node node;
    uint64_t cull_timestamp;
    struct aws_http_connection *connection;
};

/*
 * System vtable to use under normal circumstances
 */
static struct aws_http_connection_manager_system_vtable s_default_system_vtable = {
    .create_connection = aws_http_client_connect,
    .release_connection = aws_http_connection_release,
    .close_connection = aws_http_connection_close,
    .is_connection_available = aws_http_connection_new_requests_allowed,
    .get_monotonic_time = aws_high_res_clock_get_ticks,
    .is_callers_thread = aws_channel_thread_is_callers_thread,
    .connection_get_channel = aws_http_connection_get_channel,
};

const struct aws_http_connection_manager_system_vtable *g_aws_http_connection_manager_default_system_vtable_ptr =
    &s_default_system_vtable;

bool aws_http_connection_manager_system_vtable_is_valid(const struct aws_http_connection_manager_system_vtable *table) {
    return table->create_connection && table->close_connection && table->release_connection &&
           table->is_connection_available;
}

enum aws_http_connection_manager_state_type { AWS_HCMST_UNINITIALIZED, AWS_HCMST_READY, AWS_HCMST_SHUTTING_DOWN };

/**
 * Vocabulary
 *    Acquisition - a request by a user for a connection
 *    Pending Acquisition - a request by a user for a new connection that has not been completed.  It may be
 *      waiting on http, a release by another user, or the manager itself.
 *    Pending Connect - a request to the http layer for a new connection that has not been resolved yet
 *    Vended Connection - a successfully established connection that is currently in use by something; must
 *      be released (through the connection manager) by the user before anyone else can use it.  The connection
 *      manager does not explicitly track vended connections.
 *    Task Set - A set of operations that should be attempted once the lock is released.  A task set includes
 *      completion callbacks (which can't fail) and connection attempts (which can fail either immediately or
 *      asynchronously).
 *
 * Requirements/Assumptions
 *    (1) Don't invoke user callbacks while holding the internal state lock
 *    (2) Don't invoke downstream http calls while holding the internal state lock
 *    (3) Only log unusual or rare events while the lock is held.  Common-path logging should be while it is
 *        not held.
 *    (4) Don't crash or do awful things (leaking resources is ok though) if the interface contract
 *        (ref counting + balanced acquire/release of connections) is violated by the user
 *
 *  In order to fulfill (1) and (2), all side-effecting operations within the connection manager follow a pattern:
 *
 *    (1) Lock
 *    (2) Make state changes based on the operation
 *    (3) Build a set of work (completions, connect calls, releases, self-destruction) as appropriate to the operation
 *    (4) Unlock
 *    (5) Execute the task set
 *
 *   Asynchronous work order failures are handled in the async callback, but immediate failures require
 *   us to relock and update the internal state.  When there's an immediate connect failure, we use a
 *   conservative policy to fail all excess (beyond the # of pending connects) acquisitions; this allows us
 *   to avoid a possible recursive invocation (and potential failures) to connect again.
 *
 * Lifecycle
 * Our connection manager implementation has a reasonably complex lifecycle.
 *
 * All state around the life cycle is protected by a lock.  It seemed too risky and error-prone
 * to try and mix an atomic ref count with the internal tracking counters we need.
 *
 * Over the course of its lifetime, a connection manager moves through two states:
 *
 * READY - connections may be acquired and released.  When the external ref count for the manager
 * drops to zero, the manager moves to:
 *
 * SHUTTING_DOWN - connections may no longer be acquired and released (how could they if the external
 * ref count was accurate?) but in case of user ref errors, we simply fail attempts to do so rather
 * than crash or underflow.  While in this state, we wait for a set of tracking counters to all fall to zero:
 *
 *   pending_connect_count - the # of unresolved calls to the http layer's connect logic
 *   open_connection_count - the # of connections for whom the release callback (from http) has not been invoked
 *   vended_connection_count - the # of connections held by external users that haven't been released.  Under correct
 *      usage this should be zero before SHUTTING_DOWN is entered, but we attempt to handle incorrect usage gracefully.
 *
 *  While shutting down, as pending connects resolve, we immediately release new incoming (from http) connections
 *
 *  During the transition from READY to SHUTTING_DOWN, we flush the pending acquisition queue (with failure callbacks)
 *   and since we disallow new acquires, pending_acquisition_count should always be zero after the transition.
 *
 */
struct aws_http_connection_manager {
    struct aws_allocator *allocator;

    /*
     * A union of external downstream dependencies (primarily global http API functions) and
     * internal implementation references.  Selectively overridden by tests in order to
     * enable strong coverage of internal implementation details.
     */
    const struct aws_http_connection_manager_system_vtable *system_vtable;

    /*
     * Callback to invoke when shutdown has completed and all resources have been cleaned up.
     */
    aws_http_connection_manager_shutdown_complete_fn *shutdown_complete_callback;

    /*
     * User data to pass to the shutdown completion callback.
     */
    void *shutdown_complete_user_data;

    /*
     * Controls access to all mutable state on the connection manager
     */
    struct aws_mutex lock;

    /*
     * A manager can be in one of two states, READY or SHUTTING_DOWN.  The state transition
     * takes place when ref_count drops to zero.
     */
    enum aws_http_connection_manager_state_type state;

    /*
     * The number of all established, idle connections.  So
     * that we don't have compute the size of a linked list every time.
     */
    size_t idle_connection_count;

    /*
     * The set of all available, ready-to-be-used connections, as aws_idle_connection structs.
     *
     * This must be a LIFO stack.  When connections are released by the user, they must be added on to the back.
     * When we vend connections to the user, they must be removed from the back first.
     * In this way, the list will always be sorted from oldest (in terms of time spent idle) to newest.  This means
     * we can always use the cull timestamp of the front connection as the next scheduled time for culling.
     * It also means that when we cull connections, we can quit the loop as soon as we find a connection
     * whose timestamp is greater than the current timestamp.
     */
    struct aws_linked_list idle_connections;

    /*
     * The set of all incomplete connection acquisition requests
     */
    struct aws_linked_list pending_acquisitions;

    /*
     * The number of all incomplete connection acquisition requests.  So
     * that we don't have compute the size of a linked list every time.
     */
    size_t pending_acquisition_count;

    /*
     * The number of pending new connection requests we have outstanding to the http
     * layer.
     */
    size_t pending_connects_count;

    /*
     * The number of connections currently being used by external users.
     */
    size_t vended_connection_count;

    /*
     * Always equal to # of connection shutdown callbacks not yet invoked
     * or equivalently:
     *
     * # of connections ever created by the manager - # shutdown callbacks received
     */
    size_t open_connection_count;

    /*
     * All the options needed to create an http connection
     */
    struct aws_client_bootstrap *bootstrap;
    size_t initial_window_size;
    struct aws_socket_options socket_options;
    struct aws_tls_connection_options *tls_connection_options;
    struct aws_http_proxy_config *proxy_config;
    struct aws_http_connection_monitoring_options monitoring_options;
    struct aws_string *host;
    struct proxy_env_var_settings proxy_ev_settings;
    struct aws_tls_connection_options *proxy_ev_tls_options;
    uint16_t port;
    /*
     * HTTP/2 specific.
     */
    bool prior_knowledge_http2;
    struct aws_array_list *initial_settings;
    size_t max_closed_streams;
    bool http2_conn_manual_window_management;

    /*
     * The maximum number of connections this manager should ever have at once.
     */
    size_t max_connections;

    /*
     * Lifecycle tracking for the connection manager.  Starts at 1.
     *
     * Once this drops to zero, the manager state transitions to shutting down
     *
     * The manager is deleted when all other tracking counters have returned to zero.
     *
     * We don't use an atomic here because the shutdown phase wants to check many different
     * values.  You could argue that we could use a sum of everything, but we still need the
     * individual values for proper behavior and error checking during the ready state.  Also,
     * a hybrid atomic/lock solution felt excessively complicated and delicate.
     */
    size_t external_ref_count;

    /*
     * if set to true, read back pressure mechanism will be enabled.
     */
    bool enable_read_back_pressure;

    /**
     * If set to a non-zero value, then connections that stay in the pool longer than the specified
     * timeout will be closed automatically.
     */
    uint64_t max_connection_idle_in_milliseconds;

    /*
     * Task to cull idle connections.  This task is run periodically on the cull_event_loop if a non-zero
     * culling time interval is specified.
     */
    struct aws_task *cull_task;
    struct aws_event_loop *cull_event_loop;
};

struct aws_http_connection_manager_snapshot {
    enum aws_http_connection_manager_state_type state;

    size_t idle_connection_count;
    size_t pending_acquisition_count;
    size_t pending_connects_count;
    size_t vended_connection_count;
    size_t open_connection_count;

    size_t external_ref_count;
};

/*
 * Correct usage requires AWS_ZERO_STRUCT to have been called beforehand.
 */
static void s_aws_http_connection_manager_get_snapshot(
    struct aws_http_connection_manager *manager,
    struct aws_http_connection_manager_snapshot *snapshot) {

    snapshot->state = manager->state;
    snapshot->idle_connection_count = manager->idle_connection_count;
    snapshot->pending_acquisition_count = manager->pending_acquisition_count;
    snapshot->pending_connects_count = manager->pending_connects_count;
    snapshot->vended_connection_count = manager->vended_connection_count;
    snapshot->open_connection_count = manager->open_connection_count;

    snapshot->external_ref_count = manager->external_ref_count;
}

static void s_aws_http_connection_manager_log_snapshot(
    struct aws_http_connection_manager *manager,
    struct aws_http_connection_manager_snapshot *snapshot) {
    if (snapshot->state != AWS_HCMST_UNINITIALIZED) {
        AWS_LOGF_DEBUG(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: snapshot - state=%d, idle_connection_count=%zu, pending_acquire_count=%zu, "
            "pending_connect_count=%zu, vended_connection_count=%zu, open_connection_count=%zu, ref_count=%zu",
            (void *)manager,
            (int)snapshot->state,
            snapshot->idle_connection_count,
            snapshot->pending_acquisition_count,
            snapshot->pending_connects_count,
            snapshot->vended_connection_count,
            snapshot->open_connection_count,
            snapshot->external_ref_count);
    } else {
        AWS_LOGF_DEBUG(
            AWS_LS_HTTP_CONNECTION_MANAGER, "id=%p: snapshot not initialized by control flow", (void *)manager);
    }
}

void aws_http_connection_manager_set_system_vtable(
    struct aws_http_connection_manager *manager,
    const struct aws_http_connection_manager_system_vtable *system_vtable) {
    AWS_FATAL_ASSERT(aws_http_connection_manager_system_vtable_is_valid(system_vtable));

    manager->system_vtable = system_vtable;
}

/*
 * Hard Requirement: Manager's lock must held somewhere in the call stack
 */
static bool s_aws_http_connection_manager_should_destroy(struct aws_http_connection_manager *manager) {
    if (manager->state != AWS_HCMST_SHUTTING_DOWN) {
        return false;
    }

    if (manager->external_ref_count != 0) {
        AWS_LOGF_ERROR(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: ref count is non zero while in the shut down state",
            (void *)manager);
        return false;
    }

    if (manager->vended_connection_count > 0 || manager->pending_connects_count > 0 ||
        manager->open_connection_count > 0) {
        return false;
    }

    AWS_FATAL_ASSERT(manager->idle_connection_count == 0);

    return true;
}

/*
 * A struct that functions as both the pending acquisition tracker and the about-to-complete data.
 *
 * The list in the connection manager (pending_acquisitions) is the set of all acquisition requests that we
 * haven't yet resolved.
 *
 * In order to make sure we never invoke callbacks while holding the manager's lock, in a number of places
 * we build a list of one or more acquisitions to complete.  Once the lock is released
 * we complete all the acquisitions in the list using the data within the struct (hence why we have
 * "result-oriented" members like connection and error_code).  This means we can fail an acquisition
 * simply by setting the error_code and moving it to the current transaction's completion list.
 */
struct aws_http_connection_acquisition {
    struct aws_allocator *allocator;
    struct aws_linked_list_node node;
    struct aws_http_connection_manager *manager; /* Only used by logging */
    aws_http_connection_manager_on_connection_setup_fn *callback;
    void *user_data;
    struct aws_http_connection *connection;
    int error_code;
    struct aws_channel_task acquisition_task;
};

static void s_connection_acquisition_task(
    struct aws_channel_task *channel_task,
    void *arg,
    enum aws_task_status status) {
    (void)channel_task;

    struct aws_http_connection_acquisition *pending_acquisition = arg;

    /* this is a channel task. If it is canceled, that means the channel shutdown. In that case, that's equivalent
     * to a closed connection. */
    if (status != AWS_TASK_STATUS_RUN_READY) {
        AWS_LOGF_WARN(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Failed to complete connection acquisition because the connection was closed",
            (void *)pending_acquisition->manager);
        pending_acquisition->callback(NULL, AWS_ERROR_HTTP_CONNECTION_CLOSED, pending_acquisition->user_data);
        /* release it back to prevent a leak of the connection count. */
        aws_http_connection_manager_release_connection(pending_acquisition->manager, pending_acquisition->connection);
    } else {
        AWS_LOGF_DEBUG(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Successfully completed connection acquisition with connection id=%p",
            (void *)pending_acquisition->manager,
            (void *)pending_acquisition->connection);
        pending_acquisition->callback(
            pending_acquisition->connection, pending_acquisition->error_code, pending_acquisition->user_data);
    }

    aws_mem_release(pending_acquisition->allocator, pending_acquisition);
}

/*
 * Invokes a set of connection acquisition completion callbacks.
 *
 * Soft Requirement: The manager's lock must not be held in the callstack.
 *
 * Assumes that internal state (like pending_acquisition_count, vended_connection_count, etc...) have already been
 * updated according to the list's contents.
 */
static void s_aws_http_connection_manager_complete_acquisitions(
    struct aws_linked_list *acquisitions,
    struct aws_allocator *allocator) {

    while (!aws_linked_list_empty(acquisitions)) {
        struct aws_linked_list_node *node = aws_linked_list_pop_front(acquisitions);
        struct aws_http_connection_acquisition *pending_acquisition =
            AWS_CONTAINER_OF(node, struct aws_http_connection_acquisition, node);

        if (pending_acquisition->error_code == AWS_OP_SUCCESS) {

            struct aws_channel *channel =
                pending_acquisition->manager->system_vtable->connection_get_channel(pending_acquisition->connection);
            AWS_PRECONDITION(channel);

            /* For some workloads, going ahead and moving the connection callback to the connection's thread is a
             * substantial performance improvement so let's do that */
            if (!pending_acquisition->manager->system_vtable->is_callers_thread(channel)) {
                aws_channel_task_init(
                    &pending_acquisition->acquisition_task,
                    s_connection_acquisition_task,
                    pending_acquisition,
                    "s_connection_acquisition_task");
                aws_channel_schedule_task_now(channel, &pending_acquisition->acquisition_task);
                return;
            }
            AWS_LOGF_DEBUG(
                AWS_LS_HTTP_CONNECTION_MANAGER,
                "id=%p: Successfully completed connection acquisition with connection id=%p",
                (void *)pending_acquisition->manager,
                (void *)pending_acquisition->connection);

        } else {
            AWS_LOGF_WARN(
                AWS_LS_HTTP_CONNECTION_MANAGER,
                "id=%p: Failed to complete connection acquisition with error_code %d(%s)",
                (void *)pending_acquisition->manager,
                pending_acquisition->error_code,
                aws_error_str(pending_acquisition->error_code));
        }

        pending_acquisition->callback(
            pending_acquisition->connection, pending_acquisition->error_code, pending_acquisition->user_data);
        aws_mem_release(allocator, pending_acquisition);
    }
}

/*
 * Moves the first pending connection acquisition into a (task set) list.  Call this while holding the lock to
 * build the set of callbacks to be completed once the lock is released.
 *
 * Hard Requirement: Manager's lock must held somewhere in the call stack
 *
 * If this was a successful acquisition then connection is non-null
 * If this was a failed acquisition then connection is null and error_code is hopefully a useful diagnostic (extreme
 * edge cases exist where it may not be though)
 */
static void s_aws_http_connection_manager_move_front_acquisition(
    struct aws_http_connection_manager *manager,
    struct aws_http_connection *connection,
    int error_code,
    struct aws_linked_list *output_list) {

    AWS_FATAL_ASSERT(!aws_linked_list_empty(&manager->pending_acquisitions));
    struct aws_linked_list_node *node = aws_linked_list_pop_front(&manager->pending_acquisitions);

    AWS_FATAL_ASSERT(manager->pending_acquisition_count > 0);
    --manager->pending_acquisition_count;

    if (error_code == AWS_ERROR_SUCCESS && connection == NULL) {
        AWS_LOGF_FATAL(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Connection acquisition completed with NULL connection and no error code. Investigate.",
            (void *)manager);
        error_code = AWS_ERROR_UNKNOWN;
    }

    struct aws_http_connection_acquisition *pending_acquisition =
        AWS_CONTAINER_OF(node, struct aws_http_connection_acquisition, node);
    pending_acquisition->connection = connection;
    pending_acquisition->error_code = error_code;

    aws_linked_list_push_back(output_list, node);
}

/*
 * Encompasses all of the external operations that need to be done for various
 * events:
 *   manager release
 *   connection release
 *   connection acquire
 *   connection_setup
 *   connection_shutdown
 *
 * The transaction is built under the manager's lock (and the internal state is updated optimistically),
 * but then executed outside of it.
 */
struct aws_connection_management_transaction {
    struct aws_http_connection_manager *manager;
    struct aws_allocator *allocator;
    struct aws_linked_list completions;
    struct aws_http_connection *connection_to_release;
    struct aws_linked_list connections_to_release; /* <struct aws_idle_connection> */
    struct aws_http_connection_manager_snapshot snapshot;
    size_t new_connections;
    bool should_destroy_manager;
};

static void s_aws_connection_management_transaction_init(
    struct aws_connection_management_transaction *work,
    struct aws_http_connection_manager *manager) {
    AWS_ZERO_STRUCT(*work);

    aws_linked_list_init(&work->connections_to_release);
    aws_linked_list_init(&work->completions);
    work->manager = manager;
    work->allocator = manager->allocator;
}

static void s_aws_connection_management_transaction_clean_up(struct aws_connection_management_transaction *work) {
    AWS_FATAL_ASSERT(aws_linked_list_empty(&work->connections_to_release));
    AWS_FATAL_ASSERT(aws_linked_list_empty(&work->completions));
}

static void s_aws_http_connection_manager_build_transaction(struct aws_connection_management_transaction *work) {
    struct aws_http_connection_manager *manager = work->manager;

    if (manager->state == AWS_HCMST_READY) {
        /*
         * Step 1 - If there's free connections, complete acquisition requests
         */
        while (!aws_linked_list_empty(&manager->idle_connections) > 0 && manager->pending_acquisition_count > 0) {
            AWS_FATAL_ASSERT(manager->idle_connection_count >= 1);
            /*
             * It is absolutely critical that this is pop_back and not front.  By making the idle connections
             * a LIFO stack, the list will always be sorted from oldest (in terms of idle time) to newest.  This means
             * we can always use the cull timestamp of the first connection as the next scheduled time for culling.
             * It also means that when we cull connections, we can quit the loop as soon as we find a connection
             * whose timestamp is greater than the current timestamp.
             */
            struct aws_linked_list_node *node = aws_linked_list_pop_back(&manager->idle_connections);
            struct aws_idle_connection *idle_connection = AWS_CONTAINER_OF(node, struct aws_idle_connection, node);
            struct aws_http_connection *connection = idle_connection->connection;

            AWS_LOGF_DEBUG(
                AWS_LS_HTTP_CONNECTION_MANAGER,
                "id=%p: Grabbing pooled connection (%p)",
                (void *)manager,
                (void *)connection);
            s_aws_http_connection_manager_move_front_acquisition(
                manager, connection, AWS_ERROR_SUCCESS, &work->completions);
            ++manager->vended_connection_count;
            --manager->idle_connection_count;
            aws_mem_release(idle_connection->allocator, idle_connection);
        }

        /*
         * Step 2 - if there's excess pending acquisitions and we have room to make more, make more
         */
        if (manager->pending_acquisition_count > manager->pending_connects_count) {
            AWS_FATAL_ASSERT(
                manager->max_connections >= manager->vended_connection_count + manager->pending_connects_count);

            work->new_connections = manager->pending_acquisition_count - manager->pending_connects_count;
            size_t max_new_connections =
                manager->max_connections - (manager->vended_connection_count + manager->pending_connects_count);

            if (work->new_connections > max_new_connections) {
                work->new_connections = max_new_connections;
            }

            manager->pending_connects_count += work->new_connections;
        }
    } else {
        /*
         * swap our internal connection set with the empty work set
         */
        AWS_FATAL_ASSERT(aws_linked_list_empty(&work->connections_to_release));
        aws_linked_list_swap_contents(&manager->idle_connections, &work->connections_to_release);
        manager->idle_connection_count = 0;

        /*
         * Move all manager pending acquisitions to the work completion list
         */
        while (!aws_linked_list_empty(&manager->pending_acquisitions)) {
            AWS_LOGF_DEBUG(
                AWS_LS_HTTP_CONNECTION_MANAGER,
                "id=%p: Failing pending connection acquisition due to manager shut down",
                (void *)manager);
            s_aws_http_connection_manager_move_front_acquisition(
                manager, NULL, AWS_ERROR_HTTP_CONNECTION_MANAGER_SHUTTING_DOWN, &work->completions);
        }

        AWS_LOGF_INFO(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: manager release, failing %zu pending acquisitions",
            (void *)manager,
            manager->pending_acquisition_count);
        manager->pending_acquisition_count = 0;

        work->should_destroy_manager = s_aws_http_connection_manager_should_destroy(manager);
    }

    s_aws_http_connection_manager_get_snapshot(manager, &work->snapshot);
}

static void s_aws_http_connection_manager_execute_transaction(struct aws_connection_management_transaction *work);

/*
 * The final last gasp of a connection manager where memory is cleaned up.  Destruction is split up into two parts,
 * a begin and a finish.  Idle connection culling requires a scheduled task on an arbitrary event loop.  If idle
 * connection culling is on then this task must be cancelled before destruction can finish, but you can only cancel
 * a task from the same event loop that it is scheduled on.  To resolve this, when using idle connection culling,
 * we schedule a finish destruction task on the event loop that the culling task is on.  This finish task
 * cancels the culling task and then calls this function.  If we are not using idle connection culling, we can
 * call this function immediately from the start of destruction.
 */
static void s_aws_http_connection_manager_finish_destroy(struct aws_http_connection_manager *manager) {
    if (manager == NULL) {
        return;
    }

    AWS_LOGF_INFO(AWS_LS_HTTP_CONNECTION_MANAGER, "id=%p: Destroying self", (void *)manager);

    AWS_FATAL_ASSERT(manager->pending_connects_count == 0);
    AWS_FATAL_ASSERT(manager->vended_connection_count == 0);
    AWS_FATAL_ASSERT(manager->pending_acquisition_count == 0);
    AWS_FATAL_ASSERT(manager->open_connection_count == 0);
    AWS_FATAL_ASSERT(aws_linked_list_empty(&manager->pending_acquisitions));
    AWS_FATAL_ASSERT(aws_linked_list_empty(&manager->idle_connections));

    aws_string_destroy(manager->host);
    if (manager->initial_settings) {
        aws_array_list_clean_up(manager->initial_settings);
        aws_mem_release(manager->allocator, manager->initial_settings);
    }
    if (manager->tls_connection_options) {
        aws_tls_connection_options_clean_up(manager->tls_connection_options);
        aws_mem_release(manager->allocator, manager->tls_connection_options);
    }
    if (manager->proxy_ev_tls_options) {
        aws_tls_connection_options_clean_up(manager->proxy_ev_tls_options);
        aws_mem_release(manager->allocator, manager->proxy_ev_tls_options);
    }
    if (manager->proxy_config) {
        aws_http_proxy_config_destroy(manager->proxy_config);
    }

    /*
     * If this task exists then we are actually in the corresponding event loop running the final destruction task.
     * In that case, we've already cancelled this task and when you cancel, it runs synchronously.  So in that
     * case the task has run as cancelled, it was not rescheduled, and so we can safely release the memory.
     */
    if (manager->cull_task) {
        aws_mem_release(manager->allocator, manager->cull_task);
    }

    aws_mutex_clean_up(&manager->lock);

    aws_client_bootstrap_release(manager->bootstrap);

    if (manager->shutdown_complete_callback) {
        manager->shutdown_complete_callback(manager->shutdown_complete_user_data);
    }

    aws_mem_release(manager->allocator, manager);
}

/* This is scheduled to run on the cull task's event loop.  If there's no cull task we just destroy the
 * manager directly without a cross-thread task.  */
static void s_final_destruction_task(struct aws_task *task, void *arg, enum aws_task_status status) {
    (void)status;
    struct aws_http_connection_manager *manager = arg;
    struct aws_allocator *allocator = manager->allocator;

    if (manager->cull_task) {
        AWS_FATAL_ASSERT(manager->cull_event_loop != NULL);
        aws_event_loop_cancel_task(manager->cull_event_loop, manager->cull_task);
    }

    s_aws_http_connection_manager_finish_destroy(manager);

    aws_mem_release(allocator, task);
}

static void s_aws_http_connection_manager_begin_destroy(struct aws_http_connection_manager *manager) {
    if (manager == NULL) {
        return;
    }

    /*
     * If we have a cull task running then we have to cancel it.  But you can only cancel tasks within the event
     * loop that the task is scheduled on.  So to solve this case, if there's a cull task, rather than doing
     * cleanup synchronously, we schedule a final destruction task (on the cull event loop) which cancels the
     * cull task before going on to release all the memory and notify the shutdown callback.
     *
     * If there's no cull task we can just cleanup synchronously.
     */
    if (manager->cull_event_loop != NULL) {
        AWS_FATAL_ASSERT(manager->cull_task);
        struct aws_task *final_destruction_task = aws_mem_calloc(manager->allocator, 1, sizeof(struct aws_task));
        aws_task_init(final_destruction_task, s_final_destruction_task, manager, "final_scheduled_destruction");
        aws_event_loop_schedule_task_now(manager->cull_event_loop, final_destruction_task);
    } else {
        s_aws_http_connection_manager_finish_destroy(manager);
    }
}

static void s_cull_task(struct aws_task *task, void *arg, enum aws_task_status status);
static void s_schedule_connection_culling(struct aws_http_connection_manager *manager) {
    if (manager->max_connection_idle_in_milliseconds == 0) {
        return;
    }

    if (manager->cull_task == NULL) {
        manager->cull_task = aws_mem_calloc(manager->allocator, 1, sizeof(struct aws_task));
        if (manager->cull_task == NULL) {
            return;
        }

        aws_task_init(manager->cull_task, s_cull_task, manager, "cull_idle_connections");
    }

    if (manager->cull_event_loop == NULL) {
        manager->cull_event_loop = aws_event_loop_group_get_next_loop(manager->bootstrap->event_loop_group);
    }

    if (manager->cull_event_loop == NULL) {
        goto on_error;
    }

    uint64_t cull_task_time = 0;
    const struct aws_linked_list_node *end = aws_linked_list_end(&manager->idle_connections);
    struct aws_linked_list_node *oldest_node = aws_linked_list_begin(&manager->idle_connections);
    if (oldest_node != end) {
        /*
         * Since the connections are in LIFO order in the list, the front of the list has the closest
         * cull time.
         */
        struct aws_idle_connection *oldest_idle_connection =
            AWS_CONTAINER_OF(oldest_node, struct aws_idle_connection, node);
        cull_task_time = oldest_idle_connection->cull_timestamp;
    } else {
        /*
         * There are no connections in the list, so the absolute minimum anything could be culled is the full
         * culling interval from now.
         */
        uint64_t now = 0;
        if (manager->system_vtable->get_monotonic_time(&now)) {
            goto on_error;
        }
        cull_task_time =
            now + aws_timestamp_convert(
                      manager->max_connection_idle_in_milliseconds, AWS_TIMESTAMP_MILLIS, AWS_TIMESTAMP_NANOS, NULL);
    }

    aws_event_loop_schedule_task_future(manager->cull_event_loop, manager->cull_task, cull_task_time);

    return;

on_error:

    manager->cull_event_loop = NULL;
    aws_mem_release(manager->allocator, manager->cull_task);
    manager->cull_task = NULL;
}

struct aws_http_connection_manager *aws_http_connection_manager_new(
    struct aws_allocator *allocator,
    struct aws_http_connection_manager_options *options) {

    aws_http_fatal_assert_library_initialized();

    if (!options) {
        AWS_LOGF_ERROR(AWS_LS_HTTP_CONNECTION_MANAGER, "Invalid options - options is null");
        aws_raise_error(AWS_ERROR_INVALID_ARGUMENT);
        return NULL;
    }

    if (!options->socket_options) {
        AWS_LOGF_ERROR(AWS_LS_HTTP_CONNECTION_MANAGER, "Invalid options - socket_options is null");
        aws_raise_error(AWS_ERROR_INVALID_ARGUMENT);
        return NULL;
    }

    if (options->max_connections == 0) {
        AWS_LOGF_ERROR(AWS_LS_HTTP_CONNECTION_MANAGER, "Invalid options - max_connections cannot be 0");
        aws_raise_error(AWS_ERROR_INVALID_ARGUMENT);
        return NULL;
    }

    struct aws_http_connection_manager *manager =
        aws_mem_calloc(allocator, 1, sizeof(struct aws_http_connection_manager));
    if (manager == NULL) {
        return NULL;
    }

    manager->allocator = allocator;

    if (aws_mutex_init(&manager->lock)) {
        goto on_error;
    }

    aws_linked_list_init(&manager->idle_connections);
    aws_linked_list_init(&manager->pending_acquisitions);

    manager->host = aws_string_new_from_cursor(allocator, &options->host);
    if (manager->host == NULL) {
        goto on_error;
    }

    if (options->tls_connection_options) {
        manager->tls_connection_options = aws_mem_calloc(allocator, 1, sizeof(struct aws_tls_connection_options));
        if (aws_tls_connection_options_copy(manager->tls_connection_options, options->tls_connection_options)) {
            goto on_error;
        }
    }
    if (options->proxy_options) {
        manager->proxy_config = aws_http_proxy_config_new_from_manager_options(allocator, options);
        if (manager->proxy_config == NULL) {
            goto on_error;
        }
    }

    if (options->monitoring_options) {
        manager->monitoring_options = *options->monitoring_options;
    }

    manager->state = AWS_HCMST_READY;
    manager->initial_window_size = options->initial_window_size;
    manager->port = options->port;
    manager->max_connections = options->max_connections;
    manager->socket_options = *options->socket_options;
    manager->bootstrap = aws_client_bootstrap_acquire(options->bootstrap);
    manager->system_vtable = g_aws_http_connection_manager_default_system_vtable_ptr;
    manager->external_ref_count = 1;
    manager->shutdown_complete_callback = options->shutdown_complete_callback;
    manager->shutdown_complete_user_data = options->shutdown_complete_user_data;
    manager->enable_read_back_pressure = options->enable_read_back_pressure;
    manager->max_connection_idle_in_milliseconds = options->max_connection_idle_in_milliseconds;
    if (options->proxy_ev_settings) {
        manager->proxy_ev_settings = *options->proxy_ev_settings;
    }
    if (manager->proxy_ev_settings.tls_options) {
        manager->proxy_ev_tls_options = aws_mem_calloc(allocator, 1, sizeof(struct aws_tls_connection_options));
        if (aws_tls_connection_options_copy(manager->proxy_ev_tls_options, manager->proxy_ev_settings.tls_options)) {
            goto on_error;
        }
        manager->proxy_ev_settings.tls_options = manager->proxy_ev_tls_options;
    }
    manager->prior_knowledge_http2 = options->prior_knowledge_http2;
    if (options->num_initial_settings > 0) {
        manager->initial_settings = aws_mem_calloc(allocator, 1, sizeof(struct aws_array_list));
        aws_array_list_init_dynamic(
            manager->initial_settings, allocator, options->num_initial_settings, sizeof(struct aws_http2_setting));
        memcpy(
            manager->initial_settings->data,
            options->initial_settings_array,
            options->num_initial_settings * sizeof(struct aws_http2_setting));
    }
    manager->max_closed_streams = options->max_closed_streams;
    manager->http2_conn_manual_window_management = options->http2_conn_manual_window_management;

    s_schedule_connection_culling(manager);

    AWS_LOGF_INFO(AWS_LS_HTTP_CONNECTION_MANAGER, "id=%p: Successfully created", (void *)manager);

    return manager;

on_error:

    s_aws_http_connection_manager_begin_destroy(manager);

    return NULL;
}

void aws_http_connection_manager_acquire(struct aws_http_connection_manager *manager) {
    aws_mutex_lock(&manager->lock);
    AWS_FATAL_ASSERT(manager->external_ref_count > 0);
    manager->external_ref_count += 1;
    aws_mutex_unlock(&manager->lock);
}

void aws_http_connection_manager_release(struct aws_http_connection_manager *manager) {
    struct aws_connection_management_transaction work;
    s_aws_connection_management_transaction_init(&work, manager);

    AWS_LOGF_INFO(AWS_LS_HTTP_CONNECTION_MANAGER, "id=%p: release", (void *)manager);

    aws_mutex_lock(&manager->lock);

    if (manager->external_ref_count > 0) {
        manager->external_ref_count -= 1;

        if (manager->external_ref_count == 0) {
            AWS_LOGF_INFO(
                AWS_LS_HTTP_CONNECTION_MANAGER,
                "id=%p: ref count now zero, starting shut down process",
                (void *)manager);
            manager->state = AWS_HCMST_SHUTTING_DOWN;
            s_aws_http_connection_manager_build_transaction(&work);
        }
    } else {
        AWS_LOGF_ERROR(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Connection manager release called with a zero reference count",
            (void *)manager);
    }

    aws_mutex_unlock(&manager->lock);

    s_aws_http_connection_manager_execute_transaction(&work);
}

static void s_aws_http_connection_manager_on_connection_setup(
    struct aws_http_connection *connection,
    int error_code,
    void *user_data);

static void s_aws_http_connection_manager_on_connection_shutdown(
    struct aws_http_connection *connection,
    int error_code,
    void *user_data);

static void s_aws_http_connection_manager_h2_on_goaway_received(
    struct aws_http_connection *http2_connection,
    uint32_t last_stream_id,
    uint32_t http2_error_code,
    struct aws_byte_cursor debug_data,
    void *user_data);

static int s_aws_http_connection_manager_new_connection(struct aws_http_connection_manager *manager) {
    struct aws_http_client_connection_options options;
    AWS_ZERO_STRUCT(options);
    options.self_size = sizeof(struct aws_http_client_connection_options);
    options.bootstrap = manager->bootstrap;
    options.tls_options = manager->tls_connection_options;
    options.allocator = manager->allocator;
    options.user_data = manager;
    options.host_name = aws_byte_cursor_from_string(manager->host);
    options.port = manager->port;
    options.initial_window_size = manager->initial_window_size;
    options.socket_options = &manager->socket_options;
    options.on_setup = s_aws_http_connection_manager_on_connection_setup;
    options.on_shutdown = s_aws_http_connection_manager_on_connection_shutdown;
    options.manual_window_management = manager->enable_read_back_pressure;
    options.proxy_ev_settings = &manager->proxy_ev_settings;
    options.prior_knowledge_http2 = manager->prior_knowledge_http2;

    struct aws_http2_connection_options h2_options;
    AWS_ZERO_STRUCT(h2_options);
    if (manager->initial_settings) {
        h2_options.initial_settings_array = manager->initial_settings->data;
        h2_options.num_initial_settings = aws_array_list_length(manager->initial_settings);
    }
    h2_options.max_closed_streams = manager->max_closed_streams;
    h2_options.conn_manual_window_management = manager->http2_conn_manual_window_management;
    h2_options.on_goaway_received = s_aws_http_connection_manager_h2_on_goaway_received;

    options.http2_options = &h2_options;

    if (aws_http_connection_monitoring_options_is_valid(&manager->monitoring_options)) {
        options.monitoring_options = &manager->monitoring_options;
    }

    struct aws_http_proxy_options proxy_options;
    AWS_ZERO_STRUCT(proxy_options);

    if (manager->proxy_config) {
        aws_http_proxy_options_init_from_config(&proxy_options, manager->proxy_config);
        options.proxy_options = &proxy_options;
    }

    if (manager->system_vtable->create_connection(&options)) {
        AWS_LOGF_ERROR(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: http connection creation failed with error code %d(%s)",
            (void *)manager,
            aws_last_error(),
            aws_error_str(aws_last_error()));
        return AWS_OP_ERR;
    }

    return AWS_OP_SUCCESS;
}

static void s_aws_http_connection_manager_execute_transaction(struct aws_connection_management_transaction *work) {

    struct aws_http_connection_manager *manager = work->manager;

    bool should_destroy = work->should_destroy_manager;
    int representative_error = 0;
    size_t new_connection_failures = 0;

    /*
     * Step 1 - Logging
     */
    s_aws_http_connection_manager_log_snapshot(manager, &work->snapshot);

    /*
     * Step 2 - Perform any requested connection releases
     */
    while (!aws_linked_list_empty(&work->connections_to_release)) {
        struct aws_linked_list_node *node = aws_linked_list_pop_back(&work->connections_to_release);
        struct aws_idle_connection *idle_connection = AWS_CONTAINER_OF(node, struct aws_idle_connection, node);

        AWS_LOGF_INFO(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Releasing connection (id=%p)",
            (void *)manager,
            (void *)idle_connection->connection);
        manager->system_vtable->release_connection(idle_connection->connection);
        aws_mem_release(idle_connection->allocator, idle_connection);
    }

    if (work->connection_to_release) {
        AWS_LOGF_INFO(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Releasing connection (id=%p)",
            (void *)manager,
            (void *)work->connection_to_release);
        manager->system_vtable->release_connection(work->connection_to_release);
    }

    /*
     * Step 3 - Make new connections
     */
    struct aws_array_list errors;
    AWS_ZERO_STRUCT(errors);
    /* Even if we can't init this array, we still need to invoke error callbacks properly */
    bool push_errors = false;

    if (work->new_connections > 0) {
        AWS_LOGF_INFO(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Requesting %zu new connections from http",
            (void *)manager,
            work->new_connections);
        push_errors = aws_array_list_init_dynamic(&errors, work->allocator, work->new_connections, sizeof(int)) ==
                      AWS_ERROR_SUCCESS;
    }

    for (size_t i = 0; i < work->new_connections; ++i) {
        if (s_aws_http_connection_manager_new_connection(manager)) {
            ++new_connection_failures;
            representative_error = aws_last_error();
            if (push_errors) {
                AWS_FATAL_ASSERT(aws_array_list_push_back(&errors, &representative_error) == AWS_OP_SUCCESS);
            }
        }
    }

    if (new_connection_failures > 0) {
        /*
         * We failed and aren't going to receive a callback, but the current state assumes we will receive
         * a callback.  So we need to re-lock and update the state ourselves.
         */
        aws_mutex_lock(&manager->lock);

        AWS_FATAL_ASSERT(manager->pending_connects_count >= new_connection_failures);
        manager->pending_connects_count -= new_connection_failures;

        /*
         * Rather than failing one acquisition for each connection failure, if there's at least one
         * connection failure, we instead fail all excess acquisitions, since there's no pending
         * connect that will necessarily resolve them.
         *
         * Try to correspond an error with the acquisition failure, but as a fallback just use the
         * representative error.
         */
        size_t i = 0;
        while (manager->pending_acquisition_count > manager->pending_connects_count) {
            int error = representative_error;
            if (i < aws_array_list_length(&errors)) {
                aws_array_list_get_at(&errors, &error, i);
            }

            AWS_LOGF_DEBUG(
                AWS_LS_HTTP_CONNECTION_MANAGER,
                "id=%p: Failing excess connection acquisition with error code %d",
                (void *)manager,
                (int)error);
            s_aws_http_connection_manager_move_front_acquisition(manager, NULL, error, &work->completions);
            ++i;
        }

        should_destroy = s_aws_http_connection_manager_should_destroy(manager);

        aws_mutex_unlock(&manager->lock);
    }

    /*
     * Step 4 - Perform acquisition callbacks
     */
    s_aws_http_connection_manager_complete_acquisitions(&work->completions, work->allocator);

    aws_array_list_clean_up(&errors);

    /*
     * Step 5 - destroy the manager if necessary
     */
    if (should_destroy) {
        s_aws_http_connection_manager_begin_destroy(manager);
    }

    /*
     * Step 6 - Clean up work.  Do this here rather than at the end of every caller.
     */
    s_aws_connection_management_transaction_clean_up(work);
}

void aws_http_connection_manager_acquire_connection(
    struct aws_http_connection_manager *manager,
    aws_http_connection_manager_on_connection_setup_fn *callback,
    void *user_data) {

    AWS_LOGF_DEBUG(AWS_LS_HTTP_CONNECTION_MANAGER, "id=%p: Acquire connection", (void *)manager);

    struct aws_http_connection_acquisition *request =
        aws_mem_calloc(manager->allocator, 1, sizeof(struct aws_http_connection_acquisition));
    if (request == NULL) {
        callback(NULL, aws_last_error(), user_data);
        return;
    }

    request->allocator = manager->allocator;
    request->callback = callback;
    request->user_data = user_data;
    request->manager = manager;

    struct aws_connection_management_transaction work;
    s_aws_connection_management_transaction_init(&work, manager);

    aws_mutex_lock(&manager->lock);

    if (manager->state != AWS_HCMST_READY) {
        AWS_LOGF_ERROR(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Acquire connection called when manager in shut down state",
            (void *)manager);

        request->error_code = AWS_ERROR_HTTP_CONNECTION_MANAGER_INVALID_STATE_FOR_ACQUIRE;
    }

    aws_linked_list_push_back(&manager->pending_acquisitions, &request->node);
    ++manager->pending_acquisition_count;

    s_aws_http_connection_manager_build_transaction(&work);

    aws_mutex_unlock(&manager->lock);

    s_aws_http_connection_manager_execute_transaction(&work);
}

static int s_idle_connection(struct aws_http_connection_manager *manager, struct aws_http_connection *connection) {
    struct aws_idle_connection *idle_connection =
        aws_mem_calloc(manager->allocator, 1, sizeof(struct aws_idle_connection));
    if (idle_connection == NULL) {
        return AWS_OP_ERR;
    }

    idle_connection->allocator = manager->allocator;
    idle_connection->connection = connection;

    uint64_t idle_start_timestamp = 0;
    if (manager->system_vtable->get_monotonic_time(&idle_start_timestamp)) {
        goto on_error;
    }

    idle_connection->cull_timestamp =
        idle_start_timestamp +
        aws_timestamp_convert(
            manager->max_connection_idle_in_milliseconds, AWS_TIMESTAMP_MILLIS, AWS_TIMESTAMP_NANOS, NULL);

    aws_linked_list_push_back(&manager->idle_connections, &idle_connection->node);
    ++manager->idle_connection_count;

    return AWS_OP_SUCCESS;

on_error:

    aws_mem_release(idle_connection->allocator, idle_connection);

    return AWS_OP_ERR;
}

int aws_http_connection_manager_release_connection(
    struct aws_http_connection_manager *manager,
    struct aws_http_connection *connection) {

    struct aws_connection_management_transaction work;
    s_aws_connection_management_transaction_init(&work, manager);

    int result = AWS_OP_ERR;
    bool should_release_connection = !manager->system_vtable->is_connection_available(connection);

    AWS_LOGF_DEBUG(
        AWS_LS_HTTP_CONNECTION_MANAGER, "id=%p: Releasing connection (id=%p)", (void *)manager, (void *)connection);

    aws_mutex_lock(&manager->lock);

    /* We're probably hosed in this case, but let's not underflow */
    if (manager->vended_connection_count == 0) {
        AWS_LOGF_FATAL(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Connection released when vended connection count is zero",
            (void *)manager);
        aws_raise_error(AWS_ERROR_HTTP_CONNECTION_MANAGER_VENDED_CONNECTION_UNDERFLOW);
        goto release;
    }

    result = AWS_OP_SUCCESS;

    --manager->vended_connection_count;

    if (!should_release_connection) {
        if (s_idle_connection(manager, connection)) {
            should_release_connection = true;
        }
    }

    s_aws_http_connection_manager_build_transaction(&work);
    if (should_release_connection) {
        work.connection_to_release = connection;
    }

release:

    aws_mutex_unlock(&manager->lock);

    s_aws_http_connection_manager_execute_transaction(&work);

    return result;
}

static void s_aws_http_connection_manager_h2_on_goaway_received(
    struct aws_http_connection *http2_connection,
    uint32_t last_stream_id,
    uint32_t http2_error_code,
    struct aws_byte_cursor debug_data,
    void *user_data) {
    struct aws_http_connection_manager *manager = user_data;
    /* We don't offer user the details, but we can still log it out for debugging */
    AWS_LOGF_DEBUG(
        AWS_LS_HTTP_CONNECTION_MANAGER,
        "id=%p: HTTP/2 connection (id=%p) received GOAWAY with: last stream id - %u, error code - %u, debug data - "
        "\"%.*s\"",
        (void *)manager,
        (void *)http2_connection,
        last_stream_id,
        http2_error_code,
        (int)debug_data.len,
        debug_data.ptr);

    struct aws_connection_management_transaction work;
    s_aws_connection_management_transaction_init(&work, manager);
    /* Release this connection as no new stream will be allowed */
    work.connection_to_release = http2_connection;
    aws_mutex_lock(&manager->lock);
    s_aws_http_connection_manager_build_transaction(&work);
    aws_mutex_unlock(&manager->lock);
    s_aws_http_connection_manager_execute_transaction(&work);
}

static void s_aws_http_connection_manager_on_connection_setup(
    struct aws_http_connection *connection,
    int error_code,
    void *user_data) {
    struct aws_http_connection_manager *manager = user_data;

    struct aws_connection_management_transaction work;
    s_aws_connection_management_transaction_init(&work, manager);

    if (connection != NULL) {
        AWS_LOGF_DEBUG(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Received new connection (id=%p) from http layer",
            (void *)manager,
            (void *)connection);
    } else {
        AWS_LOGF_WARN(
            AWS_LS_HTTP_CONNECTION_MANAGER,
            "id=%p: Failed to obtain new connection from http layer, error %d(%s)",
            (void *)manager,
            error_code,
            aws_error_str(error_code));
    }

    aws_mutex_lock(&manager->lock);

    bool is_shutting_down = manager->state == AWS_HCMST_SHUTTING_DOWN;

    AWS_FATAL_ASSERT(manager->pending_connects_count > 0);
    --manager->pending_connects_count;

    if (connection != NULL) {
        if (is_shutting_down || s_idle_connection(manager, connection)) {
            /*
             * release it immediately
             */
            AWS_LOGF_DEBUG(
                AWS_LS_HTTP_CONNECTION_MANAGER,
                "id=%p: New connection (id=%p) releasing immediately",
                (void *)manager,
                (void *)connection);
            work.connection_to_release = connection;
        }
        ++manager->open_connection_count;
    } else {
        /*
         * To be safe, if we have an excess of pending acquisitions (beyond the number of pending
         * connects), we need to fail all of the excess.  Technically, we might be able to try and
         * make a new connection, if there's room, but that could lead to some bad failure loops.
         *
         * This won't happen during shutdown since there are no pending acquisitions at that point.
         */
        while (manager->pending_acquisition_count > manager->pending_connects_count) {
            AWS_LOGF_DEBUG(
                AWS_LS_HTTP_CONNECTION_MANAGER,
                "id=%p: Failing excess connection acquisition with error code %d",
                (void *)manager,
                (int)error_code);
            s_aws_http_connection_manager_move_front_acquisition(manager, NULL, error_code, &work.completions);
        }
    }

    s_aws_http_connection_manager_build_transaction(&work);

    aws_mutex_unlock(&manager->lock);

    s_aws_http_connection_manager_execute_transaction(&work);
}

static void s_aws_http_connection_manager_on_connection_shutdown(
    struct aws_http_connection *connection,
    int error_code,
    void *user_data) {
    (void)error_code;

    struct aws_http_connection_manager *manager = user_data;

    AWS_LOGF_DEBUG(
        AWS_LS_HTTP_CONNECTION_MANAGER,
        "id=%p: shutdown received for connection (id=%p)",
        (void *)manager,
        (void *)connection);

    struct aws_connection_management_transaction work;
    s_aws_connection_management_transaction_init(&work, manager);

    aws_mutex_lock(&manager->lock);

    AWS_FATAL_ASSERT(manager->open_connection_count > 0);
    --manager->open_connection_count;

    /*
     * Find and, if found, remove it from idle connections
     */
    const struct aws_linked_list_node *end = aws_linked_list_end(&manager->idle_connections);
    for (struct aws_linked_list_node *node = aws_linked_list_begin(&manager->idle_connections); node != end;
         node = aws_linked_list_next(node)) {
        struct aws_idle_connection *current_idle_connection = AWS_CONTAINER_OF(node, struct aws_idle_connection, node);
        if (current_idle_connection->connection == connection) {
            aws_linked_list_remove(node);
            work.connection_to_release = connection;
            aws_mem_release(current_idle_connection->allocator, current_idle_connection);
            --manager->idle_connection_count;
            break;
        }
    }

    s_aws_http_connection_manager_build_transaction(&work);

    aws_mutex_unlock(&manager->lock);

    s_aws_http_connection_manager_execute_transaction(&work);
}

static void s_cull_idle_connections(struct aws_http_connection_manager *manager) {
    AWS_LOGF_INFO(AWS_LS_HTTP_CONNECTION_MANAGER, "id=%p: culling idle connections", (void *)manager);

    if (manager == NULL || manager->max_connection_idle_in_milliseconds == 0) {
        return;
    }

    uint64_t now = 0;
    if (manager->system_vtable->get_monotonic_time(&now)) {
        return;
    }

    struct aws_connection_management_transaction work;
    s_aws_connection_management_transaction_init(&work, manager);

    aws_mutex_lock(&manager->lock);

    /* Only if we're not shutting down */
    if (manager->state == AWS_HCMST_READY) {
        const struct aws_linked_list_node *end = aws_linked_list_end(&manager->idle_connections);
        struct aws_linked_list_node *current_node = aws_linked_list_begin(&manager->idle_connections);
        while (current_node != end) {
            struct aws_linked_list_node *node = current_node;
            struct aws_idle_connection *current_idle_connection =
                AWS_CONTAINER_OF(node, struct aws_idle_connection, node);
            if (current_idle_connection->cull_timestamp > now) {
                break;
            }

            current_node = aws_linked_list_next(current_node);
            aws_linked_list_remove(node);
            aws_linked_list_push_back(&work.connections_to_release, node);
            --manager->idle_connection_count;

            AWS_LOGF_DEBUG(
                AWS_LS_HTTP_CONNECTION_MANAGER,
                "id=%p: culling idle connection (%p)",
                (void *)manager,
                (void *)current_idle_connection->connection);
        }
    }

    s_aws_http_connection_manager_get_snapshot(manager, &work.snapshot);

    aws_mutex_unlock(&manager->lock);

    s_aws_http_connection_manager_execute_transaction(&work);
}

static void s_cull_task(struct aws_task *task, void *arg, enum aws_task_status status) {
    (void)task;
    if (status != AWS_TASK_STATUS_RUN_READY) {
        return;
    }

    struct aws_http_connection_manager *manager = arg;

    s_cull_idle_connections(manager);

    s_schedule_connection_culling(manager);
}
