# DatzBergamo - Project Guidelines

## Project Overview
DatzBergamo is a DayZ-style survival mod for Arma Reforger built on the Enfusion engine.

## Paths
- **Mod codebase:** `C:\Users\rober\OneDrive\Documenten\GitHub\DatzBergamo`
- **Arma Reforger base code:** `C:\Users\rober\OneDrive\Documenten\Arma Base Code`

## Code Quality Rules

### Always verify against Arma base code
Before writing or modifying any script, check the Arma base code at `C:\Users\rober\OneDrive\Documenten\Arma Base Code` to:
- Verify class signatures, method parameters, and return types
- Confirm enum values and flag definitions (e.g. `SCR_EArsenalItemType`)
- Understand parent class behavior before overriding
- Ensure our code is compatible with the base engine APIs

### We can override most Arma base code
Our mod can override nearly all base game classes and scripts. Use the `modded` keyword to extend or replace base game behavior when needed. Always check the base implementation first to understand what we're overriding.

### No lazy solutions
- Never use shortcuts, hacks, or "good enough" workarounds
- Always implement proper, clean, maintainable solutions
- Every feature must be multiplayer-performant:
  - Minimize network traffic (avoid unnecessary RPCs)
  - Use server-authoritative logic where it matters
  - Cache expensive operations instead of recalculating per frame/tick
  - Use spatial partitioning (grids) for proximity checks instead of iterating all entities
  - Batch operations where possible
  - Avoid per-frame work in `EOnFrame` unless absolutely necessary; prefer `CallLater` with intervals
- Write code that scales with player count and entity count

### Enforce script specifics
- Switch statements in functions returning non-void need a return statement after the switch block (compiler requirement)
- Use `ref` for managed object references
- Resource GUIDs are 16-character hex strings in `{GUID}path/file.ext` format
- `.meta` files contain resource GUIDs assigned by Workbench
