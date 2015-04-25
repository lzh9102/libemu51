#!/bin/sh
# generate coverage statistics using lcov

if [ $# -ne 1 ]; then
	echo "usage: $0 <builddir>"
	exit 1
fi

# check lcov
if ! which lcov 2>&1 > /dev/null; then
	echo "error: lcov not found"
	exit 1
fi

# run unit tests
make test || exit 1

# generate coverage statistics
mkdir -p "$1/coverage" && cd "$1/coverage"
lcov --capture --directory "$1" --output-file "coverage.info" \
	--rc lcov_branch_coverage=1
genhtml --rc lcov_branch_coverage=1 "coverage.info"
