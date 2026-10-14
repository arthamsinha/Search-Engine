#!/bin/bash
set -e

# Usage:
#   scripts/fetch_repos.sh                      -> ingest the whole seed list (backend/config/seed_topics.txt), then rebuild the index
#   scripts/fetch_repos.sh <max_per_topic> [seed_file]
#   scripts/fetch_repos.sh --topic <topic> [max] -> legacy single-topic fetch (no reindex)
#
# Assumes built binary is in backend/build/devfinder_cli

CLI=./backend/build/devfinder_cli

if [ "$1" = "--topic" ]; then
    "$CLI" fetch "${2:-cpp}" "${3:-20}"
    exit 0
fi

MAX_PER_TOPIC=${1:-20}
SEED_FILE=${2:-backend/config/seed_topics.txt}

# fetch-all saves each topic's repos as it goes and exits 0 even if it had to stop early
# (rate limit / network), so partial progress is never lost.
"$CLI" fetch-all "$MAX_PER_TOPIC" "$SEED_FILE"

# Fetching alone does not update the live index; rebuild it so new repos are searchable.
"$CLI" index
