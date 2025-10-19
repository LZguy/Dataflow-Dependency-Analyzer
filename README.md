# Dataflow Dependency Analyzer (dflow_calc)

A single-file implementation of a **dataflow dependency analyzer** that builds a dependency DAG from a program trace and answers queries about:
- per-instruction **dependency depth** (cycles to earliest issue), and
- **direct RAW dependencies** (two sources), and
- the program-wide **critical path** (Entry→Exit). :contentReference[oaicite:0]{index=0}

The analyzer exposes the interface defined in `dflow_calc.h` and plugs into the provided `dflow_main` harness. :contentReference[oaicite:1]{index=1} :contentReference[oaicite:2]{index=2}

---

## 📘 Overview

- We model **true (RAW) dependencies only** and ignore memory deps; the program is a DAG with special **Entry/Exit** nodes. The **depth** of an instruction is the longest weighted path (in cycles) from Entry to its issue (excluding its own latency). The **program depth** is the longest path Entry→Exit. :contentReference[oaicite:3]{index=3}  
- Input consists of:
  - `opsLatency[]` — latency per opcode (0..MAX_OPS−1), read from a simple text file (one decimal per line). :contentReference[oaicite:4]{index=4}
  - `progTrace[]` — a trace of `{opcode, dst, src1, src2}` rows in architectural commit order. :contentReference[oaicite:5]{index=5}

---

## 🧩 API (from `dflow_calc.h`)

```c
ProgCtx analyzeProg(const unsigned int opsLatency[],
                    const InstInfo progTrace[],
                    unsigned int numOfInsts);

void    freeProgCtx(ProgCtx ctx);

int     getInstDepth(ProgCtx ctx, unsigned int theInst);     // cycles from Entry (no self-latency)
int     getInstDeps (ProgCtx ctx, unsigned int theInst,
                     int *src1DepInst, int *src2DepInst);    // -1 means Entry
int     getProgDepth(ProgCtx ctx);                           // cycles Entry→Exit
```

These signatures are fixed by the harness and must be implemented exactly.

---

## 🛠️ Implementation Notes (our dflow_calc.c)

- We track, for **32 architectural registers**, the **last writer** instruction index. For each instruction i, we map src1/src2 to producing instructions (or **Entry** = -1), then compute:

```
depth[i] = max(
    dep1 == -1 ? 0 : depth[dep1] + latency[dep1],
    dep2 == -1 ? 0 : depth[dep2] + latency[dep2]
)
```
and update last_writer[dst] = i. The **program depth** is max_i (depth[i] + latency[i]). 

- Memory is ignored by design; only register RAW deps are considered. 
- The skeleton file given in the course stubbed all functions (returning defaults); we replaced it with a full implementation.

---

## 📂 Project Structure
```
dflow-analyzer/
├─ src/
│  ├─ dflow_calc.c            # full implementation (this repo)
│  ├─ dflow_calc.h            # API (provided)
│  └─ dflow_main.c            # harness: I/O + CLI queries
├─ examples/                  # provided by the course' staff
├─ tools/
│  └─ makefile                # builds 'dflow_calc' executable
├─ .gitignore
├─ LICENSE
└─ README.md
```
**Note:** Course handout and additional official examples are not included here. Use your own traces/latency files or those you’re allowed to distribute. The harness’ expected formats are documented in its source and the spec.

---

## ⚙️ Build
```bash
make            # produces: dflow_calc
```
The makefile compiles your dflow_calc.c together with the provided dflow_main.c and links them into dflow_calc. Running without args prints usage.

---

## ▶️ Run
```bash
./dflow_calc <opcode-latency-file> <program-trace> [queries...]
# Query forms:
#   p<i>  → dependency depth of instruction i
#   d<i>  → direct deps {src1, src2} of instruction i
```
Example:
```bash
./dflow_calc opcode1.dat example1.in p3 d5 p0 p12
```
The harness first prints getProgDepth()==<cycles> and then answers each query.

**Trace format** (example1.in):
```
<opcode> <dst> <src1> <src2>
1 2 1 3
1 5 1 0
0 4 2 0
...
```
(as provided in the course examples).

---

## ✅ Correctness Expectations
- getInstDepth() returns **0** for instructions that depend only on **Entry**.
- getInstDeps() returns instruction indices or **-1** for Entry.
- getProgDepth() equals the longest weighted path Entry→Exit (sum of instruction latencies along the critical chain).
