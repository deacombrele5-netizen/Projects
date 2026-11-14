# Runtime test checklist

Only use disposable copies. Compare source and output SHA-256 hashes.

- [ ] Empty file round trip.
- [ ] Small text and binary files.
- [ ] Unicode file names and long paths.
- [ ] Nested folder processing.
- [ ] Wrong password creates no plaintext output.
- [ ] Modified tag, header and ciphertext are rejected.
- [ ] Truncated and oversized files are rejected.
- [ ] Existing output names are never overwritten.
- [ ] Read-only and access-denied files show a clear error.
- [ ] Context menu can be enabled and removed as a standard user.
- [ ] x64 and Win32 launch and operate on supported Windows versions.
- [ ] Confirm original files remain byte-identical after encrypt/decrypt.

CI compilation is not runtime testing. Before release, test the edge cases above and obtain independent cryptographic review.
