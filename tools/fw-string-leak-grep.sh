#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# fw-string-leak-grep.sh — FreeWorks "SolidWorks"-string leak guard (SHELL-02 /
# D-03, threat T-01-11).
#
# Greps the new FreeWorks tree (src/Gui/FreeWorks/) for the trademark token
# "SolidWorks" and fails (exit non-zero) on ANY occurrence that is not the single
# allow-listed upstream type-name reference:
#
#     Gui::SolidWorksNavigationStyle
#
# Rationale (D-03): The ONLY legitimate "SolidWorks" identifier in new FreeWorks
# code is the upstream NavigationStyle *type name* — the configuration value the
# fork writes so FreeCAD selects the existing upstream navigation style. Every
# other "SolidWorks" token — an identifier such as `SolidWorksWorkbench`, a path
# component, or a user-facing label/string — is a trademark leak and must fail.
#
# Allow-list scope: EXACTLY the substring "Gui::SolidWorksNavigationStyle". An
# occurrence is allowed only when, after removing every instance of that exact
# substring from the line, no bare "SolidWorks" token remains. This means:
#   - `Gui::SolidWorksNavigationStyle`            -> allowed
#   - `"Gui::SolidWorksNavigationStyle"` (string) -> allowed (it is the type name)
#   - `SolidWorksWorkbench`                        -> FAILS
#   - a "SolidWorks" user-facing label             -> FAILS
#   - `SolidWorksNavigationStyle` without the `Gui::` qualifier -> FAILS
#
# Comment convention (D-03): internal `//`-comments may describe SolidWorks
# *behavior*. To keep the grep simple and unambiguous, this guard does NOT try to
# parse C++ to distinguish comments from code; instead the project convention
# (established in 01-01-SUMMARY) is that new FreeWorks SOURCE carries no bare
# "SolidWorks" word at all — behavior is described as "reference-CAD-style".
# Therefore any bare "SolidWorks" token (comment or not) is reported. Doc/Markdown
# files under the tree are scanned the same way; they reference the allow-listed
# type name in backticks, which passes.
#
# Usage:
#   tools/fw-string-leak-grep.sh [SCAN_DIR]
#     SCAN_DIR defaults to src/Gui/FreeWorks/ (repo-relative).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

SCAN_DIR="${1:-${REPO_ROOT}/src/Gui/FreeWorks}"

if [[ ! -d "${SCAN_DIR}" ]]; then
    echo "fw-string-leak-grep: FATAL: scan dir does not exist: ${SCAN_DIR}" >&2
    exit 2
fi

# The single allow-listed token (kept verbatim so this file itself is greppable
# and self-documenting: Gui::SolidWorksNavigationStyle).
readonly ALLOWED='Gui::SolidWorksNavigationStyle'

leaks=0

# Enumerate every "SolidWorks" hit with file:line, deterministically. We exclude
# binary files (-I) so image bytes can never produce a false positive.
while IFS= read -r hit; do
    [[ -z "${hit}" ]] && continue

    # hit looks like: <file>:<lineno>:<content>
    file="${hit%%:*}"
    rest="${hit#*:}"
    lineno="${rest%%:*}"
    content="${rest#*:}"

    # Remove every occurrence of the exact allow-listed token, then see whether
    # any bare "SolidWorks" word still remains on the line.
    stripped="${content//${ALLOWED}/}"

    if printf '%s' "${stripped}" | grep -q "SolidWorks"; then
        rel="${file#"${REPO_ROOT}/"}"
        echo "fw-string-leak-grep: LEAK ${rel}:${lineno}: ${content}" >&2
        leaks=$((leaks + 1))
    fi
done < <(grep -rnI "SolidWorks" "${SCAN_DIR}" || true)

if [[ ${leaks} -gt 0 ]]; then
    echo "fw-string-leak-grep: FAIL — ${leaks} disallowed 'SolidWorks' token(s) under ${SCAN_DIR}." >&2
    echo "  Only '${ALLOWED}' is allow-listed (D-03, threat T-01-11)." >&2
    exit 1
fi

echo "fw-string-leak-grep: OK — no disallowed 'SolidWorks' tokens; only '${ALLOWED}' present."
exit 0
