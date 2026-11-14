# Runtime test checklist

Run these tests in disposable directories on each supported Windows version and architecture. Do not use valuable files until all tests pass.

- [ ] Empty file encrypt/decrypt round trip; compare SHA-256 before/after.
- [ ] Small text, binary, Unicode-named file round trips.
- [ ] Folder recursion with nested subfolders.
- [ ] Wrong password leaves encrypted source intact and produces no output.
- [ ] Modified header, modified ciphertext and truncated file are rejected.
- [ ] Existing output-name collision creates a distinct output, never overwrites.
- [ ] Read-only files, access denied and locked files produce clear errors.
- [ ] Maximum-size boundary and over-limit file behavior.
- [ ] Explorer menu install/remove works under a standard user account.
- [ ] x86 and x64 builds start on the supported OS matrix.
- [ ] Verify originals remain byte-identical after both operations.

CI compilation is not a substitute for these runtime tests, fuzzing, or independent cryptographic review.
