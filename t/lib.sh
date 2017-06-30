# t/lib - common routines for tests

# tdiag - print diagnostic messages to standard error if $DEBUG_TESTS is set
# USAGE: tdiag "a helpful diagnostic message"
tdiag() {
  [[ -n ${DEBUG_TESTS} ]] && echo >&2 $*
}

T_DIAG_FILES=()
T_KILL_PIDS=()
T_RM_PATHS=()

###############################################################################
#
# cleanup - a function to trigger from a trap handler
# USAGE: trap "cleanup" EXIT TERM INT
#
cleanup() {
  tdiag "running cleanup routines"

  tdiag "killing processes started during test"
  for pid in ${T_KILL_PIDS[@]}; do
    if kill -0 ${pid} >/dev/null 2>&1; then
      tdiag ">> killing process ${pid}"
      kill -TERM ${pid}
    fi
  done

  tdiag "cleaning up filesystem paths created during test"
  for path in ${T_RM_PATHS[@]}; do
    if [[ -d "${path}" ]]; then
      tdiag ">> removing ${path}"
      rm -rf ${path}
    fi
  done
}
trap 'cleanup' EXIT TERM QUIT INT

###############################################################################
#
# clean_pid - register a process ID for termination during cleanup()
# USAGE: clean_pid $!
#
clean_pid() {
  local pid=${1?clean_pid(): no PID specified}
  tdiag "registering PID ${pid} to be cleaned up at exit"
  T_KILL_PIDS+=(${pid})
}

###############################################################################
#
# clean_path - register a filesystem path for removal during cleanup()
# USAGE: clean_path "/tmp/dir/for/work"
#
clean_path() {
  local path=${1?clean_path(): no path name specified}
  tdiag "registering path ${path} to be cleaned up at exit"
  T_RM_PATHS+=(${path})
}

###############################################################################
#
# diag_file - register a file to be dumped to stderr if the test fails
# USAGE diag_file "${ROOT}/out.log"
#
diag_file() {
  file=${1?diag_file(): no file name specified}
  T_DIAG_FILES+=(${file})
}

###############################################################################
#
# fail - terminate execution of the current test immediately, printing all
#        diagnostic and troubleshooting aids to standard error.
# USAGE: fail
#
fail() {
  for file in ${T_DIAG_FILES[@]}; do
    echo
    echo "==================[ ${file}"
    cat ${file}
    echo "-----------------------"
    echo
    echo
  done
  exit 1
}

###############################################################################
#
# bail - terminate execution of the current test immediately
# USAGE: bail "the reason to print to standard error"
#
bail() {
  echo >&2 $*
  fail
}

###############################################################################
#
# skip - terminate with exit code 77, to signify that this test cannot or
#        should not be run (i.e. because a piece of software is missing)
# USAGE: skip
#
skip() {
  exit 77
}

###############################################################################
#
# need_command - verify that a given command is installed.
# USAGE: need_command ${name} ...
#
need_command() {
  for cmd in "$@"; do
    [[ -x "$(command -v $cmd)" ]] || bail "${cmd} is not installed."
  done
}


###############################################################################
#
# tmpfs - set up a temporary working directory, and set up cleanup handlers
# USAGE: tmpfs
#
tmpfs() {
  ROOT=$(mktemp -d --tmpdir bolo.test.XXXXXXXXXXX)
  export ROOT
  clean_path ${ROOT}
}

###############################################################################
#
# file_is - check the contents of a file against an expected value
# USAGE: file_is /path/to/actual \
#                /path/to/expected \
#                "message to print on failure"
#
file_is() {
  got_f=$1
  expect_f=$2
  msg=${3:-files should match}

  if ! diff -q ${got_f} ${expect_f} &>/dev/null; then
    echo >&2 "${msg}: FAILED"
    diff -u ${got_f} ${expect_f} | sed >&2 -e 's/^/      /'
    echo >&2
    fail
  fi
  tdiag "ok ${msg}"
}

###############################################################################
#
# string_is - assert that two strings are exactly identical
# USAGE: string_is ${actual} ${expected} \
#                  "message to print on failure"
#
string_is() {
  got=$1
  expect=$2
  msg=${3:-strings should match}

  if [[ "${got}" != "${expect}" ]]; then
    echo >&2 "${msg}: FAILED"
    echo >&2 "  expected: '${expect}'"
    echo >&2 "       got: '${got}'"
    echo >&2
    fail
  fi
  tdiag "ok ${msg}"
}

###############################################################################
#
# string_like - assert that two strings are similar via regex
# USAGE: string_like ${actual} ${expected} \
#                  "message to print on failure"
#
string_like() {
  got=$1
  expect=$2
  msg=${3:-strings should match}

  if [[ ! "${got}" =~ ${expect} ]]; then
    echo >&2 "${msg}: FAILED"
    echo >&2 "  expected: '${expect}'"
    echo >&2 "       got: '${got}'"
    echo >&2
    fail
  fi
  tdiag "ok ${msg}"
}

###############################################################################
#
# string_notlike - assert that two strings do not match via regex
# USAGE: string_unlike ${actual} ${unexpected} \
#                  "message to print on failure"
#
string_notlike() {
  got=$1
  expect=$2
  msg=${3:-strings should not match}

  if [[ "${got}" =~ ${expect} ]]; then
    echo >&2 "${msg}: FAILED"
    echo >&2 "  unexpect: '${expect}'"
    echo >&2 "       got: '${got}'"
    echo >&2
    fail
  fi
  tdiag "ok ${msg}"
}
