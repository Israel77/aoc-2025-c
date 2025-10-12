#!/usr/bin/env bash

for exe in build/utils/tests/*; do
    if [[ -x "$exe" && "$exe" == *_dbg ]]; then
        ./"$exe"
    fi
done
