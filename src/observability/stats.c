#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "stats.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/*============================================================================
 * Global state
 *============================================================================*/
static Statistics g_stats = {0};
static int g_stats_initialized = 0;

/* Maximum tables we track */
#define MAX_TRACKED_TABLES 64
static TableStats g_table_stats[MAX_TRACKED_TABLES];
static int g_table_stats_count = 0;

/*============================================================================
 * Initialization
 *============================================================================*/
void stats_init(void) {
    if (g_stats_initialized) return;
    memset(&g_stats, 0, sizeof(g_stats));
    memset(g_table_stats, 0, sizeof(g_table_stats));
    g_table_stats_count = 0;
    g_stats_initialized = 1;
}

void stats_destroy(void) {
    g_stats_initialized = 0;
}

/*============================================================================
 * Counter operations
 *============================================================================*/
void stats_increment(const char* counter) {
    stats_increment_by(counter, 1);
}

void stats_increment_by(const char* counter, uint64_t value) {
    if (!counter || !g_stats_initialized) return;

    if (strcmp(counter, "connections_total") == 0) {
        __atomic_add_fetch(&g_stats.connections_total, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "connections_failed") == 0) {
        __atomic_add_fetch(&g_stats.connections_failed, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "connections_active") == 0) {
        __atomic_add_fetch(&g_stats.connections_active, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "queries_total") == 0) {
        __atomic_add_fetch(&g_stats.queries_total, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "queries_select") == 0) {
        __atomic_add_fetch(&g_stats.queries_select, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "queries_insert") == 0) {
        __atomic_add_fetch(&g_stats.queries_insert, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "queries_update") == 0) {
        __atomic_add_fetch(&g_stats.queries_update, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "queries_delete") == 0) {
        __atomic_add_fetch(&g_stats.queries_delete, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "queries_failed") == 0) {
        __atomic_add_fetch(&g_stats.queries_failed, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "transactions_total") == 0) {
        __atomic_add_fetch(&g_stats.transactions_total, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "transactions_committed") == 0) {
        __atomic_add_fetch(&g_stats.transactions_committed, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "transactions_rolled_back") == 0) {
        __atomic_add_fetch(&g_stats.transactions_rolled_back, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "pages_read") == 0) {
        __atomic_add_fetch(&g_stats.pages_read, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "pages_written") == 0) {
        __atomic_add_fetch(&g_stats.pages_written, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "pages_cache_hit") == 0) {
        __atomic_add_fetch(&g_stats.pages_cache_hit, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "pages_cache_miss") == 0) {
        __atomic_add_fetch(&g_stats.pages_cache_miss, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "wal_entries_written") == 0) {
        __atomic_add_fetch(&g_stats.wal_entries_written, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "wal_checkpoints") == 0) {
        __atomic_add_fetch(&g_stats.wal_checkpoints, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "query_time_us") == 0) {
        __atomic_add_fetch(&g_stats.query_time_us, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "cache_evictions") == 0) {
        __atomic_add_fetch(&g_stats.cache_evictions, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "fsync_calls") == 0) {
        __atomic_add_fetch(&g_stats.fsync_calls, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "errors_storage") == 0) {
        __atomic_add_fetch(&g_stats.errors_storage, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "errors_protocol") == 0) {
        __atomic_add_fetch(&g_stats.errors_protocol, value, __ATOMIC_RELAXED);
    } else if (strcmp(counter, "errors_internal") == 0) {
        __atomic_add_fetch(&g_stats.errors_internal, value, __ATOMIC_RELAXED);
    }
}

void stats_gauge_set(const char* gauge, uint64_t value) {
    if (!gauge || !g_stats_initialized) return;

    if (strcmp(gauge, "connections_active") == 0) {
        __atomic_store_n(&g_stats.connections_active, value, __ATOMIC_RELAXED);
    }
}

uint64_t stats_get(const char* counter) {
    if (!counter || !g_stats_initialized) return 0;

    if (strcmp(counter, "connections_total") == 0)
        return __atomic_load_n(&g_stats.connections_total, __ATOMIC_RELAXED);
    if (strcmp(counter, "connections_failed") == 0)
        return __atomic_load_n(&g_stats.connections_failed, __ATOMIC_RELAXED);
    if (strcmp(counter, "connections_active") == 0)
        return __atomic_load_n(&g_stats.connections_active, __ATOMIC_RELAXED);
    if (strcmp(counter, "queries_total") == 0)
        return __atomic_load_n(&g_stats.queries_total, __ATOMIC_RELAXED);
    if (strcmp(counter, "queries_failed") == 0)
        return __atomic_load_n(&g_stats.queries_failed, __ATOMIC_RELAXED);

    return 0;
}

/*============================================================================
 * Convenience increment functions
 *============================================================================*/
void stats_inc_connections_total(void)   { stats_increment("connections_total"); }
void stats_inc_connections_failed(void) { stats_increment("connections_failed"); }
void stats_inc_queries_total(void)      { stats_increment("queries_total"); }
void stats_inc_queries_select(void)     { stats_increment("queries_select"); }
void stats_inc_queries_insert(void)     { stats_increment("queries_insert"); }
void stats_inc_queries_update(void)     { stats_increment("queries_update"); }
void stats_inc_queries_delete(void)     { stats_increment("queries_delete"); }
void stats_inc_transactions_committed(void) { stats_increment("transactions_committed"); }
void stats_inc_transactions_rolled_back(void) { stats_increment("transactions_rolled_back"); }
void stats_inc_pages_read(void)        { stats_increment("pages_read"); }
void stats_inc_pages_written(void)     { stats_increment("pages_written"); }
void stats_inc_cache_hit(void)         { stats_increment("pages_cache_hit"); }
void stats_inc_cache_miss(void)         { stats_increment("pages_cache_miss"); }
void stats_add_query_time(uint64_t us)  { stats_increment_by("query_time_us", us); }

