# lamss env

*This page written by Claude*

`lamss env` makes your shells use one lamss3 checkout, the **active** one, and switches between checkouts.

| File | Written by | Holds |
|---|---|---|
| `~/.config/lamss/env.sh` | `lamss env switch` | `LAMSS_ROOT`, `LAMSS_PREFIX` (`build/usr`), and `PATH`, `LD_LIBRARY_PATH` and `PYTHONPATH` entries for the active checkout; the `lm` alias |
| `~/.config/lamss/checkouts` | `lamss env switch` | every checkout made active so far |
| block in `~/.bashrc`, `~/.zshrc` | `lamss env hook` | sources `env.sh` in each new interactive shell |

```sh
lamss env switch               # make the checkout you are in active (init.sh does this)
lamss env switch other         # by name or path
lamss env list                 # known checkouts; * marks the active one
eval "$(lamss env show)"       # this checkout's exports, for scripts
lamss env hook --remove        # remove the ~/.bashrc block
```

`env.sh` is plain shell, so a shell starts even when the build is broken. Non-interactive contexts (CI, cron, supervisord) do not read `~/.bashrc`: source `~/.config/lamss/env.sh` or use `lamss env show`. bash, zsh and dash are supported.
