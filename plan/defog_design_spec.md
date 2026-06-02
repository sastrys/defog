# Defog (`defog`) - Human-First Code Comprehension Tool

**Design and implementation specification**  
**Version:** 0.1  
**Date:** May 7, 2026  
**Primary audience:** Coding agents such as Codex, plus human reviewers  

---

## 1. Product intent

Build **Defog**, a local developer tool for understanding code that was just written, generated, or modified.

The CLI executable should be named:

```bash
defog
```

Defog's job is to help a human developer quickly answer:

```text
What changed?
Where is the important logic?
How is it connected?
What should I inspect?
Can I generate a useful report with diagrams?
```

The first version should focus on **human comprehension of changed code**, especially code produced during an agentic coding session.

The tool should not start as an agent-context generator. It should first help the human understand the code that an agent or developer produced.

---

## 2. Naming

Use the following naming consistently:

```text
Product name: Defog
CLI command: defog
Project directory: defog/
Local state directory: .dfog/
Internal namespace: defog
```

The phrase **code atlas** may be used internally as a concept, meaning the indexed map of code targets and relationships. It should not be the external product name.

Examples:

```bash
defog update
defog status
defog changed --kind function
defog changed --kind function -q | defog show -v
defog report --changed --open
```

---

## 3. Primary use case

The primary use case is:

> A coding agent changed code. The developer wants to understand the change without manually reading every diff hunk.

The golden path should be:

```bash
defog update
defog status
defog changed --kind function
defog changed --kind function -q | defog show -v
defog report --changed --open
```

A good session should look like this:

```text
$ defog status

Defog status

Index:
  up to date

Git:
  unstaged modified files: 2
  staged files: 0
  untracked files: 1

Changed targets since HEAD:
  modified files: 2
  modified functions: 3
  added tests: 1

Try:
  defog changed
  defog changed --kind function
  defog report --changed --open
```

Then:

```text
$ defog changed --kind function

modified  src/data/transforms.py::build_transforms@12-45
modified  src/data/dataloader.py::create_loaders@31-88
added     tests/test_transforms.py::test_validation_transforms_are_deterministic@8-31
```

Then:

```bash
defog changed --kind function -q | defog show -v
```

Defog should produce structured explanations of each changed function.

Then:

```bash
defog report --changed --open
```

Defog should generate a static local report with summaries, diagrams, and source spans.

Everything else is secondary to this workflow.

---

## 4. Design principles

### 4.1 Optimize for changed-code comprehension

Defog should not try to become a general IDE, compiler, agent framework, database platform, or code intelligence platform.

The MVP should answer:

```text
What changed?
Which files/functions/classes were affected?
What do those changed targets do?
How are they connected?
Can I see diagrams?
Can I inspect exact code spans?
Can I generate a report?
```

### 4.2 Use existing tools instead of reinventing them

Use existing tools wherever they naturally fit:

```text
Use Git for change detection.
Use DOT/Graphviz for diagram layout and SVG rendering.
Use Tree-sitter or another existing parser for structural extraction, where useful.
Use Go's standard testing package for tests.
Use SQLite only if it meaningfully improves simplicity, performance, or reliability.
```

Do not build an independent staging system. Use Git staged, unstaged, untracked, and base-ref concepts.

Do not build a graph layout engine. Generate DOT and let Graphviz render it.

Do not build a compiler. Extract enough structure to create useful code targets, diagrams, and reports.

### 4.3 Keep the implementation simple

The coding agent should prefer simple, testable implementation choices.

Examples:

```text
Prefer a clear file-based index over SQLite if it works well enough.
Prefer deterministic summaries over LLM-generated prose in the MVP.
Prefer approximate call edges with confidence labels over pretending to have compiler-grade resolution.
Prefer shelling out to Git over reimplementing Git behavior.
Prefer shelling out to Graphviz dot over linking to Graphviz libraries.
```

### 4.4 Target realistic repository size

Target codebases around:

```text
~100k lines of code
hundreds to low thousands of files
single local repository
one developer using it locally
```

Do not optimize the MVP for:

```text
Linux-kernel-scale repositories
multi-million-line monorepos
distributed indexing
cloud indexing
multi-user collaboration
cross-repository dependency analysis
```

Performance should be good enough for local daily use, not enterprise-scale indexing.

### 4.5 Prefer explicit composition over hidden session state

Do not use hidden sessions, sticky handles, or implicit current targets in the MVP.

The interface should be based on explicit target sets:

```text
selector command -> target refs -> view/report/graph command
```

Examples:

```bash
defog changed --kind function -q | defog show -v
defog find "augmentation" --kind function -q | defog graph calls -o calls.svg
defog ls src/data --kind function -q | defog report --open
```

### 4.6 Human-readable by default, pipeable when requested

Commands should have three output modes:

```text
default      human-readable table/text
-q           quiet target refs, one per line
--jsonl      machine-readable records, one JSON object per line
```

Example:

```bash
defog changed --kind function
```

prints a readable table.

```bash
defog changed --kind function -q
```

