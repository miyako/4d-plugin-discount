# 4d-plugin-discount

![version](https://img.shields.io/badge/version-18%2B-EB8E5F)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-discount)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-discount/total)

Discount converts Markdown text to HTML inside 4D, using the Discount C library (David Loren Parsons' `libmarkdown`, vendored into the plugin) as its parsing/HTML-generation engine. It exposes a single command, `Markdown`, which takes a Markdown source string and returns generated CSS, a table of contents, and HTML as one combined `Text` result. This reference describes the plugin's behavior **after the fixes applied in this review** (independently-selectable option bits, a guaranteed return value on internal error, serialized access to Discount's shared tag table) — if you're working from a previously-built binary rather than a rebuild of the fixed source, some of the caveats below (particularly around the `options` parameter) describe the corrected behavior, not necessarily what's currently installed.

| Command | Returns | Purpose |
|---|---|---|
| [`Markdown`](#markdown) | Longint | Converts Markdown text to HTML (with optional CSS/TOC) |

**Platforms:** macOS, Windows

## Manual versioning and publication

The Actions workflows are independent manual operations:

- **Bump version**: select a branch and `patch`, `minor`, or `major`. It validates
  source metadata, creates one metadata-only commit with the Copilot co-author
  trailer, and pushes only that branch. It never tags, builds, releases, or
  dispatches another workflow.
- **Build**: select a branch using GitHub's built-in selector; there are **zero
  custom inputs**. Every job checks out the selected immutable `github.sha`.
  The source version determines `vX.Y.Z`, `Discount-X.Y.Z.zip`, and
  `Discount-X.Y.Z.dmg`. Build does not bump source metadata.

### Version authority

`Discount.xcodeproj/project.pbxproj` contains six assignments each for
`MARKETING_VERSION` and `CURRENT_PROJECT_VERSION`. The committed Bump header at
`4157b5fd8f443da399cbd7599be415ca9e1fd3b3` explicitly coordinates **both** keys
and Info.plist's `CFBundleShortVersionString`/`CFBundleVersion`; this is the
authority, despite that workflow body's marketing-first implementation.
The earlier header at `a33eda9adbd93ef46d2f37b4bd0caffcea4e0ba5` specified
marketing-first/fallback behavior, but the later explicit coordinated header
supersedes it. Equality of fields alone is not evidence of coordination.

This migration retains the existing `1.0.4` marketing/plist authority, repairing
only the stale source build values (`1.0.1`) and source English display strings
(`2.0`). **It does not increment the version.** The absent optional
`CFBundleVersion` remains absent. Subsequent bumps coordinate existing literals,
preserving supported `$(MARKETING_VERSION)`, `${MARKETING_VERSION}`,
`$(CURRENT_PROJECT_VERSION)`, and `${CURRENT_PROJECT_VERSION}` references.
Build versions may reference marketing, never themselves or unknown variables.
Versions must be strict `X.Y.Z`, with no leading zeroes, suffixes, drift, or
duplicate assignments, including quoted or same-line duplicates.

Only the project, `Info.plist`, and `English.lproj/InfoPlist.strings` are version
inputs. Rewrites preserve BOMs, every CR/LF/CRLF (including mixed endings),
permissions, references, and unrelated bytes. Vendored Discount/library versions,
4D schema/localization counters, and prebuilt `test/Plugins` files are excluded.
In particular, destination English strings remain untouched by migration/Bump.

Bump requires a clean checkout and the selected branch still at the dispatch
SHA. Its mandatory executable pre-push hook checks Git's actual advertised old
SHA and exact destination ref, literal new commit SHA/HEAD, one direct parent,
and exactly one update. It rejects advance, rewind, deletion, and same-result
races; it never force-pushes or follows tags, even with `push.followTags=true`.
A rejected preparation may leave a local commit in the runner, but pushes no
replacement branch or release.

### Native pipeline and publication safety

The original Windows Release/x64 MSBuild v143/SDK 10.0 override is retained;
the project itself still declares v140. Its actual output is
`test/plugins/Discount.bundle/contents/windows64/Discount.4DX`, normalized to
the canonical `test/Plugins/Discount.bundle/Contents/Windows64/Discount.4DX`.
Only that rebuilt file is uploaded and only `winbuild/Discount.4DX` is merged.
Recursive first-match searches are not used.

macOS retains Xcode's Default target configuration, `arm64 x86_64`,
`ONLY_ACTIVE_ARCH=NO`, deployment override `11.0`, and disabled automatic code
signing, followed by the original Developer ID signing/strict verification and
ZIP/DMG packaging. The original project frameworks, library settings, resource
phases, legacy shell phase, and deployment settings remain unchanged.
`InfoPlist.strings` is an actual Resources phase input; `manifest.json` and
`constants.xlf` are actual CopyFiles inputs (`dstSubfolderSpec = 7`).
Freshness cleanup invalidates only the exact rebuilt binary, plist, English
strings, signature, and existing intermediate outputs. It preserves committed
QuickLook/resources/dependencies and Windows content; deletion errors abort.
The rebuilt binary must contain the exact `arm64` and `x86_64` tokens, not
`arm64e`/`x86_64h` substitutes. No new i386 or exact-slice-count restriction is
invented. Rebuilt plist and localized versions are checked against selected
source, including UTF-16 and compiled binary/XML localization.

Both notary submissions must exit successfully **and** report JSON status
`Accepted`; staple must also succeed. Existing signing/notary secret **names**
are unchanged. No secret values belong in this repository.

Publication uses one constant cross-branch concurrency group with cancellation
disabled. Before native work, and again after all build/sign/notary/staple gates,
it checks exact tag absence and authenticated, fully paginated release lists,
including drafts. Authentication/network errors, API 404, malformed responses,
and later-page failures abort, never count as absence.
Publication then performs a create-only `POST git/refs` at the selected SHA,
verifies the remote tag, separately POSTs a **new public, non-prerelease**
release with explicit tag/name/target/body/draft flags (the original empty
release body is preserved), and uploads assets only to that response's new
release ID. It never reuses, patches, deletes, clobbers, rolls back, or retries
partial state.

**Recovery:** inspect the failing run and exact remote tag SHA, draft/public
releases, release ID, and uploaded assets manually. Any tag, release, or earlier
asset created before failure is deliberately retained. Do not rerun expecting
repair or overwrite: an existing tag or release blocks publication, even at the
same SHA. Resolve retained state through a separately authorized manual recovery,
or bump source on the intended branch and publish a new version. A Bump race
requires a fresh dispatch against the current branch; do not force-push.

### Offline checks and pinned prior art

Run `actionlint` and
`PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s .github/tests -v`.
The full suite also runs in a disposable Git/bare-origin copy after a genuine
guarded helper bump. Fixtures derive versions from source, exercise real local
Git races, and run literal workflow blocks with closed-PATH native/PowerShell
stubs only; those workflow tests never build, sign, notarize, run 4D, or call
production publication APIs. The separate ownership tests below compile native
sources directly. PowerShell (`pwsh`) is required to exercise Windows shell tests.

The workflows, helpers, hooks, and tests were inspected at these exact commits,
not moving branch tips, and adapted to Discount's actual paths/counts/policy:

| Repository | Pinned commit |
|---|---|
| [Native Encoding Detection](https://github.com/miyako/4d-plugin-native-encoding-detection/tree/c42ca565857f674bde5ea6ed3435c87375eda4a7) | `c42ca565857f674bde5ea6ed3435c87375eda4a7` |
| [File Information](https://github.com/miyako/4d-plugin-file-information/tree/19848973601acfd3a2b5070230fa40cdc7e4edb2) | `19848973601acfd3a2b5070230fa40cdc7e4edb2` |
| [Common String Algorithm](https://github.com/miyako/4d-plugin-common-string-algorithm/tree/2606a68201866ec9fcbb38b51462d79215f504ef) | `2606a68201866ec9fcbb38b51462d79215f504ef` |
| [Window Button](https://github.com/miyako/4d-plugin-window-button/tree/3f14facb9aaaaff0e22eeb8e07531163680c3a8a) | `3f14facb9aaaaff0e22eeb8e07531163680c3a8a` |
| [RAW Picture Data](https://github.com/miyako/4d-plugin-raw-picture-data/tree/8144537fdc8dcc3d0c053dfe97aff0a8c0c3a6f9) | `8144537fdc8dcc3d0c053dfe97aff0a8c0c3a6f9` |
| [PRNG](https://github.com/miyako/4d-plugin-PRNG/tree/522f62e1e9f388be0f3f226c1d4c39362dad2491) | `522f62e1e9f388be0f3f226c1d4c39362dad2491` |

---

## Native ownership audit

The bounded audit baseline is `19d8776c80e24d9bf52f15682a0965c2318bc852`.
Both source and committed bundle manifests declare only
`Markdown(&T;&T;&L;&L):L`; `PluginMain` dispatches selector 1 to `Markdown`.
Both native projects compile the entry point, `C_TEXT`, `C_LONGINT`, the
bundled SDK, and their listed Discount library sources. `C_TEXT` copies its
borrowed input into a C++ string and writes output with `PA_SetUnistring`.
`ARRAY_TEXT`, `C_BLOB`, and `support/4DPlugin-JSON` are tracked dormant helpers,
not compiled or included by either project's first-party entry/wrapper graph.
Their implementations and headers were included in the inventory; there is
no first-party CPicture or C_POINTER wrapper in this repository. Vendored
Discount, JsonCpp, and SDK internals are excluded from fixes.

The actual bundled `4DPluginAPI.c` was introduced by
`8e827e6b7602f79d7e0d3ff2f1269e7284d09d41`, with SHA-256
`5bac571c5c4cafd280fa7f23d775458a23701287e8735be27d4a2190002f4e3f`.
Its v14 comment and the headers' v13 comments are not verified upstream
release identities. The audit follows the canonical
[4dplugin skill at e6b492f](https://github.com/miyako/skills/blob/e6b492fcb81e5e1a7f7db723594b420e63bf6eb9/4d-skills/skills/4dplugin/SKILL.md).

Verified SDK source contracts:

- `PA_SetStringInArray` disposes the prior element and shallow-transfers the
  supplied descriptor only for Unicode arrays with non-null data, index
  `0..fNbElements`, and a non-null descriptor pointer. Rejections retain caller
  ownership. `PA_GetStringInArray` returns a borrowed descriptor; `ARRAY_TEXT`
  copies those contents and creates independent strings for transferred output.
- `PA_SetStringVariable` shallow-assigns without clearing the previous value.
  The dormant JSON setters now start overwritten string/picture temporaries
  as Undefined, avoiding loss of the SDK-created initial empty allocation.
- `PA_GetObjectProperty` and `PA_GetCollectionElement` explicitly return owning
  variables to clear after use. JSON scalar getters now copy then clear every
  successful result, including type mismatches. `ob_get_c` transfers a successful
  collection owner to its caller, which must call `PA_DisposeCollection`.
  Lookup errors throw `std::runtime_error`; failed outputs are not inspected
  or cleared. A scoped owner also handles C++ exceptions during scalar copying.
- `PA_ReturnString` delegates to the engine-backed content setter
  `PA_SetUnistring`, not the array descriptor-transfer implementation.
  There are no first-party synchronous method callbacks, JSON Parse calls,
  pointer-value getters, or collection-element consumers in this inventory.

On macOS, run:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 .github/tests/test_ownership.py -v
PYTHONDONTWRITEBYTECODE=1 python3 .github/tests/test_ownership.py --baseline 19d8776c80e24d9bf52f15682a0965c2318bc852 -v
```

The second command is an intentional negative control: the same final harness
must fail on initial string/picture owner leaks, owning getter-result leaks,
and unreported failed property lookups. Dispatcher and normal array-contract
tests pass unchanged on that baseline. Tests compile actual entry/helper sources,
the complete unchanged SDK, and the project's actual Discount source list;
only the 4D engine callback is shimmed. They use ASan/UBSan, exact ASCII/BMP/
surrogate-pair/empty fixtures, allocated and null empties, replacement/shrink/
clear/rejections, copied borrowed inputs, independent transferred outputs, and
explicit string/handle/picture/reference balances (macOS LSan is unavailable).
The test-only engine models string identity conversion, property copies/retains,
and content setters; this does not verify a real 4D engine implementation.
One legacy SDK C pointer-to-integer initialization diagnostic is suppressed
without editing the SDK.

The unsigned `Native ownership` CI runs these checks without project build
phases, signing, packaging, uploads, tags, or releases. Existing manual workflows
remain unchanged. Do not run the Xcode target for validation: its legacy shell
phase invokes signing/notarization even outside Deployment.
Normal array tests do not certify the SDK's resize-allocation-failure count
handling; that unrelated SDK/OOM issue is deferred. SDK-only application-path,
picture-data output, and pointer metadata ownership are not guessed.
No actual 4D host or Windows execution is certified by these tests, and dormant
helper fixes do not alter the exported Markdown command or committed binaries.

---

## Requirements & platform notes

- No source-level platform branching exists in the plugin (`4DPlugin-Discount.cpp`/`.h`) — behavior is identical on macOS and Windows.
- `Markdown` takes **two mandatory parameters** (`source`, `result`) and **two optional Longint parameters** (`flags`, `options`) that both default to `0` when omitted. This is inferred by combining two sources: the plugin's parameter-reading code (which tolerates a missing parameter 3/4 without special-casing it) and the plugin's own sample method, which calls `Markdown ($in;$out)` with only two arguments.
- **`flags` and `options` are two unrelated bit sets — don't confuse them.** `flags` selects Discount's own document-processing behavior (link/image/table handling, etc.). `options` is specific to this plugin and only controls two things: which Markdown dialect to parse (standard vs. GitHub-flavoured) and whether HTML5 block tags are recognized. Passing a GitHub-flavour value in `flags`, or a Discount content flag in `options`, does nothing useful.
- The command is declared `threadSafe: true` in `manifest.json`. Discount's own custom-tag table is process-wide global state (not per-call), so the plugin serializes all Discount interaction internally to make that thread-safety claim hold — you don't need to do anything extra on the 4D side, but don't expect true parallel throughput from concurrent calls on multiple 4D processes/workers.
- `Result` is a coarse success/failure signal (`0` = success), not a decodable Discount-specific error code — see [Error handling](#error-handling--troubleshooting).

---

## Markdown

### Syntax

```4d
Markdown ( source ; result ) → error
Markdown ( source ; result ; flags ) → error
Markdown ( source ; result ; flags ; options ) → error
```

| Parameter | Type | Description |
|---|---|---|
| `source` | Text | Markdown source text to convert. Mandatory. |
| `result` | Text | Variable that receives the generated output. Pass an existing variable — the command fills it in. |
| `flags` | Longint | Optional, default `0`. Discount's own document-processing flags (link/image/table/etc. behavior) — see the flags table below. Not the same bits as `options`. |
| `options` | Longint | Optional, default `0`. Plugin-specific bitmask for dialect selection and HTML5 tag support — see the options table below. |
| Result | Longint | `0` on success. Non-zero on failure — see [Error handling](#error-handling--troubleshooting). |

### Description

`result` is the concatenation, in this fixed order, of: any CSS Discount generates, then a table-of-contents block (only meaningful if `flags` includes `MKD_TOC` and the source has headers), then the compiled HTML body. All three are appended to the same `Text` variable — there's no way to retrieve just one of them separately.

**`flags`** is passed straight through to Discount for both document creation and compilation. It's a bitwise-OR of any of the following, defined in the vendored `mkdio.h`:

| Value | Constant | Effect |
|---|---|---|
| `0x00000001` | `MKD_NOLINKS` | Disable link processing; block `<a>` tags. |
| `0x00000002` | `MKD_NOIMAGE` | Disable image processing; block `<img>` tags. |
| `0x00000004` | `MKD_NOPANTS` | Don't run SmartyPants (typographic quote/dash substitution). |
| `0x00000008` | `MKD_NOHTML` | Disallow raw HTML passthrough entirely. |
| `0x00000010` | `MKD_STRICT` | Disable superscript and relaxed-emphasis extensions. |
| `0x00000020` | `MKD_TAGTEXT` | Treat input as text inside an HTML tag — no emphasis, bold, HTML, or `[]` expansion. |
| `0x00000040` | `MKD_NO_EXT` (alias `MKD_NOEXT`) | Disable pseudo-protocol extensions. |
| `0x00000080` | `MKD_CDATA` | Generate `<![CDATA[...]]>` for XML output. |
| `0x00000100` | `MKD_NOSUPERSCRIPT` | Disable `A^B` superscript syntax. |
| `0x00000200` | `MKD_NORELAXED` | Require emphasis markers everywhere (no relaxed placement). |
| `0x00000400` | `MKD_NOTABLES` | Disallow table syntax. |
| `0x00000800` | `MKD_NOSTRIKETHROUGH` | Disable `~~strikethrough~~`. |
| `0x00001000` | `MKD_TOC` | Generate a table-of-contents block. |
| `0x00002000` | `MKD_1_COMPAT` | Compatibility mode with MarkdownTest 1.0. |
| `0x00004000` | `MKD_AUTOLINK` | Auto-link bare `http://...` URLs even without `<>`. |
| `0x00008000` | `MKD_SAFELINK` | Restrict link protocols to a safe allowlist. |
| `0x00010000` | `MKD_NOHEADER` | Don't process header blocks. |
| `0x00020000` | `MKD_TABSTOP` | Expand tabs to 4 spaces. |
| `0x00040000` | `MKD_NODIVQUOTE` | Disallow `>%class%` blocks. |
| `0x00080000` | `MKD_NOALPHALIST` | Disallow alphabetic (`a.`, `b.`) lists. |
| `0x00100000` | `MKD_NODLIST` | Disallow definition lists. |
| `0x00200000` | `MKD_EXTRA_FOOTNOTE` | Enable Markdown Extra–style footnotes. |
| `0x00400000` | `MKD_NOSTYLE` | Don't extract `<style>` blocks. |
| — | `MKD_EMBED` | Convenience combo: `MKD_NOLINKS \| MKD_NOIMAGE \| MKD_TAGTEXT`. |

**`options`** is a separate, plugin-specific bitmask (defined in `4DPlugin-Discount.h`, not `mkdio.h`):

| Value | Constant | Effect |
|---|---|---|
| `1` | `MKD_GITHUB_FLAVOURED` | Parse as GitHub-Flavoured Markdown instead of standard Markdown. |
| `2` | `MKD_WITH_HTML5_TAGS` | Recognize HTML5 block tags (e.g. `<section>`, `<article>`) as raw-HTML block tags, so Discount passes them through instead of escaping/wrapping them. |

Pass `3` to enable both. These two bits are independently selectable as of the fixes applied in this review — in the previously-shipped source, any non-zero `options` value silently turned **both** on together regardless of which single bit was passed.

### Example

From the plugin's own test method (`TEST.4dm`):

```4d
//%attributes = {}
$in:="aaa | bbbb\r\n-----|------\r\nhello|sailor"

$err:=Markdown ($in;$out)
```

Requesting a table of contents and GitHub-flavoured parsing:

```4d
$in:="# Title\n\nSome *text* with a table.\n\n| a | b |\n|---|---|\n| 1 | 2 |"
$err:=Markdown ($in;$out;MKD_TOC)
```

Combining a Discount flag with both plugin options:

```4d
$in:="# Title\n\n<section>Raw HTML5 block</section>"
$err:=Markdown ($in;$out;MKD_SAFELINK;3)  // options=3 → GitHub-flavoured + HTML5 tags
If ($err=0)
	 // $out now holds any CSS, a TOC, and the HTML body, concatenated
Else
	 ALERT("Markdown conversion failed ("+String($err)+")")
End if
```

---

## Error handling & troubleshooting

- **A non-zero `Result` means the source couldn't be turned into a document at all.** The only failure this plugin's own code can currently produce comes from Discount's document-creation step returning nothing usable for the given `source`/`flags`. The exact conditions that trigger this live inside Discount's own string-parsing code, which wasn't part of this review's source set — treat any non-zero result as a generic "could not process source" signal rather than a specific, decodable error family.
- **`flags` and `options` are separate namespaces.** Passing `MKD_GITHUB_FLAVOURED` (`1`) as `flags` does not select GitHub-flavoured parsing — that constant only means anything in `options`. Double-check which parameter you're setting.
- **A caught internal error returns `-1`.** If something fails unexpectedly during conversion (for example, extremely large `source` text exhausting memory), `Result` comes back `-1` rather than the command silently doing nothing or the host hanging.
- **No built-in size limit on `source`.** Very large or deeply nested Markdown documents can be slow to process or memory-intensive — there's no plugin-side cap, so keep an eye on input size if `source` can come from an untrusted or user-supplied path.
- **`result` accumulates CSS + TOC + HTML in one string.** If your output looks like it has an unexpected leading block before the HTML, that's the generated CSS or TOC, not an error — it's expected when `flags` includes `MKD_TOC` or when the source has headers.

---

## Quick reference

```4d
// Minimal conversion
$err:=Markdown ($source;$html)

// With a table of contents
$err:=Markdown ($source;$html;MKD_TOC)

// GitHub-flavoured Markdown with HTML5 tag passthrough
$err:=Markdown ($source;$html;0;MKD_GITHUB_FLAVOURED+MKD_WITH_HTML5_TAGS)

If ($err#0)
	 ALERT("Markdown conversion failed: "+String($err))
End if
```
