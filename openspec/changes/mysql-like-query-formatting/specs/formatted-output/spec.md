## ADDED Requirements

### Requirement: Formatted table output
The CLI SHALL display query results in a MySQL-compatible box format using ASCII box-drawing characters. Each result set SHALL be formatted as a table with borders, aligned columns, a header row, and a footer with row count and execution time.

### Requirement: Box border structure
The CLI SHALL use `+` for corner and junction characters, `-` for horizontal borders, and `|` for vertical column separators. Example:
```
+------------+------------+------------+
| column1    | column2     | column3    |
+------------+------------+------------+
| value1     | value2      | value3     |
+------------+------------+------------+
```

### Requirement: Column width calculation
The CLI SHALL calculate column width as the maximum of (longest value in column, header length) plus 1 space of padding on each side. All rows in a column SHALL be left-aligned within the column width.

### Requirement: Header row formatting
The CLI SHALL display column names in a header row separated from data rows by a line of `-` characters with `+` junctions, matching the column widths calculated.

### Requirement: Result footer
The CLI SHALL display a footer line after all data rows containing the row count and execution time in the format: `N rows in set (X.XX sec)`.

## REMOVED Requirements

None.