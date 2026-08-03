# APDFS: Adaptive Parallel Depth-First Search for Enumerating All Minimal (S,T)-Cuts in Directed Graphs

## Completeness, Correctness, and Termination

---

## 1. Definitions

### 1.1. Graph

Let a directed graph be given as $G = (V, E)$, with $|V| = n$ and $|E| = m$.

Two distinguished vertices are introduced:

- $S^\*$ — the supersource ($\deg_{in}(S^\*) = 0$ in $G$);
- $T^\*$ — the supersink ($\deg_{out}(T^\*) = 0$ in $G$).

The reduction of multiple sources $\{s_i\}$ and sinks $\{t_j\}$ to $S^\*$ and $T^\*$ is accomplished by adding dummy edges $(S^\*, s_i)$ and $(t_j, T^\*)$. Such edges cannot belong to a minimal $(S^\*, T^\*)$-cut except in the trivial case, and are filtered out in postprocessing if necessary.

The graph $G$ may contain cycles. Preliminary contraction of strongly connected components (SCCs) is not required.

### 1.2. Path

**Definition 1 (Path).** A directed path from $u$ to $v$ is a sequence of vertices

$$\pi = \langle u = x_0, x_1, \ldots, x_k = v \rangle,$$

such that for every $i \in \{0, \ldots, k-1\}$, the edge $(x_i, x_{i+1}) \in E$, and all $x_i$ are pairwise distinct. The number $k$ is called the length of the path.

### 1.3. Cut

**Definition 2 (Multicut).** A set of edges $Q \subseteq E$ is called an $(S^\*, T^\*)$-multicut if every path from $S^\*$ to $T^\*$ contains at least one edge from $Q$.

**Definition 3 (Minimal cut).** A multicut $P$ is called a minimal cut if no proper subset $P' \subset P$ is a multicut.

The set of all minimal cuts of the graph $G$ is denoted by $\mathbb{D}$.

**Definition 4 (Free path).** Let $P \in \mathbb{D}$ and $e \in P$. A path $\pi$ from $S^\*$ to $T^\*$ is called free for $e$ with respect to $P$ if $e \in \pi$ and $\pi \cap P = \{e\}$.

**Theorem 1 (Minimality criterion).** A multicut $P$ is a minimal cut if and only if for every $e \in P$ there exists a free path with respect to $P$.

**Proof.**

$(\Rightarrow)$ Let $P \in \mathbb{D}$. Suppose that for some $e \in P$ no free path exists. Set $P' = P \setminus \{e\}$. Consider an arbitrary path $\pi$ from $S^\*$ to $T^\*$. If $e \notin \pi$, then $\pi$ contains an edge from $P' \subseteq P$. If $e \in \pi$, then by assumption $\pi$ contains some other edge $e' \in P$, $e' \neq e$. Hence $e' \in P'$. In both cases $\pi \cap P' \neq \varnothing$, so $P'$ is a multicut. This contradicts the minimality of $P$.

$(\Leftarrow)$ Suppose that for every $e \in P$ there exists a free path $\pi_e$. Remove an arbitrary edge $e \in P$. The path $\pi_e$ contains no other edges of $P$, hence $\pi_e \cap (P \setminus \{e\}) = \varnothing$. Therefore $P \setminus \{e\}$ is not a multicut. Since $e$ was chosen arbitrarily, $P$ is minimal. $\square$

### 1.4. Closed Vertex Set

**Definition 5 (Closed set).** For a multicut $Q$, the closed vertex set is defined as

$$V_1(Q) = \{\, v \in V \cup \{S^\*, T^\*\} \mid \exists \text{ a path } S^\* \leadsto v \text{ in } G \setminus Q \,\}.$$

**Lemma 1 (Properties of $V_1$).** Let $Q$ be a multicut. Then:

1. $S^\* \in V_1(Q)$.
2. If $u \in V_1(Q)$ and there is a path $u \leadsto v$ in $G \setminus Q$, then $v \in V_1(Q)$.
3. $Q$ is a multicut $\iff$ $T^\* \notin V_1(Q)$.
4. If $P \in \mathbb{D}$, then $P = \{\, (u, v) \in E \mid u \in V_1(P),\; v \notin V_1(P) \,\}$.

**Proof.**

