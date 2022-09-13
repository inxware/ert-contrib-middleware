{application, couch, [
    {description, "Apache CouchDB"},
    {vsn, "1.0.1"},
    {modules, [couch_external_manager,couch_rep_writer,couch_doc,couch_native_process,couch_httpd_oauth,couch_rep_reader,couch_config_writer,couch_db_updater,couch,couch_rep,couch_httpd_rewrite,couch_view_compactor,couch_db_update_notifier,couch_changes,couch_external_server,couch_util,couch_db_update_notifier_sup,couch_httpd_external,couch_server,couch_auth_cache,couch_httpd_show,couch_view,couch_config,couch_key_tree,couch_httpd_auth,couch_db,couch_stats_aggregator,couch_server_sup,couch_httpd_db,couch_stream,couch_rep_sup,couch_rep_httpc,couch_view_group,couch_rep_missing_revs,couch_btree,couch_httpd_stats_handlers,couch_work_queue,couch_os_process,couch_httpd_view,couch_rep_changes_feed,couch_file,couch_app,couch_query_servers,couch_event_sup,couch_uuids,couch_stats_collector,couch_httpd_misc_handlers,couch_httpd,couch_ref_counter,couch_view_updater,couch_task_status,couch_rep_att,couch_log]},
    {registered, [
        couch_config,
        couch_db_update,
        couch_db_update_notifier_sup,
        couch_external_manager,
        couch_httpd,
        couch_log,
        couch_primary_services,
        couch_query_servers,
        couch_rep_sup,
        couch_secondary_services,
        couch_server,
        couch_server_sup,
        couch_stats_aggregator,
        couch_stats_collector,
        couch_task_status,
        couch_view
    ]},
    {mod, {couch_app, [
        "/etc/couchdb/default.ini",
        "/etc/couchdb/local.ini"
    ]}},
    {applications, [kernel, stdlib]},
    {included_applications, [crypto, sasl, inets, oauth, ibrowse, mochiweb]}
]}.
