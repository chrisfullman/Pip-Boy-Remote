# Contributing to Pip-Boy Remote

## Getting started

1. Fork or clone the repository.
2. Install the pre-commit hooks (requires Python and the `pre-commit` package):
   ```bash
   pip install pre-commit
   pre-commit install
   ```
3. Install frontend dependencies:
   ```bash
   cd frontend && npm install
   ```
4. Download map assets before running the dev server:
   ```bash
   bash scripts/download-assets.sh
   ```

## Branch naming

| Purpose | Pattern | Example |
|---------|---------|---------|
| New feature | `feat/<short-description>` | `feat/map-waypoint-ui` |
| Bug fix | `fix/<short-description>` | `fix/inventory-weight-overflow` |
| Documentation | `docs/<short-description>` | `docs/packaging-guide` |
| Refactor | `refactor/<short-description>` | `refactor/poller-threading` |
| CI / tooling | `ci/<short-description>` | `ci/frontend-build-step` |

Branch off `main`; keep branches focused on a single concern.

## Commit messages

Use [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short imperative summary>

[optional body — explain WHY, not WHAT]

[optional footer — breaking changes, issue refs]
```

Types: `feat`, `fix`, `docs`, `refactor`, `test`, `ci`, `chore`

Scopes (optional but encouraged): `backend`, `frontend`, `schema`, `ci`, `scripts`

Examples:
```
feat(backend): add SampleMapMarkers to GameStatePoller
fix(frontend): correct Y-axis inversion in MapView worldToCanvas
docs: add packaging guide
```

## Pull requests

- Target `main`.
- Keep PRs small and focused; one logical change per PR.
- Fill out the PR description: what changed, why, how to test.
- All CI checks must pass before merge.
- At least one approving review is required for changes to `backend/` or `schema/`.

## Code review

- Review comments are suggestions unless prefixed with **MUST:** or **BLOCK:**.
- Resolve all blocking comments before requesting re-review.
- Reviewers: focus on correctness, thread safety, and schema conformance.
  Style is enforced by the pre-commit hooks — do not leave style comments.

## Running tests

```bash
# Frontend unit tests (schema validation + component tests)
cd frontend && npm test

# Backend unit tests (requires xmake and MSVC on Windows)
cd backend && xmake build PipBoyRemote_Tests && xmake run PipBoyRemote_Tests

# Offline frontend development with the replay harness
node scripts/replay.js --synthetic
```

## Linting

```bash
# C++ — check only (pre-commit also runs this)
clang-format --style=file --dry-run --Werror backend/src/*.cpp backend/src/*.h

# C++ — auto-fix
clang-format --style=file -i backend/src/*.cpp backend/src/*.h

# Frontend — check only
cd frontend && npm run lint

# Frontend — auto-fix
cd frontend && npm run lint:fix
```

See [CODE_STYLE.md](CODE_STYLE.md) for formatting conventions and tool versions.