1. The zero-length path from $S^\*$ to $S^\*$ contains no edges of $Q$; hence $S^\* \in V_1(Q)$.
2. Concatenating a path $S^\* \leadsto u$ in $G \setminus Q$ with a path $u \leadsto v$ in $G \setminus Q$ yields a path $S^\* \leadsto v$ in $G \setminus Q$. Hence $v \in V_1(Q)$.
3. If $T^\* \in V_1(Q)$, then there exists a path $S^\* \leadsto T^\*$ avoiding $Q$, so $Q$ is not a multicut. Conversely, if $Q$ is not a multicut, then there exists a path $S^\* \leadsto T^\*$ avoiding $Q$, so $T^\* \in V_1(Q)$.
4. Let $B = \{\, (u, v) \in E \mid u \in V_1(P), v \notin V_1(P) \,\}$. Every path $S^\* \leadsto T^\*$ must cross the boundary of $V_1(P)$, so $B$ is a multicut. Any edge $(u, v) \in P$ must have $u \in V_1(P)$ (otherwise the edge is unreachable in $G \setminus P$, and $P \setminus \{(u,v)\}$ is a multicut, a contradiction) and $v \notin V_1(P)$ (otherwise $v \in V_1(P)$, and $P \setminus \{(u,v)\}$ is a multicut, a contradiction). Hence $P \subseteq B$. By minimality of $P$, we have $P = B$. $\square$

---

## 2. Branching Operation

**Definition 6 (Branching operation).** Let $P \in \mathbb{D}$ and $e = (f, v) \in P$, where $v \neq T^\*$. Then

$$\Psi(P, e) = (P \setminus \{e\}) \cup \Gamma^+(v),$$

where $\Gamma^+(v) = \{\, (v, w) \in E \,\}$ is the set of outgoing edges from $v$.

**Remark.** The case $v = T^\*$ is excluded from the definition because $\Gamma^+(T^\*) = \varnothing$, and the set $P \setminus \{e\}$ is not a multicut (the free path for $e$ remains uncovered). Branching on edges leading to the sink is not performed.

**Lemma 2.** For any $P \in \mathbb{D}$ and $e = (f, v) \in P$, where $v \neq T^\*$, the set $\Psi(P, e)$ is a multicut.

**Proof.** Let $\pi$ be an arbitrary path $S^\* \leadsto T^\*$. If $e \notin \pi$, then $\pi$ contains an edge from $P \setminus \{e\} \subseteq \Psi(P, e)$. If $e \in \pi$, then $\pi = \ldots \to f \xrightarrow{e} v \to \ldots$, and after $v$ the path continues along an edge $(v, w) \in \Gamma^+(v) \subseteq \Psi(P, e)$. In both cases $\pi \cap \Psi(P, e) \neq \varnothing$. $\square$

---

## 3. Cleanup Procedure

### 3.1. Description

**Input:** a multicut $Q$ and the set $V_1 = V_1(Q)$.

**Output:** a minimal cut $P \subseteq Q$, or $\varnothing$ if $Q$ contains no minimal cut.

**Algorithm Clean$(Q, V_1)$:**

1. Construct the set $V_T$ — vertices from which $T^\*$ is reachable without using edges of $Q$:
   $$V_T = \{\, v \in V \cup \{T^\*\} \mid \exists \text{ a path } v \leadsto T^\* \text{ in } G \setminus Q \,\}.$$
   The construction is performed by a single reverse BFS from $T^\*$ along reversed edges.
2. Set $P = \{\, e = (u, w) \in Q \mid u \in V_1 \text{ and } w \in V_T \,\}$.
3. Return $P$.

### 3.2. Correctness

**Theorem 2.** Let $Q$ be a multicut, $V_1 = V_1(Q)$, $V_T$ constructed as above, and $P = \text{Clean}(Q, V_1)$. Then:

1. If $P = \varnothing$, then $Q$ contains no minimal cut.
2. If $P \neq \varnothing$, then $P$ is a minimal cut and $P \subseteq Q$.

**Proof.**

We prove the equivalence of Clean and the Reduce procedure defined below. Let $\text{Reduce}(R)$ be a procedure that iterates over the edges of $R$ and removes an edge $e$ if $R \setminus \{e\}$ is a multicut. It is well known that $\text{Reduce}(R)$ returns a minimal cut $P \subseteq R$ if $R$ is a multicut.