prints only stable target refs.

```bash
defog changed --kind function --jsonl
```

prints JSON Lines.

### 4.7 Avoid overclaiming

Defog is a reading aid, not a compiler.

It may say:

```text
Detected call-like expression: ProcessImage(...)
Confidence: approximate
```

It should not claim:

```text
This definitely resolves to defog::vision::ProcessImage(const Image&)
```

unless it truly knows that.

### 4.8 Red-green TDD

Every feature should be implemented test-first:

```text
1. Write a failing test.
2. Confirm it fails for the intended reason.
3. Implement the smallest code that passes.
4. Refactor while keeping tests green.
```

### 4.9 Implement Defog in Go

Defog should be implemented in Go.

Preferred:

```text
Go 1.22 or newer
Standard Go project layout with packages under internal/ where appropriate
Go standard library first, with small dependencies only when they clearly simplify the tool
go test for unit and integration tests
go test -bench optional for benchmarks
Explicit error returns with useful context
Deterministic output
```

Avoid global mutable state in production logic unless the implementation has a strong reason and documents it.

---

## 5. Non-goals for the MVP

Do not build these in the MVP:

```text
Agent context-packet generation
LLM-based summarization as a required dependency
Interactive web app
VS Code extension
Language Server Protocol implementation
Compiler-grade type resolution
Perfect C++ call graph
Macro expansion engine
Cross-repository analysis
Cloud service
Persistent CLI sessions
Custom graph layout engine
Large-scale monorepo indexing
```

LLM-based summaries may be added later behind an interface, but the MVP should be deterministic and testable without a live model.

---

## 6. Core mental model

Defog has four central concepts:

```text
Target
TargetRef
TargetSet
ChangeSet
```

### 6.1 Target

A **Target** is anything Defog can show, graph, inspect, or report on.

Examples:

```text
repository
directory
file
namespace
class
struct
function
method
test
changed region
branch
call
return statement
```

MVP target kinds:

```text
repo
dir
file
class
struct
function
method
test
region
```

Optional later target kinds:

```text
namespace
branch
loop
call
return
expression
```

### 6.2 TargetRef

A **TargetRef** is a stable, human-readable reference to a target.

Preferred format:

```text
path
path:line
path::symbol
path::qualified_symbol@start-end
```

Examples:

```text
src/data/transforms.py
src/data/transforms.py:22
src/data/transforms.py::build_transforms
src/data/transforms.py::build_transforms@12-45
src/loader.cc::defog::Loader::Load@44-96
```

The exact internal ID can be different, but the CLI should expose compact refs.

Rules:

```text
1. TargetRefs must be copyable.
2. TargetRefs must work as command inputs.
3. TargetRefs must be safe to print one per line.
4. TargetRefs should be stable within a given code state.
5. If a target moves, the old ref may become stale; this is acceptable.
```

### 6.3 TargetSet

A **TargetSet** is a list of targets produced by selector commands and consumed by view commands.

Selector commands:

```bash
defog changed
defog ls
defog find
```

Consumer commands:

```bash
defog show
defog graph
defog inspect
defog report
```

Pipeline example:

```bash
defog changed --kind function -q | defog show -v
```

### 6.4 ChangeSet

A **ChangeSet** is a target set derived from Git state.

Examples:

```bash
defog changed
defog changed --staged
defog changed --unstaged
defog changed --untracked
defog changed --base main
```

Use Git as the source of truth for changes.

---

## 7. Command-line interface

### 7.1 Command overview

The MVP command surface should be:

```bash
defog update
defog status
defog changed
defog ls
defog find
defog show
defog graph
defog inspect
defog report
```

Avoid long commands like:

```bash
defog diagram . --type dependencies --format svg --node ...
```

Prefer:

```bash
defog graph deps
defog graph calls build_transforms
defog report --changed --open
```

---

## 8. Command specifications

### 8.1 `defog update`

Refresh the local Defog index.

```bash
defog update
defog update --force
```

Responsibilities:

```text
1. Discover supported files.
2. Detect changed files by content hash and/or mtime.
3. Parse changed files.
4. Extract targets.
5. Store or update index.
6. Remove stale targets for deleted files.
7. Print summary.
```

Example output:

```text
Defog index updated

Files scanned: 412
Files indexed: 37
Files skipped unchanged: 375
Targets found: 1,284
Functions: 642
Classes/structs: 91
Tests: 104
Elapsed: 1.3s
```

If no index exists, `defog update` creates one.

If a command needs the index and it is missing, it should print:

```text
No Defog index found. Run:
  defog update
```

A command may offer lightweight file-level fallback, but it should not silently do expensive work unless that behavior is documented.

### 8.2 `defog status`

Show Git and index state.

```bash
defog status
```

Responsibilities:

```text
1. Report whether the Defog index exists.
2. Report whether indexed files are stale.
3. Report Git working tree summary.
4. Report changed target summary if possible.
5. Suggest next commands.
```

Example output:

