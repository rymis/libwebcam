#!/bin/sh

cat "$1" | while read line; do
	echo "$line" | sed 's/"/\\"/g;s/^/"/g;s/$/"/g'
done

