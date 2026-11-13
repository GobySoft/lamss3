# lamss clean

*This page written by Claude*

`lamss clean` removes build products: the per-toolbox build trees, the top-level CMake files and `build/usr`. It keeps the `lamss` tool itself (`build/usr/bin/lamss` and its bootstrap tree `build/lamss-tool`), so `lm b` still works afterwards.

```sh
lamss clean            # asks first
lamss clean --yes
lamss clean --all      # all of build/, the tool included; then ./init.sh --no-deps
```
