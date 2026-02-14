#!/bin/bash

# /-----------------------------------------------------------------------\
# |                                                                       |
# |                              H E L L O !                              |
# |                                                                       |
# | This is a simple script that should be helpful building DROD on Linux |
# | Just run this script to see the list of options.                      |
# \-----------------------------------------------------------------------/


# Save the initial arg count so we can detect "no arguments provided"
INITIAL_ARGS=$#

# Help text extracted to its own function
show_help() {
        cat <<'EOF'
Usage: $0 ACTION [OPTIONS]

Actions:
    all               Run docker-up, build-deps, build-drod, build-drod-rpg (in that order)
    docker-up         Start the docker compose container (detached)
    docker-down       Stop and remove the docker compose containers
    docker-bash       Open bash in the docker container
    build-deps        Build project dependencies inside the docker container
    build-drod        Build DROD (Master/Linux) inside the docker container
    build-drod-rpg    Build DROD RPG (drodrpg/Master/Linux) inside the docker container
    run-tests         Build & run DROD tests

Options:
    -r, --root        Use root user where applicable
    --release         Use release builds where applicable (defaults to debug)
    -f, --force       Force rebuild all libraries even if they exist (passed to build-deps)
    -h, --help        Show this help message
EOF
}

# If the script was invoked with no arguments, show help and exit
if [[ $INITIAL_ARGS -eq 0 ]]; then
    show_help
    exit 0
fi

# If the first argument is a help flag, show help
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    show_help
    exit 0
fi

# The first argument is the action; remaining arguments are for that action
ACTION="$1"
shift

BASH_USER=1000
BUILD=debug

# Parse command line arguments for the action
while [[ $# -gt 0 ]]; do
    case $1 in
        -f|--force)
            FORCE_REBUILD=true
            shift
            ;;
        -r|--root)
            BASH_USER=0
            shift
            ;;
        --release)
            BUILD=release
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Unknown option for action '$ACTION': $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

# Directory of this script (so docker-compose.yml is referenced correctly)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Helper: run a command in the drod-linux-builder container
# Uses docker compose from the `Scripts/Linux` directory
docker_exec() {
    local cmd="$*"
    (cd "$SCRIPT_DIR" && docker compose exec --user $BASH_USER -T drod-linux-builder bash -lc "$cmd")
}

# Action implementations
action_all() {
    action_docker_up || return $?
    action_build_deps || return $?
    action_build_drod || return $?
    action_build_drod_rpg || return $?
}

action_docker_up() {
    echo "=> docker-up: starting container (detached)"
    (cd "$SCRIPT_DIR" && docker compose up -d)
}

action_docker_down() {
    echo "=> docker-down: stopping containers"
    (cd "$SCRIPT_DIR" && docker compose down)
}

action_docker_bash() {
    echo "=> docker-bash: entering container"
    (cd "$SCRIPT_DIR" && docker compose exec --user $BASH_USER drod-linux-builder bash)
}

action_build_deps() {
    echo "=> build-deps: running build-deps-linux.sh inside container"
    docker_exec "cd /drod/Scripts && ./build-deps-linux.sh"
}

action_build_drod() {
    echo "=> build-drod: running ninjamaker and build in /drod/Master/Linux"
    docker_exec "cd /drod/Master/Linux && ./ninjamaker -64 -release && ./build"
}

action_build_drod_rpg() {
    echo "=> build-drod-rpg: running ninjamaker and build in /drod/drodrpg/Master/Linux"
    docker_exec "cd /drod/drodrpg/Master/Linux && ./ninjamaker -64 -release && ./build"
}

action_run_tests() {
    echo "=> run-tests: running DROD tests"
    # Only run ninjamaker when it's not already built
    docker_exec "cd /drod/Master/Linux && [ ! -f 'build.custom.$BUILD.x86_64.ninja' ] && ./ninjamaker -64 -$BUILD"
    # Build DRODLibTests
    docker_exec "cd /drod/Master/Linux && ninja -f build.custom.$BUILD.x86_64.ninja builds/custom.$BUILD.x86_64/drod_tests"
    # Run the tests
    docker_exec "cd /drod/Master/Linux && builds/custom.$BUILD.x86_64/drod_tests"
}

# Dispatch the selected action
case "$ACTION" in
    all)
        action_all
        ;;
    docker-up)
        action_docker_up
        ;;
    docker-down)
        action_docker_down
        ;;
    docker-bash)
        action_docker_bash
        ;;
    build-deps)
        action_build_deps
        ;;
    build-drod)
        action_build_drod
        ;;
    build-drod-rpg)
        action_build_drod_rpg
        ;;
    run-tests)
        action_run_tests
        ;;
    *)
        echo "Unknown action: $ACTION"
        show_help
        exit 1
        ;;
esac
