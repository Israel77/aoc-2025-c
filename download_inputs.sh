#!/usr/bin/env bash

YEAR=2025

# Save your session string from the browser in a single line file named session.cookie
# Example: If you find session=abcdef... in your browser cookies on the AoC website,
# the content of session.cookie should be abcdef...
SESSION_COOKIE=$(head -n 1 session.cookie)

while [ $# -gt 0 ]; do
    OUT_NUM=$1
    if [ $1 -lt 10 ]; then
        OUT_NUM="0${OUT_NUM}"
    fi

    curl --cookie "session=$SESSION_COOKIE" "https://adventofcode.com/$YEAR/day/$1/input" > "inputs/day_${OUT_NUM}.txt"
    shift 1
done
