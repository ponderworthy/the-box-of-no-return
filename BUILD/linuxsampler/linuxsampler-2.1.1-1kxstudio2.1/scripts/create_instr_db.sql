CREATE TABLE instr_dirs (
    dir_id         INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_dir_id  INTEGER DEFAULT 0,
    dir_name       TEXT,
    created        TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    modified       TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    description    TEXT,
    FOREIGN KEY(parent_dir_id) REFERENCES instr_dirs(dir_id),
    UNIQUE (parent_dir_id,dir_name)
);

INSERT INTO instr_dirs (dir_id, parent_dir_id, dir_name) VALUES (0, -2, "/");

CREATE TABLE instruments (
    instr_id        INTEGER PRIMARY KEY AUTOINCREMENT,
    dir_id          INTEGER DEFAULT 0,
    instr_name      TEXT,
    instr_file      TEXT,
    instr_nr        INTEGER,
    format_family   TEXT,
    format_version  TEXT,
    instr_size      INTEGER,
    created         TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    modified        TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    description     TEXT,
    is_drum         INTEGER(1),
    product         TEXT,
    artists         TEXT,
    keywords        TEXT,
    FOREIGN KEY(dir_id) REFERENCES instr_dirs(dir_id),
    UNIQUE (dir_id,instr_name)
);
