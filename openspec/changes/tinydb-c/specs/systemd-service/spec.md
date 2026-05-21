## ADDED Requirements

### Requirement: Systemd service unit
A systemd service unit file SHALL be provided for the database server.

#### Scenario: Service installed
- **WHEN** the tinydb package is installed
- **THEN** the systemd unit file SHALL be placed in `/etc/systemd/system/tinydb.service`

### Requirement: Service management
The service SHALL be manageable via systemctl commands:
- `systemctl start tinydb` - Start the server
- `systemctl stop tinydb` - Stop the server
- `systemctl restart tinydb` - Restart the server
- `systemctl status tinydb` - Show status

#### Scenario: Start command
- **WHEN** `systemctl start tinydb` is executed
- **THEN** the server daemon SHALL start
- **AND** become ready to accept connections

### Requirement: Socket activation
The service SHOULD support socket activation for on-demand startup.

#### Scenario: Client connects
- **WHEN** a client connects to the socket
- **THEN** systemd SHALL start the server if not running

### Requirement: Runtime directory
The service SHALL ensure `/run/tinydb/` exists with proper permissions.

#### Scenario: Service starts
- **WHEN** the service starts
- **THEN** it SHALL create `/run/tinydb/` directory if it doesn't exist
- **AND** set ownership to the tinydb user

### Requirement: Data directory
The service SHALL ensure `/var/lib/tinydb/` exists with proper permissions.

#### Scenario: Service starts
- **WHEN** the service starts
- **THEN** it SHALL create `/var/lib/tinydb/` directory if it doesn't exist
- **AND** set ownership to the tinydb user