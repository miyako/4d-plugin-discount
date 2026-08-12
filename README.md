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