```text
Defog status

Index:
  up to date

Git:
  unstaged modified files: 2
  staged files: 0
  untracked files: 1

Changed targets since HEAD:
  modified files: 2
  modified functions: 3
  added tests: 1

Try:
  defog changed
  defog changed --kind function
  defog report --changed --open
```

Implementation guidance:

```text
Use git status --porcelain.
Use git diff for unstaged changes.
Use git diff --cached for staged changes.
Use git ls-files --others --exclude-standard for untracked files.
```

Shell out to Git. Do not reimplement Git.

### 8.3 `defog changed`

Select targets changed relative to Git state.

```bash
defog changed
defog changed --kind file
defog changed --kind function
defog changed --kind class
defog changed --kind test
defog changed --kind region
defog changed --staged
defog changed --unstaged
defog changed --untracked
defog changed --base main
defog changed -q
defog changed --jsonl
```

Default behavior:

```text
Show changed files and changed symbols since HEAD.
```

Example human output:

```text
Changed since HEAD

modified file      src/data/transforms.py
modified function  src/data/transforms.py::build_transforms@12-45

added file         tests/test_transforms.py
added test         tests/test_transforms.py::test_validation_transforms_are_deterministic@8-31
```

Quiet output:

```bash
defog changed --kind function -q
```

prints:

```text
src/data/transforms.py::build_transforms@12-45
tests/test_transforms.py::test_validation_transforms_are_deterministic@8-31
```

JSONL output:

```json
{"status":"modified","kind":"function","ref":"src/data/transforms.py::build_transforms@12-45","path":"src/data/transforms.py","start_line":12,"end_line":45}
{"status":"added","kind":"test","ref":"tests/test_transforms.py::test_validation_transforms_are_deterministic@8-31","path":"tests/test_transforms.py","start_line":8,"end_line":31}
```

MVP changed-symbol detection:

```text
1. Use Git to get changed files and diff hunks.
2. For each changed file, find indexed current targets whose spans intersect changed hunks.
3. Mark those targets as modified.
4. For untracked files, parse current file and mark file and symbols as added.
5. For deleted files, report deleted file target only in MVP.
```

Do not attempt perfect deleted-function detection in MVP. That requires parsing the base version from Git and comparing symbols. Add later.

### 8.4 `defog ls`

List targets from the index.

```bash
defog ls
defog ls src
defog ls src/data
defog ls src/data --kind function
defog ls --kind class
defog ls --kind test
defog ls --changed
defog ls -q
defog ls --jsonl
```

Example:

```text
$ defog ls src/data

file      src/data/transforms.py
function  src/data/transforms.py::build_transforms@12-45
function  src/data/transforms.py::normalize_image@48-63
file      src/data/dataloader.py
function  src/data/dataloader.py::create_loaders@31-88
function  src/data/dataloader.py::seed_worker@91-103
```

Responsibilities:

```text
1. List targets under an optional scope.
2. Support filtering by kind.
3. Support changed-only filtering.
4. Support quiet and JSONL output.
```

This command is the basic target discovery tool.

### 8.5 `defog find`

Search indexed targets.

```bash
defog find "validation augmentation"
defog find RandomCrop
defog find "decoder" --kind function
defog find "transform" --changed
defog find "augmentation" -q
defog find "augmentation" --jsonl
```

Example output:

```text
Matches for "validation augmentation"

function  src/data/transforms.py::build_transforms@12-45
  matched: validation, augment, transforms

test      tests/test_transforms.py::test_validation_transforms_are_deterministic@8-31
  matched: validation, transforms
```

MVP search may be simple:

```text
symbol name match
path match
content text match
summary/preview match, if available
```

If SQLite FTS5 is already being used, use it for full-text search. If the codebase is small enough and a simpler implementation performs well, an indexed JSON file plus in-memory search is acceptable for MVP. Document the choice.

### 8.6 `defog show`

Show overview or structure for target refs.

```bash
defog show TARGET...
defog show TARGET... -v
defog show -v < targets.txt
defog changed --kind function -q | defog show -v
```

If positional targets are supplied, use them.

If no positional targets are supplied and stdin is not a TTY, read target refs from stdin.

Default mode: overview.

Verbose mode: structure.

#### Overview output

```text
src/data/transforms.py::build_transforms@12-45

Kind:
  function

Location:
  src/data/transforms.py:12-45

Purpose:
  Builds image preprocessing transforms for dataset splits.

Changed:
  modified since HEAD

Relationships:
  called by src/data/dataloader.py::create_loaders@31-88
  uses RandomCrop, RandomHorizontalFlip, Normalize

Try:
  defog show src/data/transforms.py::build_transforms@12-45 -v
  defog graph calls src/data/transforms.py::build_transforms@12-45
  defog inspect src/data/transforms.py:22
```

#### Structure output

