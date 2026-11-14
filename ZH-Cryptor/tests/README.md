# Runtime test checklist

Only use disposable copies. Record SHA-256 before and after; never test against the only copy of important data.

## Crypto and file I/O
- [ ] Empty file round trip (zero-byte chunk authentication).
- [ ] 1-byte, small text, binary and multi-gigabyte files.
- [ ] Files whose sizes are exact multiples of 4 MiB and one byte over.
- [ ] Unicode names, spaces, long paths and files on removable drives.
- [ ] One-character password is accepted; hidden mode asks for confirmation; visible mode asks once.
- [ ] Wrong password creates no final plaintext output.
- [ ] Modified header, nonce, tag, ciphertext, truncated block and trailing bytes are rejected.
- [ ] Existing output names are never overwritten.
- [ ] Disk-full, access-denied, read-only and interrupted-write behavior.
- [ ] Verify source bytes remain identical after encryption and decryption.

## Folders and Explorer
- [ ] Empty folders, nested folders and mixed encrypted/plain files.
- [ ] Folder mode skips its own desktop.ini, .zhcryptor.ico and temporary files.
- [ ] Gold icon appears on .zhc files after association is registered.
- [ ] Folder icon appears for every directory containing encrypted descendants.
- [ ] Existing custom desktop.ini is preserved.
- [ ] Folder icon is removed only after no .zhc files remain below that folder.
- [ ] Context menu encrypt/decrypt passes paths with spaces and Unicode correctly.
- [ ] Enable and remove menu as a standard user; verify registry cleanup.

## Platform and UI
- [ ] x64 and Win32 binaries launch on supported Windows 10/11 systems.
- [ ] Russian/English selector updates every visible label.
- [ ] Light/dark theme trackbar changes the full window appearance.
- [ ] No UI freeze on large files; progress and cancellation behavior.
- [ ] Verify screen reader/keyboard navigation and DPI scaling.

CI compilation is not runtime testing. Do not release until this matrix passes and the cryptographic format has received independent review.
