# lamss docs

*This page written by Claude*

`lamss docs` builds this site from the pages kept next to the code.

```sh
lamss docs serve               # http://127.0.0.1:8000, rebuilt as you edit pages
lamss docs build               # into build/docs/site
lamss docs build --strict      # fail on broken links and other warnings, as CI does
```

- **What it publishes:** `docs/`, the `src/doc/` directory of every in-tree toolbox (active or not), and that of every active submodule toolbox, so a private toolbox's pages appear in your local site.
- **Generated pages:** the [messages](../reference/messages/index.md) (from each toolbox's `.proto` files, with `protoc`), the [DCCL IDs](../reference/dccl-ids.md), the [command reference](../reference/cli.md) (from the built `lamss` tool), and an overview for each toolbox that has no `src/doc/index.md`.
- **mkdocs** and its plugins are installed into `build/docs-venv` from `docs/requirements.txt` on first use, and again when the file changes. This needs `python3-venv` and network access to PyPI. Set `LAMSS_DOCS_NO_VENV=1` to use the current Python's packages instead.
- **Links:** write links as they work on GitHub, relative to the page's place in the repository. On the site, links to other published pages point at those pages, and links to anything else in the repository (source files, `AGENTS.md`) point at GitHub.
- **Publishing:** a GitHub Actions workflow ([`.github/workflows/docs.yml`](../../.github/workflows/docs.yml)) builds the site from `main` and deploys it to GitHub Pages.

`lamss docs stage` only assembles `build/docs/stage`, which helps when a page does not appear where expected. How to write pages: [Documentation](../developing/documentation.md).
