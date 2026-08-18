# pre-commit hook

Put this into `.git/hooks/pre-commit`. It:
 - Prevents commits directly to `master` branch
 - Prevents committing changes to any of the `CaravelNet/` directories.

```shell
#!/bin/sh

check_branch() {
    protected="$1"
    branch=$(git symbolic-ref --short HEAD 2>/dev/null)
    if [ "$branch" = "$protected" ]; then
        echo "ERROR: Direct commits to '${protected}' are not allowed." >&2
        echo "       Please create a feature branch and open a pull request." >&2
        exit 1
    fi
}

check_protected_dirs() {
    staged=$(git diff --cached --name-only)
    for dir in "$@"; do
        dir="${dir%/}"
        matches=$(echo "$staged" | grep -E "^${dir}(/|$)")
        if [ -n "$matches" ]; then
            echo "ERROR: Commits touching '${dir}/' are not allowed." >&2
            echo "       Blocked file(s):" >&2
            echo "$matches" | sed 's/^/         /' >&2
            exit 1
        fi
    done
}

check_branch "master"
check_protected_dirs "CaravelNet" "drodrpg/CaravelNet"
```