# Porting old code

*This page written by Claude*

Code comes over from `lamss`, `lamss-shared` and `lamss-internal` with its git history, so `git log --follow` and `git blame` still show who wrote what.

1. **Extract the history:** in a scratch clone of the old repository, `git filter-repo --path src/moos/<app> --path-rename src/moos/<app>:<toolbox>/src/bin/<app>`.
2. **Scan it for secrets** (passwords, keys, hostnames) before anything is pushed. Stop and ask if you find one; such history is rewritten or the code is not imported.
3. **Merge it in:** `git fetch <scratch clone>` and `git merge --allow-unrelated-histories`.
4. **Format it** with `clang-format-18 -i`, in a commit that changes nothing else, and add that commit's hash to [`.git-blame-ignore-revs`](../../.git-blame-ignore-revs).
5. **Make it build:** SPDX and copyright headers, `CMakeLists.txt` with the [CMake helpers](build-system.md#cmake-helpers), proto packages renamed to `lamss.<toolbox>`, removal of what lamss3 does not carry (ICEX-only paths go to the `icex` toolbox). Each change is a commit of its own.
6. **Write its page:** an inferred page from the template, see [Documentation](documentation.md#inferred-pages). A port is not finished without it.
7. **Add a test** that at least starts it, where it can run without hardware.

Stacks of related ports are merged, never rebased, so the formatting commits keep the hashes listed in `.git-blame-ignore-revs`. Which app goes to which toolbox is in [plan 01](../../core/src/doc/plan/claude/01-source-inventory.md).
