#!/bin/sh

## script to initialize a cloned repository
## with per (local) repository settings.

# - ignore quilt's .pc/ directory
# - enable the "--follow-tags" mode for pushing

echo "tuning git-repository for ${NAME}"
git config push.followTags true && echo "enabled push.followTags"

GITEXCLUDE=".git/info/exclude"
egrep "^/?\.pc/?$" "${GITEXCLUDE}" >/dev/null 2>&1 \
  || (echo "/.pc/" >> "${GITEXCLUDE}" && echo "ignoring /.pc/")
