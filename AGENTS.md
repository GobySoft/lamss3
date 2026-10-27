# AGENTS.md

Guidance for AI coding agents (Claude, Codex, Copilot, ...) working in this repository. Where a rule below says `<agent>`, use your own name, e.g. `Claude` or `Codex`.

## Code comments

- Default to no comments. Well-named identifiers and clear code should speak for themselves.
- Only add a comment when the *why* isn't obvious from the code.
- When a comment is warranted, keep it short — a single line beats a paragraph.
- Don't explain *what* the code does, restate the diff, or reference the current task/fix/caller (e.g. "used by X", "added for the Y flow"). That belongs in the commit message or PR description, not the code.
- Don't overly emphasis historical reasons for the change, assume the current state of the code speaks for itself.

## Branches and PR titles

- Name branches `<issue type>/<short name>`, where issue type is one of: bug, feature, minor. 
- Title PRs the same way: `<issue type> / <short description>`.

## Github PR

- When replying to Github PRs, prefix each comment with `# <agent>` so the other users can clearly see it is your text.

## Pull request descriptions

Structure the description so the author's own notes come first, and write only
your own section:

```markdown
# <github username of the person the PR is for>

# <agent>

<your description>
```

- Leave the author's section empty for them to fill in. Never write in it or in
  another agent's section, and never remove text already there.
- When updating a PR description, first pull any changes from GitHub to get changes from the user, and rewrite only your own `# <agent>` section.

## Documentation

- Prefix any documentation you write with `*This section written by <agent>*`: directly below the heading of a new section, or on its own line above text added to an existing section. Small edits can be made without this disclaimer.


