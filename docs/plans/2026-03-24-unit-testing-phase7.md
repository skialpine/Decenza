# Phase 7: Database Migration Chain

> **For Claude:** Create in-memory SQLite databases, run migration chain, verify schema and data integrity. Uses `withTempDb()` from dbutils.h for connection management.

**Rationale:** ShotHistoryStorage migrations had 4+ rounds of cascading fixes (64% fix rate). Migration 7 alone needed 4 fix rounds. Testing the full chain catches data loss, schema drift, and FTS breakage.

## Test File: `tests/tst_dbmigration.cpp`

**Source deps:** CORE_SOURCES + history sources. Needs `Qt6::Sql`.

**Approach:** Create a fresh in-memory `:memory:` SQLite database, create the initial schema (v0), then run each migration step and verify the result. Uses friend class access to call migration methods directly.

### Test Cases (~20)

**Schema creation:**
- [ ] Fresh DB: schema_version table created with version 0
- [ ] Initial tables: `shots`, `shot_samples`, `shot_phases` exist

**Migration v3 (columns):**
- [ ] After v3: `temperature_override` column exists in shots
- [ ] After v3: `yield_override` column exists in shots

**Migration v4 (transition_reason):**
- [ ] After v4: `transition_reason` column exists in shot_phases

**Migration v5 (FTS rebuild):**
- [ ] After v5: FTS table `shots_fts` exists and is searchable
- [ ] Insert a shot, verify FTS finds it by profile_name

**Migration v6 (beverage_type):**
- [ ] After v6: `beverage_type` column exists in shots
- [ ] Backfill: insert shot with profile_json containing beverage_type, verify extracted

**Migration v7 (weight smoothing):**
- [ ] After v7: version bumped to 7
- [ ] Insert shot_samples with jagged weight data, verify smoothed after migration

**Migration v8 (grinder split):**
- [ ] After v8: `grinder_brand` and `grinder_burrs` columns exist
- [ ] Backfill: insert shot with grinder_model="Niche Zero", verify brand populated
- [ ] FTS rebuilt: search by grinder_brand works

**Migration v9 (knowledge base ID):**
- [ ] After v9: `profile_kb_id` column exists with index
- [ ] schema_version = 9

**Idempotency:**
- [ ] Run full migration chain twice → no crash, version still 9
- [ ] Run on already-current DB → no-op

**Edge cases:**
- [ ] Migration on empty DB (no shots) → succeeds
- [ ] Migration with NULL values in optional columns → no crash