/*============================================================================
 * Table statistics
 *============================================================================*/
void stats_update_table_stats(const char* table_name, int operation, uint64_t row_count_delta) {
    if (!table_name || !g_stats_initialized) return;

    TableStats* ts = NULL;

    /* Find existing entry */
    for (int i = 0; i < g_table_stats_count; i++) {
        if (strcmp(g_table_stats[i].table_name, table_name) == 0) {
            ts = &g_table_stats[i];
            break;
        }
    }

    /* Create new entry if needed */
    if (!ts && g_table_stats_count < MAX_TRACKED_TABLES) {
        ts = &g_table_stats[g_table_stats_count++];
        strncpy(ts->table_name, table_name, sizeof(ts->table_name) - 1);
        ts->table_name[sizeof(ts->table_name) - 1] = '\0';
    }

    if (!ts) return;

    /* Update based on operation (0=scan, 1=insert, 2=update, 3=delete) */
    switch (operation) {
        case 0: ts->total_scans++; break;
        case 1: ts->inserts++; ts->row_count += row_count_delta; break;
        case 2: ts->updates++; break;
        case 3: ts->deletes++; ts->row_count -= row_count_delta; break;
    }
}

TableStats* stats_get_table_stats(const char* table_name) {
    if (!table_name || !g_stats_initialized) return NULL;

    for (int i = 0; i < g_table_stats_count; i++) {
        if (strcmp(g_table_stats[i].table_name, table_name) == 0) {
            return &g_table_stats[i];
        }
    }
    return NULL;
}

/*============================================================================
 * Output
 *============================================================================*/
void stats_dump(void) {
    if (!g_stats_initialized) stats_init();

    fprintf(stderr, "=== TinyDB Statistics ===\n");
    fprintf(stderr, "Connections:   total=%lu failed=%lu active=%lu\n",
            (unsigned long)g_stats.connections_total,
            (unsigned long)g_stats.connections_failed,
            (unsigned long)g_stats.connections_active);
    fprintf(stderr, "Queries:       total=%lu select=%lu insert=%lu update=%lu delete=%lu failed=%lu\n",
            (unsigned long)g_stats.queries_total,
            (unsigned long)g_stats.queries_select,
            (unsigned long)g_stats.queries_insert,
            (unsigned long)g_stats.queries_update,
            (unsigned long)g_stats.queries_delete,
            (unsigned long)g_stats.queries_failed);
    fprintf(stderr, "Transactions:  total=%lu committed=%lu rolled_back=%lu\n",
            (unsigned long)g_stats.transactions_total,
            (unsigned long)g_stats.transactions_committed,
            (unsigned long)g_stats.transactions_rolled_back);
    fprintf(stderr, "Storage:       pages_read=%lu pages_written=%lu\n",
            (unsigned long)g_stats.pages_read,
            (unsigned long)g_stats.pages_written);
    fprintf(stderr, "Cache:         hits=%lu misses=%lu evictions=%lu\n",
            (unsigned long)g_stats.pages_cache_hit,
            (unsigned long)g_stats.pages_cache_miss,
            (unsigned long)g_stats.cache_evictions);
    fprintf(stderr, "WAL:           entries=%lu checkpoints=%lu\n",
            (unsigned long)g_stats.wal_entries_written,
            (unsigned long)g_stats.wal_checkpoints);
    fprintf(stderr, "Performance:   query_time_us=%lu fsync_calls=%lu\n",
            (unsigned long)g_stats.query_time_us,
            (unsigned long)g_stats.fsync_calls);
    fprintf(stderr, "Errors:        storage=%lu protocol=%lu internal=%lu\n",
            (unsigned long)g_stats.errors_storage,
            (unsigned long)g_stats.errors_protocol,
            (unsigned long)g_stats.errors_internal);

    if (g_table_stats_count > 0) {
        fprintf(stderr, "\nTable Statistics:\n");
        for (int i = 0; i < g_table_stats_count; i++) {
            TableStats* ts = &g_table_stats[i];
            fprintf(stderr, "  %s: rows=%lu scans=%lu inserts=%lu updates=%lu deletes=%lu\n",
                    ts->table_name,
                    (unsigned long)ts->row_count,
                    (unsigned long)ts->total_scans,
                    (unsigned long)ts->inserts,
                    (unsigned long)ts->updates,
                    (unsigned long)ts->deletes);
        }
    }
    fprintf(stderr, "========================\n");
}

void stats_reset(void) {
    if (!g_stats_initialized) return;
    memset(&g_stats, 0, sizeof(g_stats));
    memset(g_table_stats, 0, sizeof(g_table_stats));
    g_table_stats_count = 0;
}

/*============================================================================
 * Signal handler for SIGUSR1
 *============================================================================*/
static void sigusr1_handler(int sig) {
    (void)sig;
    stats_dump();
}

void stats_register_sigusr1(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
}

/*============================================================================
 * Global access
 *============================================================================*/
Statistics* stats_get_global(void) {
    return &g_stats;
}