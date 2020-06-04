// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package db

var (
	mysql_schemas = map[string]string{
		"users": `
CREATE TABLE users (
	user_id VARBINARY(23) NOT NULL PRIMARY KEY,
	drive_guest_id VARBINARY(40),
	created_by_app_name VARBINARY(30),
	created_by_app_version VARBINARY(30),
	created_by_app_platform VARBINARY(60),
	status BINARY(1) NOT NULL,
	password_hash VARBINARY(128) NOT NULL,
	dob VARBINARY(10), -- gorp doesn't support DATE :-(
	email VARCHAR(100),
	family_name VARCHAR(100),
	gender BINARY(1),
	given_name VARCHAR(100),
	username VARCHAR(12),
	deactivated_username VARCHAR(12),
	email_is_verified BOOL,
	email_failure_code VARCHAR(50),
	email_lang VARCHAR(5),
	password_is_complex BOOL,
	time_created DATETIME NOT NULL,
	time_deactivated DATETIME,
	time_purged DATETIME,
	time_updated DATETIME,
	time_password_updated DATETIME,
	time_email_first_verified DATETIME,
	password_reset VARBINARY(16),
	email_verify VARBINARY(16),
	deactivation_notice_state INT UNSIGNED NOT NULL,
	time_deactivation_state_updated DATETIME,
	deactivation_reason VARBINARY(30),
	purge_reason VARBINARY(30),
	no_autodelete BOOL,

	spare_int1 INT,
	spare_int2 INT,
	spare_int3 INT,
	spare_int4 INT,
	spare_int5 INT,
	spare_int6 INT,
	spare_int7 INT,
	spare_int8 INT,
	spare_int9 INT,
	spare_int10 INT,

	spare_varchar1 VARCHAR(255),
	spare_varchar2 VARCHAR(255),
	spare_varchar3 VARCHAR(255),
	spare_varchar4 VARCHAR(255),
	spare_varchar5 VARCHAR(255),

	spare_varbinary1 VARBINARY(255),
	spare_varbinary2 VARBINARY(255),
	spare_varbinary3 VARBINARY(255),
	spare_varbinary4 VARBINARY(255),
	spare_varbinary5 VARBINARY(255),

	UNIQUE KEY username_idx (username),
	KEY email_idx (email),
	KEY deactivated_username_idx (deactivated_username),
	KEY time_created_idx (time_created),
	KEY time_email_first_verified_idx (time_email_first_verified),
	KEY status_deactivated_idx (status, time_deactivated),
	KEY status_unverified_idx (status, time_email_first_verified, time_created)
) ENGINE=InnoDB DEFAULT CHARACTER SET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
`,
		"blocked_emails": `
CREATE TABLE blocked_emails (
	email VARCHAR(100) NOT NULL PRIMARY KEY,
	time_blocked DATETIME NOT NULL,
	blocked_by VARCHAR(100) NOT NULL,
	reason VARCHAR(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARACTER SET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
`,
		"apikeys": `
CREATE TABLE apikeys (
	apikey_id VARBINARY(23) NOT NULL PRIMARY KEY,
	apikey_token VARBINARY(40) NOT NULL,
	scopes BIGINT UNSIGNED NOT NULL,
	description VARCHAR(100) NOT NULL,
	time_created DATETIME NOT NULL,
	time_expires DATETIME NOT NULL,

	UNIQUE KEY token_idx (apikey_token)
) ENGINE=InnoDB DEFAULT CHARACTER SET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
`,
		"sessions": `
CREATE TABLE sessions (
	session_id VARBINARY(23) NOT NULL PRIMARY KEY,
	remote_id VARBINARY(100) NOT NULL, -- Used for admin sessions; okta id, etc
	user_id VARBINARY(23) NOT NULL,
	scope BIGINT UNSIGNED NOT NULL,
	time_created DATETIME NOT NULL,
	time_expires DATETIME NOT NULL,

	KEY user_idx (user_id),
	KEY remote_id_idx (remote_id),
	KEY time_expires_idx (time_expires)
) ENGINE=InnoDB DEFAULT CHARACTER SET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
`,
	}

	// sqlite doesn't really havea  "varbinary", but it defaults to binary collation
	// map "VARBINARY" to "VARCHAR" in sqlite; having "CHAR" in the name means it will
	// have text affinity, at least.
	// "nocase" collation only works for ascii, but is good enough for most tests
	// See: http://www.sqlite.org/datatype3.html#collation
	sqlite_schemas = map[string]string{
		"users": `
CREATE TABLE users (
	user_id VARCHAR(23) NOT NULL PRIMARY KEY,
	drive_guest_id VARCHAR(40),
	created_by_app_name VARCHAR(30),
	created_by_app_version VARCHAR(30),
	created_by_app_platform VARCHAR(60),
	status BINARY(1) NOT NULL,
	password_hash VARCHAR(128) NOT NULL,
	dob VARCHAR(10),
	email VARCHAR(100) COLLATE NOCASE,
	family_name VARCHAR(100) COLLATE NOCASE,
	gender BINARY(1),
	given_name VARCHAR(100) COLLATE NOCASE,
	username VARCHAR(12) COLLATE NOCASE,
	deactivated_username VARCHAR(12) COLLATE NOCASE,
	email_is_verified BOOL,
	email_failure_code VARCHAR(50),
	email_lang VARCHAR(5),
	password_is_complex BOOL,
	time_created DATETIME NOT NULL,
	time_deactivated DATETIME,
	time_purged DATETIME,
	time_updated DATETIME,
	time_password_updated DATETIME,
	time_email_first_verified DATETIME,
    password_reset VARCHAR(16),
    email_verify VARCHAR(16),
	deactivation_notice_state INT UNSIGNED NOT NULL,
	time_deactivation_state_updated DATETIME,
	deactivation_reason VARBINARY(30),
	purge_reason VARBINARY(30),
  no_autodelete BOOL,

	spare_int1 INT,
	spare_int2 INT,
	spare_int3 INT,
	spare_int4 INT,
	spare_int5 INT,
	spare_int6 INT,
	spare_int7 INT,
	spare_int8 INT,
	spare_int9 INT,
	spare_int10 INT,

	spare_varchar1 VARCHAR(255),
	spare_varchar2 VARCHAR(255),
	spare_varchar3 VARCHAR(255),
	spare_varchar4 VARCHAR(255),
	spare_varchar5 VARCHAR(255),

	spare_varbinary1 VARCHAR(255),
	spare_varbinary2 VARCHAR(255),
	spare_varbinary3 VARCHAR(255),
	spare_varbinary4 VARCHAR(255),
	spare_varbinary5 VARCHAR(255)

);
CREATE UNIQUE INDEX users_username_idx ON users (username);
CREATE INDEX users_email_idx ON users (email);
CREATE INDEX users_deactivated_username_idx ON users (deactivated_username);
CREATE INDEX time_created_idx ON users (time_created);
CREATE INDEX time_email_first_verified_idx ON users (time_email_first_verified);
CREATE INDEX status_deactivated_idx ON users (status, time_deactivated);
CREATE INDEX status_unverified_idx ON users (status, time_email_first_verified, time_created);
`,
		"blocked_emails": `
CREATE TABLE blocked_emails (
	email  email VARCHAR(100) NOT NULL PRIMARY KEY,
	time_blocked DATETIME NOT NULL,
	blocked_by VARCHAR(100) NOT NULL,
	reason VARCHAR(100) NOT NULL
);
`,
		"apikeys": `
CREATE TABLE apikeys (
	apikey_id VARCHAR(23) NOT NULL PRIMARY KEY,
	apikey_token VARCHAR(40) NOT NULL,
	scopes BIGINT UNSIGNED NOT NULL,
	description VARCHAR(100) NOT NULL,
	time_created DATETIME NOT NULL,
	time_expires DATETIME NOT NULL
);
CREATE UNIQUE INDEX apikeys_token_idx ON apikeys (apikey_token);
`,
		"sessions": `
CREATE TABLE sessions (
	session_id VARCHAR(23) NOT NULL PRIMARY KEY,
	remote_id VARCHAR(100) NOT NULL,
	user_id VARCHAR(23) NOT NULL,
	scope BIGINT UNSIGNED NOT NULL,
	time_created DATETIME NOT NULL,
	time_expires DATETIME NOT NULL
);
CREATE INDEX session_user_idx ON sessions (user_id);
CREATE INDEX session_remote_id_idx ON sessions (remote_id);
CREATE INDEX time_expires_idx ON sessions (time_expires);
`,
	}

	dbSchemas = map[string]map[string]string{
		"mysql": mysql_schemas,
		//"mysql":  sqlite_schemas,
		"sqlite": sqlite_schemas,
	}
)
