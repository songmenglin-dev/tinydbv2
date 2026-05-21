#define _POSIX_C_SOURCE 200809L

#include "pager.h"
#include "page_cache.h"
#include <stdlib.h>

/* Page manager module - uses pager and page_cache */

int page_init(void) {
    return 0;
}

Page* page_pin_from_cache(PageCache* cache, uint32_t page_id) {
    return page_pin(cache, page_id);
}

void page_unpin_from_cache(Page* page) {
    page_unpin(page);
}

void page_mark_dirty_page(Page* page) {
    page_mark_dirty(page);
}