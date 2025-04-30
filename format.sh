#!/usr/bin/env sh

cd $(dirname $0)

REQUIRED_VERSION="17"

if [ -x "$(command -v clang-format-$REQUIRED_VERSION)" ]; then
  CLANG_FORMAT=clang-format-$REQUIRED_VERSION
elif [ -x "$(command -v clang-format)" ]; then
  VERSION=$(clang-format --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
  MAJOR_VERSION=$(echo "$VERSION" | cut -d "." -f 1)
  if [ "$MAJOR_VERSION" = "$REQUIRED_VERSION" ]; then
    CLANG_FORMAT=clang-format
  else
    echo "Error: clang-format version $REQUIRED_VERSION is required, but found version $VERSION."
    exit 1
  fi
fi

# if clang-format not found, exit
if [ -z "$CLANG_FORMAT" ]; then
  # bold red error message
  printf '\033[1;31mError:\033[0m clang-format not found\n' >&2
  exit 1
fi


# set find command according to OS
if [ "$(uname)" = "Darwin" ]; then
  alias find_command='/usr/bin/find -E . -iregex ".*\.(cpp|hpp|cc|cxx|ipp|h)"'
else
  alias find_command='/usr/bin/find . -regex ".*\.\(cpp\|hpp\|cc\|cxx\|ipp\|h\)"'
fi

if [ $# -ge 1 ] && [ $1 = "--check" ]; then
  find_command \
    -not -path './.git/*' \
    -not -path './.gitlab/*' \
    -not -path './.cache/*' \
    -not -path './.venv/*' \
    -not -path './venv/*' \
    -not -path './node_modules/*' \
    -not -path './external/*' \
    -not -path './doxygen/*' \
    -not -path './build/*' |
    xargs -n 1 -P8 $CLANG_FORMAT --dry-run -Werror
else
  find_command \
    -not -path './.git/*' \
    -not -path './.gitlab/*' \
    -not -path './.cache/*' \
    -not -path './.venv/*' \
    -not -path './venv/*' \
    -not -path './node_modules/*' \
    -not -path './external/*' \
    -not -path './doxygen/*' \
    -not -path './build/*' |
    xargs -n 1 -P8 $CLANG_FORMAT -i
fi
