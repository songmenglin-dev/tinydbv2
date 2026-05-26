## ADDED Requirements

### Requirement: Execution time measurement
The CLI SHALL measure the elapsed time from when a query is sent to the server until the first response is received, with millisecond precision (minimum).

### Requirement: Execution time display
The execution time SHALL be displayed in the result footer in the format `(X.XX sec)`. If execution time is less than 1ms, display `(0.00 sec)`.

### Requirement: Timing mechanism
The CLI SHALL use `gettimeofday()` or equivalent to measure time with at least millisecond resolution. The timing SHALL start immediately before writing the query and stop when the first byte of response is received.

## REMOVED Requirements

None.