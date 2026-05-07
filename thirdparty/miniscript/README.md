# MiniScript Third-Party Source

Vendored MiniScript C++ interpreter source for the MiniGodot integration.

## Version

- **Upstream repository:** https://github.com/JoeStrout/miniscript (MiniScript-cpp)
- **Pinned commit:** `18eced65c6a7a2c744e327f07eac9cf907f3fa2d`
- **Tag at pin:** `v1.6.2-65-g18eced6` (65 commits past v1.6.2)
- **Files vendored from:** `MiniScript-cpp/src/MiniScript/`

## License

MiniScript is MIT-licensed. The full license text is in `LICENSE` (this directory).
Copyright (c) 2019 JoeStrout.

## Update Procedure

1. Check out the upstream MiniScript repository.
2. Copy updated files from `MiniScript-cpp/src/MiniScript/` into this directory.
3. Update the pinned commit and tag above.
4. Build the editor and run `scripts/run_miniscript_smoke.sh` to verify compatibility.
5. Run the full Gate A runtime suite: `scripts/run_miniscript_runtime_suite.sh`
6. Record any breaking API changes and update the MiniGodot adapter layer as needed.

## Files

Core interpreter (required):
- `SimpleString.h/cpp` — string class
- `SimpleVector.h/cpp` — dynamic array
- `RefCountedStorage.h` — reference-counted allocation base
- `List.h/cpp` — linked list
- `Dictionary.h/cpp` — hash map
- `MiniscriptErrors.h` — exception types
- `MiniscriptKeywords.h/cpp` — keyword table
- `MiniscriptTypes.h/cpp` — `Value`, `ValueList`, `ValueDict`, `FunctionStorage`
- `MiniscriptLexer.h/cpp` — lexer
- `MiniscriptParser.h/cpp` — parser → TAC compiler
- `MiniscriptTAC.h/cpp` — three-address code VM
- `MiniscriptInterpreter.h/cpp` — high-level interpreter wrapper
- `MiniscriptIntrinsics.h/cpp` — standard intrinsic functions
- `UnicodeUtil.h/cpp` — Unicode helpers
- `SplitJoin.h/cpp` — string split/join utilities
- `QA.h/cpp` — assertion utilities (used by intrinsics)
- `UnitTest.h/cpp` — unit test runner (used by intrinsics)