```text
src/data/transforms.py::build_transforms@12-45

Signature:
  build_transforms(split, image_size, augment)

Inputs:
  split
  image_size
  augment

Internal structure:
  1. Initializes transform list.
  2. Conditionally adds random augmentation.
  3. Adds tensor conversion.
  4. Adds normalization.
  5. Returns composed transform pipeline.

Important spans:
  augmentation branch: src/data/transforms.py:22-26
  return expression:   src/data/transforms.py:41-45

Calls:
  Resize
  RandomCrop
  RandomHorizontalFlip
  ToTensor
  Normalize
  Compose

Called by:
  src/data/dataloader.py::create_loaders@31-88
```

The structure output should be mostly deterministic and derived from indexed structure. Avoid free-form claims that cannot be tied to code.

### 8.7 `defog graph`

Generate diagrams.

```bash
defog graph contain [TARGET...] [-o FILE]
defog graph deps [TARGET...] [-o FILE]
defog graph calls [TARGET...] [-d N] [-o FILE]
defog graph classes [TARGET...] [-o FILE]
defog graph flow TARGET [-o FILE]
defog changed --kind function -q | defog graph calls -o changed_calls.svg
```

Graph kinds:

```text
contain   containment: repo -> dirs -> files -> symbols
deps      imports/includes dependencies
calls     local call graph
classes   classes, methods, inheritance-like relationships where available
flow      function-level reading flow
```

Output behavior:

```text
If -o ends with .dot, write DOT.
If -o ends with .svg, write SVG by invoking Graphviz dot if available.
If -o is omitted, write SVG to .dfog/graphs/<generated-name>.svg when Graphviz is available, otherwise write DOT.
```

Do not link against Graphviz for MVP. Shell out to `dot` if installed.

If Graphviz is missing:

```text
Graphviz 'dot' not found.
Wrote DOT instead:
  .dfog/graphs/changed_calls.dot

Install Graphviz or run:
  dot -Tsvg .dfog/graphs/changed_calls.dot -o changed_calls.svg
```

Multi-target behavior:

```text
contain/deps/calls/classes:
  May aggregate multiple targets into one graph.

flow:
  Requires one function/method target by default.
  If multiple flow targets are provided, either write one file per target with --out-dir or tell the user to use defog report.
```

### 8.8 `defog inspect`

Inspect a precise target or location.

```bash
defog inspect TARGET
defog inspect src/data/transforms.py:22
defog inspect src/data/transforms.py::build_transforms@12-45
```

This command should be focused and code-centered.

Example:

```text
$ defog inspect src/data/transforms.py:22

Location:
  src/data/transforms.py:22

Containing target:
  src/data/transforms.py::build_transforms@12-45

Source:
  22 | if augment and split != "test":

Structure:
  branch condition
  branch span: src/data/transforms.py:22-26

Calls inside branch:
  RandomCrop
  RandomHorizontalFlip
```

Do not turn `inspect` into a broad explanation command. It should stay local:

```text
line
small span
branch
call
return
changed region
```

### 8.9 `defog report`

Generate a static human-readable report.

```bash
defog report
defog report --changed
defog report --changed --base main
defog report src/data
defog report src/data/transforms.py::build_transforms@12-45
defog changed --kind function -q | defog report --open
defog report --open
defog report -o .dfog/report
```

Default output:

```text
.dfog/report/index.html
```

Report types:

```text
whole repo report
changed-code report
subtree report
target-set report
single-target report
```

For the MVP, prioritize:

```bash
defog report --changed --open
```

#### Changed-code report contents

```text
1. Header
   - repo path
   - base ref
   - time generated
   - number of changed files/targets

2. Change summary
   - files changed
   - functions changed
   - tests changed
   - untracked files

3. Changed targets
   - overview for each target
   - structure for important targets
   - source span excerpts

4. Diagrams
   - changed containment graph
   - changed dependency graph
   - call graphs for changed functions
   - flow graph for selected changed functions where useful

5. Source
   - syntax-highlight optional
   - plain preformatted source acceptable for MVP
   - line numbers required

6. Footer
   - command used to generate report
```

The report can be static HTML with embedded SVG or links to SVG files.

No server is required.

---

## 9. Target resolution

Defog must resolve user input into targets.

Accepted target forms:

```text
path
path:line
path::symbol
path::qualified_symbol
path::qualified_symbol@start-end
bare symbol name, if unambiguous
```

Examples:

```bash
defog show src/data/transforms.py
defog show src/data/transforms.py:22
defog show src/data/transforms.py::build_transforms
defog show build_transforms
```

Resolution order:

```text
1. Exact TargetRef.
2. Existing path.
3. Path plus line.
4. Path plus symbol.
5. Exact qualified symbol.
6. Exact unqualified symbol.
7. Fuzzy symbol/path match.
8. Search fallback.
```

If ambiguous:

```text
"load" matched multiple targets:

function  src/io/image.py::load_image@14-48
method    src/data/dataset.py::Dataset.load@31-72
method    src/loader.cc::defog::Loader::Load@44-96

Run:
  defog show src/io/image.py::load_image@14-48
  defog show src/data/dataset.py::Dataset.load@31-72
  defog show src/loader.cc::defog::Loader::Load@44-96
```

Do not use session-local handles in the MVP.

---

## 10. Indexing and storage

### 10.1 Storage principle

