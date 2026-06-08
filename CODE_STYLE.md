# Code Style

## C++ (backend)

**Formatter:** `clang-format` v19+ using the `.clang-format` file at the project root.

Key rules:
- 4-space indentation, no tabs, 120-column limit.
- Allman braces for functions and classes; braces attached for control flow.
- `PascalCase` for types and public methods; `_camelCase` for private members.
- `SCREAMING_SNAKE_CASE` for `static constexpr` constants.
- No magic numbers — extract to named constants.
- Early returns to reduce nesting.
- Comments explain *why*, not *what*; one line maximum for inline comments.
- Every `.cpp` file must include `PCH.h` first.

Run the formatter:
```bash
# Check (CI-equivalent)
clang-format --style=file --dry-run --Werror backend/src/*.cpp backend/src/*.h

# Fix in place
clang-format --style=file -i backend/src/*.cpp backend/src/*.h
```

Treat all compiler warnings as errors (`/WX` in MSVC). Warnings must be resolved before committing.

## TypeScript / Vue (frontend)

**Formatter:** `prettier` v3+ using `frontend/.prettierrc`.

Key rules:
- 2-space indentation, no semicolons, single quotes, trailing commas in ES5 positions.
- 100-column print width.
- `PascalCase` for Vue components and TypeScript interfaces/types.
- `camelCase` for variables, functions, and Pinia store actions.
- Prefer `const` over `let`; never use `var`.
- No `any` — use explicit types or `unknown` with a type guard.
- Vue `<script setup lang="ts">` for all new components.

Run the formatter:
```bash
cd frontend

# Check
npm run lint

# Fix in place
npm run lint:fix
```

## JSON Schema

- All schemas use JSON Schema draft 2020-12 (`"$schema": "https://json-schema.org/draft/2020-12/schema"`).
- `"additionalProperties": false` on every object.
- All required fields listed in `"required"`.
- Schemas live in `schema/` and are imported by both frontend AJV validators and backend tests.

## Pre-commit hooks

Install once per clone:
```bash
pip install pre-commit
pre-commit install
```

Hooks run automatically on `git commit`. To run manually against all files:
```bash
pre-commit run --all-files
```

The hooks enforce:
- `clang-format` on all C/C++ files.
- `prettier` on all TypeScript and Vue files under `frontend/`.
- Trailing whitespace, end-of-file newlines, YAML/JSON validity, LF line endings.
