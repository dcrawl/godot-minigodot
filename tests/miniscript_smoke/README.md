# MiniScript Smoke Test Scaffold

Goal: verify first end-to-end path once MiniScript language registration exists.

## Expected Scenario

1. Load `Smoke.tscn` (Node with `hello.ms` attached).
2. Run headless project.
3. Confirm `_ready` output indicates script executed.

## Pass Criteria

- Scene runs without script-load errors.
- Script `_ready` callback executes and prints expected output.
- Runtime errors (if any) include actionable file/line context.

## Automated Check

From project root:

```bash
scripts/run_miniscript_smoke.sh ./godot
```

## Notes

- This scaffold is intentionally lightweight and should be promoted into automated tests as Phase 3 progresses.