Use the simplest storage design that satisfies performance and reliability.

The target repo size is around 100k LOC. The first implementation does not need a heavy database if a simpler design works.

Acceptable MVP storage options:

```text
Option A: JSON/JSONL index files under .dfog/index/
Option B: SQLite database under .dfog/index.sqlite
```

The coding agent should choose based on simplicity and performance.

Guidance:

```text
Use JSON/JSONL if:
  - implementation is simpler
  - target listing and lookup are fast enough
  - search can be handled in memory at ~100k LOC

Use SQLite if:
  - search becomes noticeably slow
  - incremental updates become awkward
  - durable querying becomes simpler
  - FTS5 is desired for fast text search
```

Do not introduce SQLite just because it sounds architectural. Introduce it when it reduces complexity or improves performance.

### 10.2 Required index capabilities

Regardless of storage backend, the index must support:

```text
list targets by scope
list targets by kind
resolve TargetRef
find targets by name/path/content
map changed hunks to current targets
show parent/child relationships
show basic dependency/call relationships where available
generate diagrams
```

Minimum target record:

```json
{
  "ref": "src/data/transforms.py::build_transforms@12-45",
  "kind": "function",
  "name": "build_transforms",
  "qualified_name": "build_transforms",
  "path": "src/data/transforms.py",
  "language": "python",
  "start_line": 12,
  "end_line": 45,
  "signature": "build_transforms(split, image_size, augment)",
  "parent_ref": "src/data/transforms.py",
  "content_hash": "...",
  "is_test": false
}
```

Minimum edge record:

```json
{
  "source_ref": "src/data/dataloader.py::create_loaders@31-88",
  "target_ref": "src/data/transforms.py::build_transforms@12-45",
  "kind": "calls",
  "confidence": 0.7
}
```

Edge kinds:

```text
contains
imports
calls
references
test_related
```

### 10.3 File discovery

Supported file types for MVP:

```text
.go
.py
.cc
.cpp
.cxx
.h
.hpp
```

Ignore by default:

```text
.git/
.dfog/
build/
bazel-*/
cmake-build-*/
node_modules/
.venv/
venv/
__pycache__/
dist/
third_party/ optional, configurable
external/ optional, configurable
```

Keep ignore rules simple.

Later support:

```text
.dfogignore
respect .gitignore
language-specific ignores
```

---

## 11. Language support

### 11.1 MVP language targets

The MVP should support:

```text
Go
Python
C++
```

But implementation should be staged.

Recommended order:

```text
1. File discovery and target model.
2. Go extractor.
3. Python extractor.
4. C++ extractor.
5. Diagrams.
6. Reports.
```

If scope must be reduced, implement one language first but keep the parser/extractor interface language-agnostic.

### 11.2 Extraction scope

Extract only structural information.

Go:

```text
imports
packages
structs
interfaces
functions
methods
signatures
line spans
containment
call-like expressions
test files/functions
basic branch/loop/return locations, for flow graphs
```

Python:

```text
imports
classes
functions
methods
signatures
line spans
containment
call-like expressions
test functions/classes
basic branch/loop/return locations, for flow graphs
```

C++:

```text
includes
namespaces
classes
structs
constructors
methods
free functions
signatures
line spans
containment
call-like expressions
test files/functions
basic branch/loop/return locations, for flow graphs
```

Do not implement:

```text
full type checking
Go interface/type inference
C++ overload resolution
template instantiation
macro expansion correctness
full compiler-grade control-flow graph
full data-flow analysis
```

Approximate call edges are acceptable if marked with confidence below `1.0`.

---

## 12. Explanation model

The user-facing model has two main explanation modes:

```text
overview
structure
```

And one focused local mode:

```text
inspect
```

Do not implement six explanation levels in the MVP.

### 12.1 Overview

Answers:

```text
What is this?
Where does it live?
Why might I care?
How does it relate to nearby code?
```

### 12.2 Structure

Answers:

```text
What is its signature?
What are its major internal parts?
What does it call?
What calls it?
What spans are important?
```

### 12.3 Inspect

Answers:

```text
What is this exact line/span/branch/call/return?
What contains it?
What local structure surrounds it?
```

The MVP summaries should be deterministic and structure-derived.

Avoid free-form LLM summaries in the MVP.

---

## 13. Diagrams

### 13.1 Diagram philosophy

Diagrams are a primary part of the product, but Defog should not implement diagram layout.

Pipeline:

```text
Defog target graph
  -> DiagramGraph
  -> DOT
  -> Graphviz dot
  -> SVG
  -> HTML report
```

### 13.2 Diagram types

MVP diagram types:

```text
contain
deps
calls
flow
```

Optional later:

```text
classes
tests
changed-impact
```

### 13.3 `contain` graph

Shows containment:

```text
directory -> file -> class/function/method
```

Useful for:

```text
What exists here?
What did the agent add?
How is this file/subtree organized?
```

### 13.4 `deps` graph

Shows import/include dependencies:

```text
file -> imported file/module
```

For Python, use import statements.

