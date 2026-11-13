# lamss toolbox

*This page written by Claude*

`lamss toolbox` (`lm tb`) chooses which toolboxes are built. A toolbox is **active** when it is listed in `.lamss/toolboxes`; `core` is always active.

```sh
lm tb list                       # every toolbox: kind, active, path, git describe
lm tb activate sonar             # sonar and what it requires (autonomy)
lm tb deactivate sonar
lm tb add https://github.com/<owner>/<repo>.git   # external toolbox, as a git submodule
lm tb remove <name>              # deactivate and remove the submodule
```

- **In-tree toolboxes** are directories of this repository: `core`, `autonomy`, `sonar`, `virtual_ocean`, `gui` and `mit`.
- **Submodule toolboxes** are separate git repositories added with `lamss toolbox add`; they are active once added.
- **Requirements** come from each toolbox's `lamss_toolbox(NAME … REQUIRES …)` call in its `CMakeLists.txt`. Activating a toolbox activates what it requires; deactivating one that another active toolbox requires is refused unless `--force`, which deactivates both.
- `.lamss/toolboxes` is plain text, one name per line, and can be edited by hand.

Activating or deactivating changes the next `lamss build` and `lamss dependencies install`; for a submodule toolbox, it also decides whether `lamss docs` includes its pages.