**Lemma 3 (Characterization of redundant edges).** An edge $e' = (a, b) \in Q$ is redundant (i.e., $Q \setminus \{e'\}$ is a multicut) if and only if $b \notin V_T$.

**Proof of Lemma 3.**

$(\Rightarrow)$ Suppose $e'$ is redundant. Then $R = Q \setminus \{e'\}$ is a multicut. Assume that $b \in V_T$. By definition of $V_T$, there exists a path $\pi_b: b \leadsto T^\*$ in $G \setminus Q$. Since $a \in V_1$, there exists a path $\pi_a: S^\* \leadsto a$ in $G \setminus Q$. Then $\pi = \pi_a \circ (a, b) \circ \pi_b$ is a path $S^\* \leadsto T^\*$ containing $e'$ and no other edges of $Q$ (since $\pi_a$ and $\pi_b$ avoid $Q$). Hence $\pi \cap R = \varnothing$, contradicting that $R$ is a multicut. Therefore $b \notin V_T$.

$(\Leftarrow)$ Suppose $b \notin V_T$. We show that $e'$ is redundant. Consider an arbitrary path $\pi$ from $S^\*$ to $T^\*$. If $e' \notin \pi$, then $\pi$ contains an edge of $Q \setminus \{e'\}$, since $Q$ is a multicut. If $e' \in \pi$, then $\pi = \pi_{pre} \circ (a, b) \circ \pi_{suf}$. Since $b \notin V_T$, every path $b \leadsto T^\*$ contains an edge of $Q$. In particular, $\pi_{suf}$ contains $e'' \in Q$, $e'' \neq e'$ (since the path is simple, $b$ appears exactly once). Hence $\pi \cap (Q \setminus \{e'\}) \neq \varnothing$. Thus $Q \setminus \{e'\}$ is a multicut. $\square$

From Lemma 3 it follows that the set of non-redundant edges in $Q$ is exactly $\{ (a, b) \in Q \mid b \in V_T \}$. Since by definition of $V_1$ all non-redundant edges of $Q$ have $a \in V_1$ (Lemma 1.4), this set coincides with the output of Clean. The Reduce procedure, removing redundant edges in any order, also leaves exactly this set. Therefore $\text{Clean}(Q, V_1) = \text{Reduce}(Q)$, and the result is a minimal cut. $\square$

**Corollary 1 (Uniqueness).** Any multicut $Q$ contains at most one minimal cut.

**Proof.** The Clean procedure deterministically computes a set $P$. By Theorem 2, every minimal cut $R \subseteq Q$ must coincide with $P$. $\square$

---

## 4. Search Graph

### 4.1. Transition Operator

**Definition 7 (Transition operator).** For $P \in \mathbb{D}$ and $e = (f, v) \in P$, where $v \neq T^\*$, the operator is defined as

$$\Phi(P, e) = \text{Clean}(\Psi(P, e), V_1(\Psi(P, e))).$$

If $\Phi(P, e) \neq \varnothing$, then it is a minimal cut. Otherwise, the transition along $e$ is said to produce no cut.

### 4.2. Implicit Graph

**Definition 8 (Search graph).** Define the directed graph $\mathcal{G} = (\mathbb{D}, \mathcal{E})$, where $(P, P') \in \mathcal{E}$ if and only if $\exists e \in P: \Phi(P, e) = P'$.

**Definition 9 (Root).** $P_0 = \Gamma^+(S^\*) = \{\, (S^\*, v) \in E \,\}$.

**Lemma 4.** $P_0 \in \mathbb{D}$.

**Proof.** $P_0$ is a multicut (every path from $S^\*$ begins with an edge from $P_0$). Removing any $(S^\*, v) \in P_0$ makes the path $S^\* \to v \leadsto T^\*$ free of edges of $P_0 \setminus \{(S^\*, v)\}$, hence $P_0$ is minimal by Theorem 1. $\square$

### 4.3. Monotonicity

**Lemma 5 (Monotonicity of $V_1$).** If $(A, B) \in \mathcal{E}$, then $V_1(A) \subset V_1(B)$ (strict inclusion).

**Proof.** Let $B = \Phi(A, e)$ for $e = (f, v) \in A$. Then $Q = \Psi(A, e) = (A \setminus \{e\}) \cup \Gamma^+(v)$. During the incremental construction of $V_1(Q)$, we add $v$ to $V_1(A)$. We show that $v \notin V_1(A)$. Suppose to the contrary that $v \in V_1(A)$. Then there exists a path $S^\* \leadsto v$ in $G \setminus A$. By Theorem 1, for $e$ there exists a free path $\pi$ containing $e$ and no other edges of $A$. Concatenating $S^\* \leadsto v$ (avoiding $A$) with the suffix of $\pi$ from $v$ to $T^\*$ (avoiding $A$) yields a path $S^\* \leadsto T^\*$ avoiding $A$, a contradiction. Hence $v \notin V_1(A)$. Since $v \in V_1(Q)$ and $V_1(Q) \subseteq V_1(B)$ (Clean does not shrink $V_1$: removing edges cannot reduce reachability), we obtain $V_1(A) \subset V_1(B)$. $\square$

**Corollary 2.** The graph $\mathcal{G}$ is a directed acyclic graph (DAG).

**Proof.** A cycle in $\mathcal{G}$ would imply a sequence of transitions returning to a cut with the same $V_1$, contradicting the strict increase of $V_1$ by Lemma 5. $\square$

---

## 5. Completeness

### 5.1. Direct Proof via Working Edges

**Theorem 3 (Completeness).** For any finite directed graph $G$, every minimal cut $P \in \mathbb{D}$ is reachable from $P_0$ in the search graph $\mathcal{G}$.

**Proof.**

Let $P \in \mathbb{D}$. By Theorem 1, for every $e \in P$ there exists a free path $\pi_e$. Fix one such path for each $e \in P$.

Consider the subgraph $\mathcal{F}$ consisting of all prefixes of these paths from $S^\*$ up to the first edge of $P$:

$$\mathcal{F} = \bigcup_{e \in P} \{\, \text{prefix of } \pi_e \text{ from } S^\* \text{ to } e \,\}.$$

Edges belonging to $\mathcal{F}$ are called *working edges*.

We construct a sequence of cuts $P_0, P_1, \ldots, P_k = P$, where each successive cut is obtained from the previous one by the operator $\Phi$. Define an invariant.

**Invariant.** For every $e \in P$, the current cut $P_i$ contains exactly one working edge on the path $\pi_e$, and this edge is located no farther than $e$ from $S^\*$ (by path length).

**Base.** $P_0 = \Gamma^+(S^\*)$. On every $\pi_e$, the first edge belongs to $P_0$; hence the invariant holds.

**Step.** Suppose $P_i$ satisfies the invariant and $P_i \neq P$. Then there exists $e_{curr} \in P_i \setminus P$. This edge lies on some free path $\pi_e$ for $e \in P$ and is strictly before $e$. Let $e_{next}$ be the next edge on $\pi_e$ after $e_{curr}$.

Apply $\Phi(P_i, e_{curr})$. Let $e_{curr} = (u, v)$. Then $\Psi(P_i, e_{curr}) = (P_i \setminus \{e_{curr}\}) \cup \Gamma^+(v)$.

$e_{next} \in \Gamma^+(v)$, hence $e_{next} \in \Psi(P_i, e_{curr})$.

During cleanup, $e_{next}$ will not be removed. Indeed, the suffix of $\pi_e$ from the end of $e_{next}$ to $T^\*$ contains no edges of $\Psi(P_i, e_{curr})$, because:

- it contains no edges of $P_i \setminus \{e_{curr}\}$ (by the invariant, $\pi_e$ contains no edges of $P_i$ other than $e_{curr}$);
- it contains no edges of $\Gamma^+(v)$ other than $e_{next}$ (the path is simple, so it does not return to $v$).

Hence $e_{next} \in P_{i+1}$.

For every other $e' \in P$, the corresponding working edge either remains in $P_{i+1}$ or is replaced by the next edge on $\pi_{e'}$. This occurs if the working edge was removed during cleanup; then another edge of the cut must appear on $\pi_{e'}$ (otherwise $\pi_{e'}$ would be free of $P_{i+1}$, contradicting that $P_{i+1}$ is a multicut). This new edge lies farther along $\pi_{e'}$ than the removed one. The invariant is preserved.

**Finiteness.** Each free path $\pi_e$ has finite length. Each branching advances at least one working edge strictly forward along its path. The total number of advances is bounded by $\sum_{e \in P} |\pi_e| \le |P| \cdot n$. Hence the sequence construction terminates.

**Completion.** When no advances remain, for every $e \in P$ the current cut contains exactly $e$. Therefore $P_k = P$.

Thus $P$ is reachable from $P_0$ in the search graph $\mathcal{G}$. $\square$

**Corollary 3 (Termination).** The APDFS algorithm, traversing $\mathcal{G}$ from $P_0$ with memoization, enumerates all minimal cuts and terminates on any finite directed graph.

**Proof.** By Corollary 2, $\mathcal{G}$ is a DAG with finitely many vertices. Memoization (`Seen`) guarantees that each vertex is visited at most once. Hence the traversal terminates. By Theorem 3, every vertex of $\mathcal{G}$ is reachable, so all minimal cuts are enumerated. $\square$

---

## 6. Hashing

Each edge $e \in E$ is assigned a 128-bit random number $h[e]$ generated by the deterministic Xoroshiro128+ generator with a fixed seed.

**Definition 10.** The hash of an edge set $P \subseteq E$ is defined as

$$H(P) = \bigoplus_{e \in P} h[e].$$

**Properties:**

1. **Commutativity:** $H(P)$ is independent of edge order.
2. **Incrementality:** $H(P \cup \{e\}) = H(P) \oplus h[e]$; $H(P \setminus \{e\}) = H(P) \oplus h[e]$.
3. **Collision probability:** For $P_1 \neq P_2$, $\Pr[H(P_1) = H(P_2)] = 2^{-128}$.

Deduplication is performed using a global hash table `Seen` with states `PROCESSING` and `DONE`. Hash equality means cut equality with error probability negligibly small for practical applications. The algorithm is probabilistic (Las Vegas type): it either produces the correct answer or (with probability $2^{-128}$) fails due to hash collisions.

---

## 7. Algorithm

### 7.1. Initialization

1. Construct the initial cut:
   $$P_0 \leftarrow \Gamma^+(S^\*).$$
2. Compute its hash:
   $$H_0 \leftarrow H(P_0).$$
3. Push the frame $(H_0, P_0, 0)$ onto thread 0's stack.

### 7.2. Frame Processing

**Input:** Frame $\mathcal{F} = (H, P, i)$, where $H$ is a hash, $P$ is a set of edges, and $i$ is an iterator index.

1. **Deduplication.** If $H$ is already present in the global hash table (in any state), terminate processing of this frame. Otherwise, insert $H$ with state `PROCESSING`.

2. **Recover $V_1$.** Perform forward BFS from $S^\*$ in the graph $G \setminus P$. Obtain:
   - $V_1$ — the set of vertices reachable from $S^\*$;
   - $P_{clean} = \{\, (u, v) \in E \mid u \in V_1,\; v \notin V_1 \,\}$.
   
   If $T^\* \in V_1$, then $P$ is not a multicut — abort with error.

3. **Output.** The set $P_{clean}$ is a minimal cut. Write it to the output.

4. **Branch.** For each index $j = i, \ldots, |P_{clean}| - 1$:
   
   4.1. Let $e_j = (u, v) \in P_{clean}$. If $v = T^\*$, skip this edge.
   
   4.2. Construct the candidate multicut:
   $$Q \leftarrow (P_{clean} \setminus \{e_j\}) \cup \Gamma^+(v).$$
   
   4.3. Perform incremental BFS from $v$ in $G \setminus Q$. Obtain $V_1^{new} \supseteq V_1$.
   
   4.4. If $T^\* \in V_1^{new}$, skip ( $Q$ is not a multicut).
   
   4.5. Clean up:
   $$P_{new} \leftarrow \text{Clean}(Q, V_1^{new}).$$
   
   4.6. If $P_{new} = \varnothing$, skip.
   
   4.7. Compute the hash:
   $$H_{new} \leftarrow H(P_{new}).$$
   
   4.8. If $H_{new}$ is in the hash table, skip (duplicate).
   
   4.9. Push the frame $(H_{new}, P_{new}, 0)$ onto the current thread's stack.

5. **Finalize.** Update the state of $H$ in the hash table to `DONE`.

### 7.3. Parallel Traversal

The algorithm employs work-stealing parallel depth-first search:

- Each of the $P$ threads maintains a local stack of frames.
- When a thread's stack becomes empty, it attempts to steal the **lower half** of a randomly selected thread's stack.
- When a thread's stack exceeds a threshold, the **lower half** is moved to a global queue.
- Termination occurs when all local stacks are empty and the global queue is empty.

### 7.4. Hash Table

The global hash table stores hashes of processed cuts. Each entry is in one of two states:

- `PROCESSING` — the cut is currently being processed by some thread;
- `DONE` — the cut has been fully processed and all its branches have been explored.

A frame is skipped if its hash is already present in the table (in either state).

---

## 8. Complexity

**Theorem 4 (Time per frame).** Processing a frame requires $O(|P| \cdot (n + m))$ operations.

**Proof.** Recovering $V_1$ (forward BFS): $O(n + m)$. For each of the $|P|$ edges: incremental BFS in $O(n + m)$ and Clean (one reverse BFS) in $O(n + m)$. Total: $O(|P| \cdot (n + m))$. $\square$

**Corollary 4 (Total time).** $O(|\mathbb{D}| \cdot \bar{p} \cdot (n + m) / P)$, where $\bar{p}$ is the average cut size and $P$ is the number of threads. In practice, parallel efficiency depends on synchronization overhead and load balancing.

**Theorem 5 (Memory).** The algorithm requires $O(|\mathbb{D}|)$ memory for the global hash table, plus $O(n + m + L \cdot \bar{p})$ per thread, where $L$ is the maximum local stack size and $\bar{p}$ is the average cut size. The hash occupies 16 bytes; with hash-table overhead, actual consumption may be higher.

---

## 9. Applications and Extensions

### 9.1. Application Areas

The APDFS algorithm is intended for problems requiring complete structural enumeration of all minimal $(S,T)$-cuts. Unlike flow algorithms, which find only minimum-weight cuts, APDFS enumerates all inclusion-minimal cuts.

**Reliability Engineering.** Minimal cuts of a reliability graph are minimal cut sets. APDFS provides an enumeration method that does not require prior construction of BDDs and supports streaming generation of cut sets with controlled memory consumption.

**Network Policy Verification.** In SDN and firewall systems, a minimal cut corresponds to the minimal set of rules whose removal breaks isolation between network segments. Completeness of enumeration guarantees that no isolation-breaking method is missed.

**Bioinformatics (metabolic networks).** In metabolic pathways, a minimal cut defines the set of reactions (edges) whose blocking halts synthesis of a target product. Structural analysis requires no kinetic parameters.

**Stochastic Sampling.** The APDFS branching operator naturally defines a neighborhood relation on $\mathbb{D}$, enabling the construction of Markov chains (MCMC) for sampling cuts without full enumeration.

### 9.2. Extension 1: Enumeration of Size-Bounded Cuts

**Problem.** Find all minimal cuts whose cardinality does not exceed a given $K$.

**Method.** An upper size bound is introduced. For a multicut $Q$ before cleanup, an upper bound on the size of any minimal cut $R \subseteq Q$ is known: $|R| \le |Q|$. Pruning rule:

- If $|Q| \le K$, then any $R \subseteq Q$ satisfies the bound — the frame is generated unconditionally.
- If $|Q| > K$, the frame is generated as *speculative*: after cleanup, $|R|$ may decrease. If $|R| \le K$, the cut is kept; otherwise it is discarded.

**Correctness.** All minimal cuts of size $\le K$ are found. Only branches whose size exceeds $K$ even after cleanup are pruned. The number of wasted BFS runs is reduced by early pruning on $|Q|$.

### 9.3. Extension 2: K-Best by Number of Edges

**Problem.** Find the $K$ minimal cuts with the fewest edges.

**Method.** Iterative application of Extension 1 with increasing size bound: for $s = 1, 2, 3, \ldots$, run Extension 1 with $K_{max} = s$. Cuts are extracted in nondecreasing size order.

**Remark.** This extension is inapplicable for K-best under arbitrary weights. The weighted K-best problem for inclusion-minimal cuts requires an external constrained minimum-cut solver.

### 9.4. Extension 3: Pareto-Optimal Cuts

**Problem.** Given $d \ge 1$ weight functions $w_1, \ldots, w_d: E \to \mathbb{R}_{\ge 0}$, find all Pareto-optimal minimal cuts.

**Method.** A criterion vector is added to `Seen`. During duplicate checking, Pareto dominance is also checked: if an already found cut dominates the new one on all criteria, the new one is discarded. If the new one dominates an existing one, the existing one is replaced. If the vectors are incomparable, both are kept.

**Correctness.** The algorithm performs a complete traversal of $\mathcal{G}$. For every Pareto-optimal cut $P^\*$, when generated, it is either absent from `Seen` or already present as $P'$ with the same hash. If $W(P') \prec W(P^\*)$, this contradicts the Pareto optimality of $P^\*$. If $W(P^\*) \prec W(P')$, then $P^\*$ replaces $P'$. Completeness of traversal guarantees finding all Pareto-optimal cuts.

**Implementation note.** When replacing an existing cut with a new one, it is necessary to account for the fact that descendants of the old cut may already have been processed. Additional synchronization is required for correct handling of such cases.

### 9.5. Extension 4: Incremental Update

**Problem.** When an edge is added to or removed from a graph, update the set of minimal cuts without full recomputation.

**Definition 11 (Affected cuts).** A cut $P \in \mathbb{D}(G)$ is called *affected* by a change $e^\*$ if:

- $e^\*$ is added, and there exists a path $S^\* \leadsto T^\*$ in $G'$ containing $e^\*$ and no other edges of $P$;
- $e^\*$ is removed, and $e^\* \in P$.

**Lemma 7.** If $P \in \mathbb{D}(G)$ is not affected by a change $e^\*$, then $P \in \mathbb{D}(G')$.

**Incremental update algorithm:**

1. Identify affected cuts $Z \subseteq \mathbb{D}(G)$.
2. $\mathbb{D}_{keep} \leftarrow \mathbb{D}(G) \setminus Z$ (by Lemma 7, they are preserved).
3. For each $P \in Z$, if $P$ is still a minimal cut in $G'$, add it to $\mathbb{D}_{keep}$.
4. Run APDFS on $G'$ with `Seen` initialized to $\mathbb{D}_{keep}$ and with additional roots from $Z$.

### 9.6. Extension 5: Stochastic Sampling (MCMC)

**Problem.** Construct a Markov chain on the space $\mathbb{D}$ with stationary distribution $\pi(P)$.

**Definition 12 (Neighborhood relation).** Two cuts $P, P' \in \mathbb{D}$ are called *neighbors* if $\exists e \in P: \Phi(P, e) = P'$ or $\exists e' \in P': \Phi(P', e') = P$.

**Lemma 8.** The neighborhood relation is symmetric and connected on $\mathbb{D}$.

**MCMC sampler algorithm:**

1. Start with $P \leftarrow P_0$.
2. At each step:
   - Choose a random edge $e \in P$ uniformly.
   - Compute $P' = \Phi(P, e)$. If $\Phi(P, e) = \varnothing$, stay at $P$.
   - Compute $n(P, P') = |\{ e \in P \mid \Phi(P, e) = P' \}|$.
   - Compute $n(P', P) = |\{ e' \in P' \mid \Phi(P', e') = P \}|$.
   - Accept the transition $P \to P'$ with probability
     $$\alpha = \min\left(1, \frac{\pi(P')}{\pi(P)} \cdot \frac{n(P', P) \cdot |P|}{n(P, P') \cdot |P'|}\right).$$
3. Repeat, recording states.

**Theorem 6 (MCMC correctness).** The constructed chain is irreducible and aperiodic on $\mathbb{D}$ with stationary distribution $\pi(P)$.

**Complexity note.** Computing $n(P, P')$ and $n(P', P)$ requires iterating over all edges of $P$ and $P'$ and computing $\Phi$ for each, costing $O(|P| \cdot (n + m))$ operations per step. For large graphs this may be expensive; caching of transitions is a possible optimization.

---

## References

1. Tsukiyama, S., Ide, M., Ariyoshi, H., & Shirakawa, I. (1980). A new algorithm for generating all the minimal cuts in a directed graph. *Electronics and Communications in Japan*, 63(1), 1-8.
2. Picard, J. C., & Queyranne, M. (1980). On the structure of all minimum cuts in a network and applications. *Mathematical Programming Study*, 13, 8-16.
3. Lawler, E. L. (1972). A procedure for computing the K best solutions to discrete optimization problems and its application to the shortest path problem. *Management Science*, 18(7), 401-405.
4. Karger, D. R. (1993). Global min-cuts in RNC, and other ramifications of a simple min-cut algorithm. *Proceedings of SODA '93*, 21-30.