For Go, use import declarations.

For C++, use includes.

Do not attempt perfect include-path resolution in MVP. If unresolved, show external dependency label.

### 13.5 `calls` graph

Shows approximate local call relationships.

Input:

```text
one or more function/method targets
```

Output:

```text
target function(s)
callers where known
callees where detected
```

MVP call graph can be heuristic.

### 13.6 `flow` graph

Shows a function-level reading outline:

```text
function start
assignment/init
branch
loop
call
return
```

This is not a formal CFG. It is a reading aid.

---

## 14. Report design

The report is the primary visual reading surface.

### 14.1 Static HTML

The MVP report should be static HTML.

No local web server required.

Output directory:

```text
.dfog/report/
```

Default entry:

```text
.dfog/report/index.html
```

### 14.2 Changed-code report layout

For:

```bash
defog report --changed --open
```

Generate:

```text
1. Header
   - repo path
   - base ref
   - time generated
   - number of changed files/targets

2. Change summary
   - files changed
   - functions changed
   - tests changed
   - untracked files

3. Changed targets
   - overview for each target
   - structure for important targets
   - source span excerpts

4. Diagrams
   - changed containment graph
   - changed dependency graph
   - call graphs for changed functions
   - flow graph for selected changed functions

5. Source
   - syntax-highlight optional
   - plain preformatted source acceptable for MVP
   - line numbers required

6. Footer
   - command used to generate report
```

### 14.3 Links

Where practical, link:

```text
target summary -> source span
graph node -> target section
changed file -> file section
function name -> function section
```

Do not overbuild frontend interactivity.

---

## 15. Go implementation guidance

### 15.1 Project shape

Recommended structure:

```text
defog/
  go.mod
  cmd/
    defog/
      main.go

  internal/
    app/
    cli/
    core/
    discovery/
    git/
    parsing/
    indexing/
    search/
    explain/
    diagram/
    report/
    output/
    util/

  tests/
    golden/

  testdata/
    repos/
```

The exact structure can vary, but keep boundaries clear.

### 15.2 Core modules

Suggested modules:

```text
internal/core/target.go
internal/core/target_ref.go
internal/core/target_set.go
internal/core/code_span.go
internal/core/edge.go

internal/discovery/file_discovery.go
internal/git/git_client.go
internal/git/change_detector.go

internal/parsing/parser.go
internal/parsing/parser_registry.go
internal/parsing/go_extractor.go
internal/parsing/python_extractor.go
internal/parsing/cpp_extractor.go

internal/indexing/index_store.go
internal/indexing/file_index_store.go
internal/indexing/sqlite_index_store.go optional

internal/search/searcher.go
internal/explain/explainer.go

internal/diagram/diagram_graph.go
internal/diagram/diagram_builder.go
internal/diagram/dot_exporter.go
internal/diagram/graphviz_renderer.go

internal/report/report_builder.go
internal/report/html_writer.go

internal/cli/command.go
internal/cli/target_input.go
internal/cli/output_mode.go
```

### 15.3 Error handling

Use ordinary Go error returns. Wrap errors at package boundaries with enough context for CLI messages and tests, using `fmt.Errorf("...: %w", err)` where appropriate.

Do not introduce a custom status/result abstraction unless it removes concrete duplication and keeps call sites clearer than idiomatic Go.

### 15.4 Determinism

Output order must be deterministic.

Sort by:

```text
path
start_line
kind
name
```

This matters for tests, reports, and user trust.

---

## 16. Build and test tooling

Use the Go toolchain.

Use Go's standard `testing` package for unit and integration tests.

Keep tests colocated with the packages they exercise using `_test.go` files. Use `testdata/` for fixtures and golden outputs.

Suggested commands:

```bash
go test ./...
go build ./cmd/defog
```

Formatting/linting:

```text
gofmt
go vet optional
staticcheck optional
```

---

## 17. TDD milestones

### Milestone 0 - CLI skeleton

Tests first:

```text
defog with no args prints help.
defog --help prints command list.
defog version prints version string.
Unknown command exits nonzero.
```

Commands may be stubs.

### Milestone 1 - TargetRef parser

Tests first:

```text
Parses path target.
Parses path:line target.
Parses path::symbol target.
Parses path::symbol@start-end target.
Rejects malformed refs.
Round-trips refs deterministically.
```

### Milestone 2 - File discovery

Tests first:

```text
Discovers .go, .py, .cc, .cpp, .h, .hpp files.
Ignores .git.
Ignores .dfog.
Ignores build directories.
Ignores venv/node_modules.
Returns sorted paths.
```

### Milestone 3 - Index store minimal

Tests first:

```text
Can write target.
Can read target by ref.
Can list targets by path scope.
Can list targets by kind.
Can replace targets for one file.
Can delete targets for one file.
```

Implementation may be JSON/JSONL or SQLite.

Do not over-engineer.

### Milestone 4 - Git client

Tests first using a temporary Git repo:

```text
Detects modified file.
Detects staged file.
Detects untracked file.
Detects diff hunks.
Detects base ref diff.
```

