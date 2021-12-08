CREATE TABLE "user" (
    "uid" INTEGER UNIQUE,
    "username" TEXT NOT NULL UNIQUE,
    "password" TEXT NOT NULL,
    "salt" TEXT NOT NULL,
    PRIMARY KEY("uid" AUTOINCREMENT)
);
CREATE TABLE "message" (
    "id" INTEGER NOT NULL UNIQUE,
    "from_user" INTEGER NOT NULL,
    "message" TEXT NOT NULL,
    "is_dm" INTEGER DEFAULT 1,
    "to_user" INTEGER,
    "created_at" TEXT,
    PRIMARY KEY("id" AUTOINCREMENT)
);