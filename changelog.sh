#!/usr/bin/sh

# Small shell script to automatically generate release changelogs.

echo "New features:"
git log --pretty=format:%s $1..HEAD | grep "feat:" | sed 's/feat\:/*/g'
echo ""

echo "Fixes:"
git log --pretty=format:%s $1..HEAD | grep "fix:" | sed 's/fix\:/*/g'