Use shell-out to Git.

### Milestone 5 - Go extraction

Tests first with small fixture:

```go
package data

import "context"

type Loader struct {
    root string
}

func NewLoader(root string) *Loader {
    return &Loader{root: root}
}

func (l *Loader) Load(ctx context.Context, index int) int {
    return index
}
```

Expected targets:

```text
file
package data
struct Loader
function NewLoader
method Loader.Load
```

Expected:

```text
stable refs
correct spans
containment edges
signature text
```

### Milestone 6 - Python extraction

Tests first with small fixture:

```python
import os

class Dataset:
    def __init__(self, root):
        self.root = root

    def load(self, index):
        return index

def build_transforms(split, image_size, augment):
    return None
```

Expected targets:

```text
file
class Dataset
method Dataset.__init__
method Dataset.load
function build_transforms
```

Expected:

```text
stable refs
correct spans
containment edges
signature text
```

### Milestone 7 - C++ extraction

Tests first with small fixture:

```cpp
#include <string>

namespace defog {

class Loader {
 public:
  explicit Loader(std::string root);
  int Load(int index) const;
};

int BuildTransforms(int split) {
  return split;
}

}  // namespace defog
```

Expected targets:

```text
file
namespace defog
class Loader
constructor Loader::Loader
method Loader::Load
function defog::BuildTransforms
```

Keep this structural. Do not solve all C++.

### Milestone 8 - `defog update`

Tests first:

```text
update creates index.
update stores targets.
update skips unchanged files where possible.
update removes targets for deleted files.
update output is deterministic.
```

### Milestone 9 - `defog ls`

Tests first:

```text
ls lists all targets.
ls scope filters by path.
ls --kind filters by kind.
ls -q emits refs only.
ls --jsonl emits valid JSONL.
```

### Milestone 10 - `defog changed`

Tests first:

```text
changed lists modified files.
changed --kind function lists functions intersecting diff hunks.
changed --staged uses staged diff.
changed --unstaged uses unstaged diff.
changed --untracked lists untracked files.
changed -q emits refs only.
```

MVP can report deleted files but not deleted functions.

### Milestone 11 - `defog show`

Tests first:

```text
show path prints file overview.
show function ref prints function overview.
show -v prints structure.
show reads refs from stdin.
show handles ambiguous bare symbols with helpful choices.
```

### Milestone 12 - `defog find`

Tests first:

```text
find by symbol name.
find by path.
find by content.
find --kind filters.
find --changed filters.
find -q emits refs.
```

Start with simple search.

### Milestone 13 - Diagram model and DOT export

Tests first:

```text
DiagramGraph stores nodes and edges.
Rejects or handles duplicate node IDs deterministically.
DOT exporter escapes labels.
DOT output is deterministic.
Contain graph exports expected DOT.
Deps graph exports expected DOT.
Calls graph exports expected DOT.
Flow graph exports expected DOT for simple function.
```

### Milestone 14 - `defog graph`

Tests first:

```text
graph contain writes DOT.
graph deps writes DOT.
graph calls writes DOT.
graph flow writes DOT.
-o .dot writes DOT.
-o .svg invokes renderer abstraction.
If dot is missing, useful error/fallback.
Reads targets from stdin.
```

Use a fake renderer in tests.

### Milestone 15 - `defog inspect`

Tests first:

```text
inspect file:line finds containing target.
inspect prints source line.
inspect function prints important spans.
inspect changed region prints span.
```

### Milestone 16 - `defog report`

Tests first:

```text
report writes index.html.
report --changed includes changed files.
report includes target summaries.
report includes diagrams or DOT fallback.
report includes source spans with line numbers.
report reads targets from stdin.
report output is deterministic for fixture repo.
```

### Milestone 17 - Performance sanity checks

Tests or benchmarks:

```text
Index fixture with 1k files.
Warm update skips unchanged files.
changed command completes quickly after index exists.
find command completes quickly on synthetic 100k LOC repo.
```

Benchmarks should inform design, not block early development.

---

## 18. Performance targets

For a repo around 100k LOC:

```text
Cold update: under 10 seconds on a normal developer laptop
Warm update with no changes: under 1 second
Changed command after update: under 500 ms
Find command: under 300 ms
Show command: under 100 ms
Report --changed: under 3 seconds for typical small change set
```

These are goals, not absolute guarantees.

Design choices:

```text
hash files to skip unchanged work
parse only changed files during update
store enough index data to avoid reparsing for ls/find/show
batch writes if using SQLite
avoid loading full source text unless needed
keep report scope bounded
```

---

## 19. Data contracts

### 19.1 Quiet output

Quiet output is one TargetRef per line:

```text
src/data/transforms.py::build_transforms@12-45
src/data/dataloader.py::create_loaders@31-88
```

No headers. No extra text.

### 19.2 JSONL output

Each line is a JSON object.

Common fields:

```json
{
  "ref": "src/data/transforms.py::build_transforms@12-45",
  "kind": "function",
  "name": "build_transforms",
  "path": "src/data/transforms.py",
  "start_line": 12,
  "end_line": 45
}
```

