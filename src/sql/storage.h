#ifndef TINYDB_STORAGE_H
#define TINYDB_STORAGE_H

#include "../../include/tinydb.h"
#include "../storage/pager.h"
#include "../storage/page_cache.h"
#include "catalog.h"

/* Get catalog from storage */
Catalog* storage_get_catalog(Storage* storage);

/* Get pager from storage */
Pager* storage_get_pager(Storage* storage);

/* Get page cache from storage */
PageCache* storage_get_cache(Storage* storage);

#endif /* TINYDB_STORAGE_H */