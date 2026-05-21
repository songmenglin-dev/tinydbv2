#ifndef TINYDB_STATS_H
#define TINYDB_STATS_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * Statistics counters
 *============================================================================*/
typedef struct {
    /* Connection statistics */
    uint64_t connections_total;
    uint64_t connections_failed;
    uint64_t connections_active;

    /* Query statistics */
    uint64_t queries_total;
    uint64_t queries_select;
    uint64_t queries_insert;
    uint64_t queries_update;
    uint64_t queries_delete;
    uint64_t queries_failed;

    /* Transaction statistics */
    uint64_t transactions_total;
    uint64_t transactions_committed;
    uint64_t transactions_rolled_back;

    /* Storage statistics */
    uint64_t pages_read;
    uint64_t pages_written;
    uint64_t pages_cache_hit;
    uint64_t pages_cache_miss;
    uint64_t wal_entries_written;
    uint64_t wal_checkpoints;

    /* Performance */
    uint64_t query_time_us;      /* total query execution time in microseconds */
    uint64_t cache_evictions;
    uint64_t fsync_calls;

    /* Errors */
    uint64_t errors_storage;
    uint64_t errors_protocol;
    uint64_t errors_internal;
} Statistics;

/*============================================================================
 * Per-table statistics
 *============================================================================*/
typedef struct {
    char table_name[64];
    uint64_t row_count;
    uint64_t pages_used;
    uint64_t index_size;
    uint64_t total_scans;
    uint64_t range_scans;
    uint64_t point_lookups;
    uint64_t inserts;
    uint64_t updates;
    uint64_t deletes;
} TableStats;

/*============================================================================
 * Statistics interface
 *============================================================================*/

/* Initialize/shutdown statistics subsystem */
void stats_init(void);
void stats_destroy(void);

/* Counter operations */
void stats_increment(const char* counter);
void stats_increment_by(const char* counter, uint64_t value);
void stats_gauge_set(const char* gauge, uint64_t value);
uint64_t stats_get(const char* counter);

/* Bulk operations */
void stats_inc_connections_total(void);
void stats_inc_connections_failed(void);
void stats_inc_queries_total(void);
void stats_inc_queries_select(void);
void stats_inc_queries_insert(void);
void stats_inc_queries_update(void);
void stats_inc_queries_delete(void);
void stats_inc_transactions_committed(void);
void stats_inc_transactions_rolled_back(void);
void stats_inc_pages_read(void);
void stats_inc_pages_written(void);
void stats_inc_cache_hit(void);
void stats_inc_cache_miss(void);
void stats_add_query_time(uint64_t us);

/* Table statistics */
void stats_update_table_stats(const char* table_name, int operation, uint64_t row_count_delta);
TableStats* stats_get_table_stats(const char* table_name);

/* Output */
void stats_dump(void);           /* dump to log */
void stats_reset(void);          /* reset all counters */

/* SIGUSR1 handler registration */
void stats_register_sigusr1(void);

/* Access to global stats */
Statistics* stats_get_global(void);

#endif /* TINYDB_STATS_H */