For changed targets:

```json
{
  "status": "modified",
  "ref": "src/data/transforms.py::build_transforms@12-45",
  "kind": "function",
  "path": "src/data/transforms.py",
  "start_line": 12,
  "end_line": 45
}
```

### 19.3 Stdin behavior

These commands must read TargetRefs from stdin when no positional target is provided and stdin is not a TTY:

```bash
defog show
defog graph
defog report
```

`defog inspect` may require exactly one target in MVP.

---

## 20. MVP acceptance criteria

The MVP is complete when all of this works:

```bash
defog update
defog status
defog changed
defog changed --kind function
defog changed --kind function -q | defog show -v
defog ls src --kind function
defog find "some query"
defog graph contain -o contain.dot
defog graph deps -o deps.dot
defog graph calls <function-ref> -o calls.dot
defog inspect <path>:<line>
defog report --changed --open
```

And:

```text
All tests pass.
Output is deterministic.
Target refs are pipeable.
No hidden session state is required.
Git is used for change detection.
Graphviz/DOT is used for diagrams.
The tool handles ~100k LOC repos acceptably.
The MVP does not require a live LLM.
The MVP does not attempt compiler-grade analysis.
```

---

## 21. Suggested direct instruction block for Codex

```text
You are implementing Defog, a local human-first code comprehension tool.
The CLI command is defog.

Primary goal:
Help a developer understand code that was just generated or changed.

Optimize the MVP for this workflow:
  defog update
  defog status
  defog changed --kind function
  defog changed --kind function -q | defog show -v
  defog report --changed --open

Design principles:
- Use existing tools instead of reinventing them.
- Use Git for change detection.
- Use DOT/Graphviz for diagrams.
- Use parsing only for structural extraction, not compiler-grade semantics.
- Keep the implementation simple.
- Use a database only if it is clearly useful for performance or simplicity.
- Target repos around 100k LOC, not multi-million-line monorepos.
- Do not implement hidden sessions.
- Use explicit TargetRefs and Unix-style pipelines.
- Human-readable output by default.
- Use -q for pipeable refs.
- Use --jsonl for machine-readable output.
- Keep output deterministic.
- Do not require live LLM calls in the MVP.

Implementation requirements:
- Implement Defog in Go.
- Use Go 1.22 or newer.
- Use Go's standard `testing` package.
- Prefer the Go standard library and small, justified dependencies.
- Use ordinary Go error returns with contextual wrapping.
- Keep packages small and testable.

Implement red-green TDD:
1. Write failing tests first.
2. Confirm failure.
3. Implement minimum code to pass.
4. Refactor while green.

MVP commands:
  defog update
  defog status
  defog changed [--kind KIND] [--staged|--unstaged|--untracked] [--base REF] [-q|--jsonl]
  defog ls [SCOPE] [--kind KIND] [--changed] [-q|--jsonl]
  defog find QUERY [--kind KIND] [--changed] [-q|--jsonl]
  defog show [TARGET...] [-v]
  defog graph <contain|deps|calls|flow> [TARGET...] [-d N] [-o FILE] [--out-dir DIR]
  defog inspect TARGET
  defog report [TARGET...] [--changed] [--base REF] [--open] [-o DIR]

TargetRef format:
  path
  path:line
  path::symbol
  path::qualified_symbol@start-end

Selector commands produce target sets:
  defog changed
  defog ls
  defog find

View commands consume target sets:
  defog show
  defog graph
  defog report
  defog inspect

Commands that consume targets should accept positional targets or read TargetRefs from stdin when no targets are supplied.

Do not use session-local handles such as @1 or @2 in the MVP.
```

---

## 22. Final product shape

The clean design is:

```text
Git-aware selectors
  changed
  ls
  find

Pipeable target refs
  path::symbol@start-end

Human-facing views
  show
  graph
  inspect
  report

Simple durable index
  enough to list, search, resolve, and map changes to code targets
```

Defog should feel like:

```text
git status
git diff
grep
ctags
Graphviz
static report generator
```

combined into one focused tool for reading changed code.

It should not feel like:

```text
an IDE
a compiler
a database project
an agent framework
a custom visualization platform
```

The MVP wins if, after Codex changes a repo, the developer can run one or two commands and quickly understand what happened.

---

## 23. Reference notes

These references support the implementation choices, but the spec intentionally leaves room for the coding agent to make simpler choices when appropriate.

- Go documentation: https://go.dev/doc/
- Effective Go: https://go.dev/doc/effective_go
- Go testing package: https://pkg.go.dev/testing
- Git status documentation: https://git-scm.com/docs/git-status
- Git diff documentation: https://git-scm.com/docs/git-diff
- Graphviz DOT language: https://graphviz.org/doc/info/lang.html
- Graphviz overview: https://graphviz.org/
- Tree-sitter overview: https://tree-sitter.github.io/
- SQLite FTS5 documentation: https://www.sqlite.org/fts5.html
