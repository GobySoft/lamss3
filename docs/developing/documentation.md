# Documentation

*This page written by Claude*

This site is built by [`lamss docs`](../tool/docs.md) from Markdown kept next to the code. The plan behind it is [plan 11](../../core/src/doc/plan/claude/11-documentation.md).

## Where pages go

| Page | File |
|---|---|
| site-wide (home, getting started, `lamss` tool, developing) | `docs/` |
| a toolbox's overview | `<toolbox>/src/doc/index.md` (generated when missing) |
| an application | `<toolbox>/src/doc/apps/<app>.md` |
| a library | `<toolbox>/src/doc/libraries/<library>.md` |
| an IvP behavior | `<toolbox>/src/doc/behaviors/<behavior>.md` |
| guides for a toolbox | `<toolbox>/src/doc/guide/` |

New pages under a toolbox appear in the navigation by themselves; site-wide pages are listed in [`docs/nav.yml`](../nav.yml).

## Writing

- Write links relative to the page's place in the repository, as they work on GitHub. Links to source files become GitHub links on the site.
- Mermaid diagrams go in ` ```mermaid ` blocks; prefer a figure where it explains better than text.
- Admonitions (`!!! note`) are available, as in [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/reference/admonitions/).
- Pages written by an agent say so under their heading, as [AGENTS.md](../../AGENTS.md) asks.
- Check with `lamss docs build --strict` before pushing; CI does the same.

## Inferred pages

Most ported code came with no documentation. Its pages are written from the code, by an agent or a developer, using the template [`core/src/doc/templates/inferred-app.md`](../../core/src/doc/templates/inferred-app.md); [pViewMessage](../../core/src/doc/apps/pViewMessage.md) is a worked example.

- The front matter says who inferred the page (`inferred_by`), from which commit (`inferred_from`), and its `status`.
- A banner, "Inferred by &lt;agent&gt;", stays on the page until a maintainer has reviewed it and set `status: reviewed`. Inferred pages are published before review.
- Every statement is tagged: **[code]** read from the code, **[test]** seen in a test or a run, **[guess]** inferred intent. Review turns a confirmed **[guess]** into plain text, or corrects it.
- Open questions on the page become GitHub issues.
