# Enumerating All Minimal (S,T)-Cuts in Directed Graphs: Completeness, Correctness, and Termination of the APDFS Algorithm

**Abstract.** We study the problem of enumerating the family $\mathbb{D}$ of all inclusion-minimal
$(S^\*,T^\*)$-cuts of a finite directed graph. Unlike minimum-cut problems, which ask for a single
optimal solution and are solved by flow algorithms, the task here is to list every minimal multicut,
a family that can be exponentially large. We present APDFS, a depth-first enumeration algorithm
whose state is a minimal cut together with its closed vertex set $V_1$, and whose transition replaces
one cut edge by the outgoing edges of its head and then reduces the resulting multicut to the unique
minimal cut it contains. We prove that the reduction is well defined and unique (Theorem 2 and
Corollary 1), that $V_1$ strictly increases along every transition (Lemma 5, Corollary 2), that every
minimal cut is reachable from the initial cut (Theorem 3), and that traversal with memoisation
terminates on every finite graph (Corollary 3). The algorithm has polynomial delay
$O(\bar p\,(n+m))$ and output-polynomial total time; deduplication uses 128-bit XOR fingerprints and
is correct up to one-sided error $2^{-128}$. We describe a parallel implementation based on
work stealing, and five extensions: size-bounded enumeration, $K$-best cuts by cardinality,
Pareto-optimal cuts, incremental recomputation after an edge update, and Markov-chain sampling.

**Keywords:** minimal cut, multicut, enumeration, polynomial delay, depth-first search, work stealing,
reliability, metabolic networks.

---

## 1. Introduction

A minimal $(S,T)$-cut of a directed graph is an edge set that meets every path from $S$ to $T$ and
contains no proper subset with the same property. Such sets arise whenever one asks for the *structural
fragilities* of a network: in reliability engineering they are the minimal cut sets whose failure
disconnects the network; in network-policy verification they are the minimal sets of rules whose
removal breaks a separation; in metabolic-network analysis they are the minimal sets of reactions
whose knock-out blocks the synthesis of a target metabolite [5]. In each case the requirement is
completeness — no cut set may be missed — rather than optimality.

This is not the minimum-cut problem. Flow algorithms and the lattice representation of
Picard and Queyranne [2] answer questions about *minimum-weight* cuts and their structure; the
inclusion-minimal cuts form a much larger family, and no flow computation lists them. The relevant
precedent is the enumeration literature: Tsukiyama, Ide, Ariyoshi and Shirakawa [1] gave the first
polynomial-delay algorithm for minimal cuts of a directed graph, and Provan and Shier [4] unified
listing algorithms for $(S,T)$-cuts under a common framework. In contrast to those approaches, the
algorithm studied here abandons a global canonical ordering in favour of a local transition operator
and a hash-based seen set; this trade makes the search easy to distribute over threads and to stream
to disk, at the cost of $O(|\mathbb{D}|)$ memory for the seen set and a constant number of rejected
duplicates per emitted cut.

**Contributions.** (i) A self-contained proof that the cleanup operator of Section 4 returns the
unique minimal cut contained in a multicut (Theorem 2, Corollary 1). (ii) A proof that the reachable
vertex set is a strictly increasing potential on the search graph, hence that the graph is acyclic
(Lemma 5, Corollary 2). (iii) A completeness proof by a monotone "working edges" invariant
(Theorem 3) and a termination corollary (Corollary 3). (iv) A precise account of the one-sided-error
nature of fingerprint deduplication (Section 7). (v) The sequential and parallel algorithms
(Section 8), their cost (Section 9), and five extensions (Section 10).

**Related work.** Listing all minimal cuts with polynomial delay was introduced in [1]; [4] gives a
paradigm covering $(S,T)$-cuts and related families. Minimum cuts and all *minimum* cuts are treated
in [2], and global minimum cuts in [3]. The $K$-best extension follows Lawler's partitioning scheme
[6]. Extensions to metabolic networks are discussed in [5].

---

## 2. Preliminaries

### 2.1. Graphs and paths

Let a directed graph be given as $G = (V, E)$, with $|V| = n$ and $|E| = m$. Two distinguished
vertices are introduced:

- $S^\*$ — the supersource ($\deg_{in}(S^\*) = 0$ in $G$);
- $T^\*$ — the supersink ($\deg_{out}(T^\*) = 0$ in $G$).

Multiple sources $\{s_i\}$ and sinks $\{t_j\}$ are reduced to $S^\*$ and $T^\*$ by adding dummy edges
$(S^\*, s_i)$ and $(t_j, T^\*)$. Such edges cannot belong to a minimal $(S^\*, T^\*)$-cut except in
the trivial case, and are filtered out in postprocessing if necessary. The graph $G$ may contain
cycles; preliminary contraction of strongly connected components is not required.

**Standing assumption (trimmed graph).** Unless stated otherwise we assume that every vertex and every
edge of $G$ lies on at least one $S^\*$–$T^\*$ path; in particular, every out-neighbour of $S^\*$
reaches $T^\*$ and every in-neighbour of $T^\*$ is reachable from $S^\*$. This is the standard
reduction for cut enumeration (vertices and edges that lie on no $S^\*$–$T^\*$ path can never occur in
a minimal cut). An implementation enforces it with a reachability precondition and reports the empty
family when no $S^\*$–$T^\*$ path exists. Without this assumption the initial cut of Definition 9 is
not minimal in general and the empty family needs separate handling.

**Definition 1 (Path).** A directed path from $u$ to $v$ is a sequence of vertices
$\pi = \langle u = x_0, x_1, \ldots, x_k = v \rangle$ such that $(x_i, x_{i+1}) \in E$ for every
$i \in \{0, \ldots, k-1\}$ and all $x_i$ are pairwise distinct. The number $k$ is the length of the
path.

### 2.2. Cuts

**Definition 2 (Multicut).** A set of edges $Q \subseteq E$ is an $(S^\*, T^\*)$-multicut if every
path from $S^\*$ to $T^\*$ contains at least one edge of $Q$.

**Definition 3 (Minimal cut).** A multicut $P$ is a minimal cut if no proper subset
$P' \subset P$ is a multicut. The set of all minimal cuts of $G$ is denoted by $\mathbb{D}$.

**Definition 4 (Free path).** Let $P \in \mathbb{D}$ and $e \in P$. A path $\pi$ from $S^\*$ to
$T^\*$ is free for $e$ with respect to $P$ if $e \in \pi$ and $\pi \cap P = \{e\}$.

**Theorem 1 (Minimality criterion).** A multicut $P$ is a minimal cut if and only if for every
$e \in P$ there exists a free path with respect to $P$.

**Proof.**

$(\Rightarrow)$ Let $P \in \mathbb{D}$. Suppose that for some $e \in P$ no free path exists. Set
$P' = P \setminus \{e\}$. Consider an arbitrary path $\pi$ from $S^\*$ to $T^\*$. If $e \notin \pi$,
then $\pi$ contains an edge from $P' \subseteq P$. If $e \in \pi$, then by assumption $\pi$ contains
some other edge $e' \in P$, $e' \neq e$. Hence $e' \in P'$. In both cases $\pi \cap P' \neq
\varnothing$, so $P'$ is a multicut, contradicting the minimality of $P$.

$(\Leftarrow)$ Suppose that for every $e \in P$ there exists a free path $\pi_e$. Remove an arbitrary
edge $e \in P$. The path $\pi_e$ contains no other edges of $P$, hence
$\pi_e \cap (P \setminus \{e\}) = \varnothing$. Therefore $P \setminus \{e\}$ is not a multicut.
Since $e$ was arbitrary, $P$ is minimal. $\square$

### 2.3. Closed vertex sets

**Definition 5 (Closed set).** For a multicut $Q$, the closed vertex set is

$$V_1(Q) = \{\, v \in V \cup \{S^\*, T^\*} \mid \exists \text{ a path } S^\* \leadsto v \text{ in }
G \setminus Q \,\}.$$

**Lemma 1 (Properties of $V_1$).** Let $Q$ be a multicut. Then:

1. $S^\* \in V_1(Q)$.
2. If $u \in V_1(Q)$ and there is a path $u \leadsto v$ in $G \setminus Q$, then $v \in V_1(Q)$.
3. $Q$ is a multicut $\iff$ $T^\* \notin V_1(Q)$.
4. If $P \in \mathbb{D}$, then
   $P = \{\, (u, v) \in E \mid u \in V_1(P),\; v \notin V_1(P) \,\}$.

**Proof.**

1. The zero-length path from $S^\*$ to $S^\*$ contains no edges of $Q$; hence $S^\* \in V_1(Q)$.
2. Concatenating a path $S^\* \leadsto u$ in $G \setminus Q$ with a path $u \leadsto v$ in
   $G \setminus Q$ yields a path $S^\* \leadsto v$ in $G \setminus Q$, so $v \in V_1(Q)$.
3. If $T^\* \in V_1(Q)$, there is a path $S^\* \leadsto T^\*$ avoiding $Q$, so $Q$ is not a
   multicut. Conversely, if $Q$ is not a multicut, such a path exists, so $T^\* \in V_1(Q)$.
4. Let $B = \{\, (u, v) \in E \mid u \in V_1(P),\, v \notin V_1(P) \,\}$. Every path
   $S^\* \leadsto T^\*$ crosses the boundary of $V_1(P)$, so $B$ is a multicut. Any edge
   $(u, v) \in P$ satisfies $u \in V_1(P)$ (otherwise the edge is unreachable in $G \setminus P$ and
   $P \setminus \{(u,v)\}$ is a multicut) and $v \notin V_1(P)$ (otherwise
   $P \setminus \{(u,v)\}$ is a multicut). Hence $P \subseteq B$, and by minimality $P = B$.
   $\square$

---

## 3. Branching operation

**Definition 6 (Branching operation).** Let $P \in \mathbb{D}$ and $e = (f, v) \in P$, where
$v \neq T^\*$. Then

$$\Psi(P, e) = (P \setminus \{e\}) \cup \Gamma^+(v),$$

where $\Gamma^+(v) = \{\, (v, w) \in E \,\}$ is the set of outgoing edges of $v$.

**Remark 1.** The case $v = T^\*$ is excluded because $\Gamma^+(T^\*) = \varnothing$ and
$P \setminus \{e\}$ is not a multicut (the free path for $e$ remains uncovered). Branching on edges
leading to the sink is therefore never performed.

**Lemma 2.** For any $P \in \mathbb{D}$ and $e = (f, v) \in P$ with $v \neq T^\*$, the set
$\Psi(P, e)$ is a multicut.

**Proof.** Let $\pi$ be an arbitrary path $S^\* \leadsto T^\*$. If $e \notin \pi$, then $\pi$ contains
an edge of $P \setminus \{e\} \subseteq \Psi(P, e)$. If $e \in \pi$, then
$\pi = \ldots \to f \xrightarrow{e} v \to \ldots$ and after $v$ the path continues along an edge
$(v, w) \in \Gamma^+(v) \subseteq \Psi(P, e)$. In both cases
$\pi \cap \Psi(P, e) \neq \varnothing$. $\square$

A cut may contain several edges with the same head. Such edges generate the same successor, which
justifies branching once per distinct head rather than once per edge.

**Proposition 1.** Let $P \in \mathbb{D}$ and let $e_1 = (f_1, v), e_2 = (f_2, v) \in P$ with
$v \neq T^\*$. Then $\Phi(P, e_1) = \Phi(P, e_2)$.

**Proof.** Put $Q_i = \Psi(P, e_i)$ for $i = 1, 2$. Branching on $e_i$ un-forbids $e_i$ and forbids
$\Gamma^+(v)$, and no other boundary edge of $V_1(P)$ changes; hence $V_1(Q_1) = V_1(P) \cup \{v\} =
V_1(Q_2)$, and $v$ has no outgoing edge in either graph. For any $w \notin V_1(P) \cup \{v\}$, a path
$w \leadsto T^\*$ in $G \setminus Q_i$ cannot visit $v$ (it would have to leave $v$ through
$\Gamma^+(v) \subseteq Q_i$), so $V_T(Q_1) = V_T(Q_2)$. The two sets differ only in $e_1$ and $e_2$,
both of which join $V_1(P)$ to $v$ and are therefore internal to $V_1(Q_i)$; they are removed by the
cleanup of Section 4, and all remaining edges coincide. Hence the two candidate multicuts reduce to
the same minimal cut. $\square$

---

## 4. Cleanup procedure

### 4.1. Description

**Input:** a multicut $Q$ and the set $V_1 = V_1(Q)$.

**Output:** a minimal cut $P \subseteq Q$, or $\varnothing$ if $Q$ contains no minimal cut.

**Boundary hypothesis.** Throughout this section we require that $Q$ is a *boundary* multicut, that is
$Q = \delta^+(V_1)$: every edge $(u,w) \in Q$ has $u \in V_1$ and $w \notin V_1$. This is the only
form in which Clean is ever invoked — by the construction of Section 8.2 the candidate
$\Psi(P,e)$ is always a boundary multicut, because its edges are either cut edges of $P$ (whose heads
lie outside $V_1(P) \subseteq V_1(Q)$) or outgoing edges of $v$ (whose heads lie outside $V_1(Q)$).
For an arbitrary multicut the statements of this section can fail; see the remark after Corollary 1.

**Algorithm Clean$(Q, V_1)$:**

1. Construct $V_T$, the set of vertices from which $T^\*$ is reachable without using edges of $Q$:
   $$V_T = \{\, v \in V \cup \{T^\*\} \mid \exists \text{ a path } v \leadsto T^\* \text{ in }
   G \setminus Q \,\}.$$
   The set is obtained by one reverse breadth-first search from $T^\*$ along reversed edges.
2. Set $P = \{\, e = (u, w) \in Q \mid u \in V_1 \text{ and } w \in V_T \,\}$.
3. Return $P$.

### 4.2. Correctness

**Theorem 2.** Let $Q$ be a boundary multicut, $V_1 = V_1(Q)$, $V_T$ be constructed as above, and
$P = \text{Clean}(Q, V_1)$. Then:

1. If $P = \varnothing$, then $Q$ contains no minimal cut.
2. If $P \neq \varnothing$, then $P$ is a minimal cut and $P \subseteq Q$.

**Proof.** We compare Clean with the elimination procedure $\text{Reduce}(R)$, which iterates over
the edges of $R$ and removes an edge $e$ whenever $R \setminus \{e\}$ is still a multicut. It is
well known that $\text{Reduce}(R)$ returns a minimal cut $P \subseteq R$ when $R$ is a multicut.

**Lemma 3 (Characterization of redundant edges).** Under the boundary hypothesis, an edge
$e' = (a, b) \in Q$ is redundant (i.e. $Q \setminus \{e'\}$ is a multicut) if and only if
$b \notin V_T$.

**Proof of Lemma 3.**

$(\Rightarrow)$ Suppose $e'$ is redundant and put $R = Q \setminus \{e'\}$. Assume $b \in V_T$. By
definition of $V_T$ there is a path $\pi_b: b \leadsto T^\*$ in $G \setminus Q$. Since $a \in V_1$,
there is a path $\pi_a: S^\* \leadsto a$ in $G \setminus Q$. Then
$\pi = \pi_a \circ (a, b) \circ \pi_b$ is a path $S^\* \leadsto T^\*$ containing $e'$ and no other
edge of $Q$, hence $\pi \cap R = \varnothing$, contradicting that $R$ is a multicut. Therefore
$b \notin V_T$.

$(\Leftarrow)$ Suppose $b \notin V_T$. We show that $e'$ is redundant. Let $\pi$ be an arbitrary path
$S^\* \leadsto T^\*$. If $e' \notin \pi$, then $\pi$ contains an edge of $Q \setminus \{e'\}$, since
$Q$ is a multicut. If $e' \in \pi$, write $\pi = \pi_{pre} \circ (a, b) \circ \pi_{suf}$. Since
$b \notin V_T$, every path $b \leadsto T^\*$ contains an edge of $Q$; in particular $\pi_{suf}$
contains $e'' \in Q$ with $e'' \neq e'$ (the path is simple, so $b$ appears exactly once). Hence
$\pi \cap (Q \setminus \{e'\}) \neq \varnothing$, and $Q \setminus \{e'\}$ is a multicut. $\square$

By Lemma 3 the set of non-redundant edges of $Q$ is exactly
$\{\, (a, b) \in Q \mid b \in V_T \,\}$. By Lemma 1.4 every non-redundant edge of $Q$ has
$a \in V_1$, so this set coincides with the output of Clean. The elimination procedure, removing
redundant edges in any order, leaves exactly the same set. Therefore
$\text{Clean}(Q, V_1) = \text{Reduce}(Q)$, which is a minimal cut. $\square$

**Corollary 1 (Uniqueness).** Any boundary multicut $Q$ contains at most one minimal cut.

**Proof.** Clean deterministically computes a set $P$. By Theorem 2 every minimal cut
$R \subseteq Q$ coincides with $P$. $\square$

**Remark (necessity of the boundary hypothesis).** The hypothesis is essential. Take the path
$S^\* \to a \to b \to T^\*$ and the multicut $Q = \{S^\*a, ab\}$. Here $V_1(Q) = \{S^\*\}$ and
$V_T(Q) = \{b, T^\*\}$, so $\text{Clean}(Q) = \varnothing$, yet $Q$ contains *two* minimal cuts,
$\{S^\*a\}$ and $\{ab\}$. The edge $ab$ has its tail outside $V_1(Q)$, so the characterisation of
Lemma 3 does not apply to it. Clean is only guaranteed to recover the unique minimal cut for boundary
multicuts, which is precisely the family produced by $\Psi$.

---

## 5. Search graph

### 5.1. Transition operator

**Definition 7 (Transition operator).** For $P \in \mathbb{D}$ and $e = (f, v) \in P$ with
$v \neq T^\*$,

$$\Phi(P, e) = \text{Clean}(\Psi(P, e), V_1(\Psi(P, e))).$$

If $\Phi(P, e) \neq \varnothing$ it is a minimal cut by Theorem 2; otherwise the transition along $e$
produces no cut.

### 5.2. Implicit graph

**Definition 8 (Search graph).** Let $\mathcal{G} = (\mathbb{D}, \mathcal{E})$, where
$(P, P') \in \mathcal{E}$ if and only if $\Phi(P, e) = P'$ for some $e \in P$.

**Definition 9 (Root).** $P_0 = \Gamma^+(S^\*) = \{\, (S^\*, v) \in E \,\}$.

**Lemma 4.** Under the standing assumption of Section 2.1, $P_0 \in \mathbb{D}$.

**Proof.** $P_0$ is a multicut, since every path from $S^\*$ begins with an edge of $P_0$. Removing
$(S^\*, v) \in P_0$ leaves the path $S^\* \to v \leadsto T^\*$ free of edges of
$P_0 \setminus \{(S^\*, v)\}$; the suffix $v \leadsto T^\*$ exists by the standing assumption (every
out-neighbour of $S^\*$ reaches $T^\*$). Hence $P_0$ is minimal by Theorem 1. $\square$

### 5.3. Monotonicity

**Lemma 5 (Monotonicity of $V_1$).** If $(A, B) \in \mathcal{E}$, then
$V_1(A) \subset V_1(B)$ strictly.

**Proof.** Let $B = \Phi(A, e)$ for $e = (f, v) \in A$ and let
$Q = \Psi(A, e) = (A \setminus \{e\}) \cup \Gamma^+(v)$. During the incremental construction of
$V_1(Q)$ the vertex $v$ is added to $V_1(A)$. We show that $v \notin V_1(A)$. Otherwise there is a
path $S^\* \leadsto v$ in $G \setminus A$; by Theorem 1 there is a free path $\pi$ for $e$ containing
$e$ and no other edge of $A$. Concatenating $S^\* \leadsto v$ (avoiding $A$) with the suffix of $\pi$
from $v$ to $T^\*$ (avoiding $A$) yields a path $S^\* \leadsto T^\*$ avoiding $A$, a contradiction.
Hence $v \notin V_1(A)$. Since $v \in V_1(Q) \subseteq V_1(B)$ (cleanup only removes edges, which
cannot reduce reachability), we obtain $V_1(A) \subset V_1(B)$. $\square$

**Corollary 2.** The graph $\mathcal{G}$ is a directed acyclic graph.

**Proof.** A cycle in $\mathcal{G}$ would return to a cut with the same $V_1$, contradicting the
strict increase of Lemma 5. $\square$

---

## 6. Completeness and termination

**Theorem 3 (Completeness).** For any finite directed graph $G$, every minimal cut
$P \in \mathbb{D}$ is reachable from $P_0$ in the search graph $\mathcal{G}$.

**Proof.** Let $P \in \mathbb{D}$. By Theorem 1, for every $e \in P$ there is a free path $\pi_e$;
fix one such path for each $e$. Consider the subgraph

$$\mathcal{F} = \bigcup_{e \in P} \{\, \text{prefix of } \pi_e \text{ from } S^\* \text{ to } e \,\},$$

whose edges are called *working edges*. We construct a sequence $P_0, P_1, \ldots, P_k = P$ in which
each cut is obtained from the previous one by $\Phi$, and maintain the invariant: for every
$e \in P$, the current cut $P_i$ contains exactly one working edge on $\pi_e$, and this edge is
located no farther than $e$ from $S^\*$ by path length.

*Base.* $P_0 = \Gamma^+(S^\*)$; the first edge of every $\pi_e$ belongs to $P_0$, so the invariant
holds.

*Step.* Suppose $P_i$ satisfies the invariant and $P_i \neq P$. Then $P_i \setminus P \neq
\varnothing$; choose $e_{curr} \in P_i \setminus P$. This edge lies on some free path $\pi_e$ for
$e \in P$ and is strictly before $e$; let $e_{next}$ be the next edge on $\pi_e$ after $e_{curr}$.
Write $e_{curr} = (u, v)$ and apply $\Phi(P_i, e_{curr})$: then
$\Psi(P_i, e_{curr}) = (P_i \setminus \{e_{curr}\}) \cup \Gamma^+(v)$, and $e_{next} \in \Gamma^+(v)$
belongs to the candidate. The edge $e_{next}$ survives the cleanup, because the suffix of $\pi_e$
from the end of $e_{next}$ to $T^\*$ contains neither an edge of $P_i \setminus \{e_{curr}\}$ (by the
invariant, $\pi_e$ contains no other edge of $P_i$) nor an edge of $\Gamma^+(v)$ other than
$e_{next}$ (the path is simple and does not return to $v$). Hence $e_{next} \in P_{i+1}$. For every
other $e' \in P$ the corresponding working edge either remains in $P_{i+1}$ or is replaced by the
next edge on $\pi_{e'}$; the latter happens only if the working edge was removed by the cleanup, in
which case another edge of the cut must appear on $\pi_{e'}$ (otherwise $\pi_{e'}$ would be free of
$P_{i+1}$, contradicting that $P_{i+1}$ is a multicut), and that edge lies farther along
$\pi_{e'}$. The invariant is preserved.

*Finiteness.* Every $\pi_e$ has finite length and each branching advances at least one working edge
strictly forward, so the number of advances is bounded by
$\sum_{e \in P} |\pi_e| \le |P| \cdot n$ and the construction terminates. When no advance is
possible, the current cut contains exactly $e$ for every $e \in P$, hence $P_k = P$.

Thus $P$ is reachable from $P_0$ in $\mathcal{G}$. $\square$

**Remark (status of the proof).** The argument above is an outline: the invariant as stated bounds only
the *working* edges (one per chosen free path $\pi_e$) and does not by itself justify the step that
picks $e_{\mathrm{curr}} \in P_i \setminus P$ and asserts that it lies on some $\pi_e$ before $e$, nor
that the intersection $P_i \cap \pi_e$ remains a single edge after a cleanup. A fully rigorous version
should additionally prove that every edge of a reachable cut lies on one of the chosen free paths
(equivalently $P_i \subseteq \bigcup_e \pi_e$) and that the working edge advances monotonically.
Completeness is independently corroborated in practice by an exhaustive oracle over small graphs
(Section 8 of the accompanying implementation notes); we state the theorem but flag the proof as
requiring completion.

**Corollary 3 (Termination).** APDFS, traversing $\mathcal{G}$ from $P_0$ with memoisation,
enumerates all minimal cuts and terminates on every finite directed graph.

**Proof.** By Corollary 2, $\mathcal{G}$ is a DAG with finitely many vertices, and memoisation
guarantees that each vertex is visited at most once. Hence the traversal terminates, and by
Theorem 3 it reaches every vertex. $\square$

---

## 7. Fingerprinting

Each edge $e \in E$ receives a 128-bit label $h[e]$ produced by the deterministic Xoroshiro128
generator with a fixed seed.

**Definition 10.** The fingerprint of an edge set $P \subseteq E$ is

$$H(P) = \bigoplus_{e \in P} h[e].$$

**Properties.**

1. *Commutativity*: $H(P)$ does not depend on the order of edges.
2. *Incrementality*: $H(P \cup \{e\}) = H(P) \oplus h[e]$ and $H(P \setminus \{e\}) = H(P) \oplus h[e]$.
3. *Collision probability*: for $P_1 \neq P_2$, $\Pr[H(P_1) = H(P_2)] = 2^{-128}$ under independent
   uniform labels.

Deduplication uses a global hash set of fingerprints; a fingerprint is inserted when a frame is
generated and never changes. The failure mode is one-sided: a collision can only cause the algorithm
to *skip* a genuine cut and can never emit a cut that is not minimal. Since a run emits at most
$|\mathbb{D}|$ cuts, the probability of missing one is at most
$\binom{|\mathbb{D}|}{2} 2^{-128} < |\mathbb{D}|^2 2^{-129}$.

**Remark (determinism and linearity).** The bound above models the labels $h[e]$ as independent
uniform random values. The implementation instead draws them from a fixed-seed Xoroshiro128 generator,
so the guarantee is heuristic rather than adversarial: it holds for the graphs the generator produces
but an adversary who knows the seed could in principle construct a collision. In addition, the
fingerprint is *linear* over $\mathrm{GF}(2)^{128}$: for $P_1 \neq P_2$ a collision means exactly that
the nonempty symmetric difference $P_1 \triangle P_2$ has labels summing to zero under $\oplus$. Both
observations follow from the choice of $\oplus$ and of the label source, and neither changes the
one-sided nature of the failure mode.

---

## 8. The algorithm

### 8.1. Initialization

1. Construct $P_0 \leftarrow \Gamma^+(S^\*)$ and $H_0 \leftarrow H(P_0)$.
2. Push the frame $(H_0, P_0)$ onto the stack of thread 0.

### 8.2. Frame processing

**Input:** a frame $(H, P)$.

1. **Deduplication.** If $H$ is present in the global hash set, stop processing this frame;
   otherwise insert $H$.
2. **Recover $V_1$.** Run a forward BFS from $S^\*$ in $G \setminus P$ and obtain $V_1$ together with
   $P_{clean} = \{\, (u, v) \in E \mid u \in V_1,\; v \notin V_1 \,\}$. If $T^\* \in V_1$ then $P$ is
   not a multicut and the frame is discarded.
3. **Output.** $P_{clean}$ is a minimal cut (Lemma 1.4) and is written to the output stream.
4. **Branch.** For every distinct head $v$ of an edge of $P_{clean}$ with $v \neq T^\*$
   (Proposition 1 allows skipping repeated heads):
   1. $Q \leftarrow (P_{clean} \setminus \{e\}) \cup \Gamma^+(v)$, where $e$ is any edge of
      $P_{clean}$ with head $v$.
   2. Run an incremental forward BFS from $v$ in $G \setminus Q$; obtain $V_1^{new} \supseteq V_1$
      and discard the branch if $T^\* \in V_1^{new}$.
   3. $P_{new} \leftarrow \text{Clean}(Q, V_1^{new})$; discard the branch if $P_{new} = \varnothing$.
   4. $H_{new} \leftarrow H(P_{new})$; discard the branch if $H_{new}$ is already in the hash set.
   5. Push $(H_{new}, P_{new})$ onto the current thread's stack.
5. **Finalize.** The fingerprint $H$ remains in the hash set as seen.

### 8.3. Parallel traversal

The search is a work-stealing parallel depth-first search.

- Each of the $T$ threads maintains a local LIFO stack of frames.
- When a stack becomes empty, the thread attempts to steal the lower half of a randomly selected
  thread's stack (at most two attempts).
- When a local stack exceeds a threshold, its lower half is moved to a shared spill buffer.
- Termination is declared when every local stack is empty, the shared buffer is empty, and all
  threads are idle.

The only shared mutable structures are the hash set and the spill buffer; both are sharded or touched
only at spill and steal events.

### 8.4. Deduplication

The global hash set stores fingerprints of cuts generated so far. An entry means that the cut was
generated by some thread; there is no per-entry state, because a cut is inserted once and never
revisited. A frame is skipped if its fingerprint is already present.

---

## 9. Complexity

**Theorem 4 (Time per frame).** Processing a frame requires $O(|P| \cdot (n + m))$ operations.

**Proof.** Recovering $V_1$ by a forward BFS costs $O(n+m)$. The branching loop runs once per
*distinct head* of $P$ (Proposition 1); with $\hbar \le |P|$ distinct heads, the incremental BFS costs
$O(\hbar\,(n+m))$ and the cleanups cost one reverse BFS each, $O(\hbar\,(n+m))$. The total is
$O(\hbar \cdot (n+m)) = O(|P| \cdot (n+m))$. $\square$

**Corollary 4 (Total time).** $O(|\mathbb{D}| \cdot \bar p \cdot (n+m) / T)$, where $\bar p$ is the
average cut size and $T$ the number of threads. Parallel efficiency additionally depends on
synchronisation overhead and load balance.

**Theorem 5 (Memory).** The algorithm requires $O(|\mathbb{D}|)$ words for the global hash set and
$O(n + m + L \cdot \bar p)$ per thread, where $L$ is the maximum local stack size and $\bar p$ the
average cut size. A fingerprint occupies 16 bytes; hash-table overhead increases the actual
consumption.

---

## 10. Applications and extensions

### 10.1. Application areas

APDFS addresses problems that require complete structural enumeration of all minimal $(S,T)$-cuts;
unlike flow algorithms it does not select a minimum-weight cut.

**Reliability engineering.** Minimal cuts of a reliability graph are minimal cut sets. APDFS
enumerates them without constructing a binary decision diagram and streams the results with bounded
memory.

**Network policy verification.** In SDN and firewall configurations a minimal cut is a minimal set of
rules whose removal breaks an isolation; completeness of the enumeration guarantees that no
isolation-breaking configuration is missed.

**Metabolic networks.** In a metabolic pathway a minimal cut is a minimal set of reactions whose
knock-out halts the synthesis of a target metabolite [5]. The analysis is structural and requires no
kinetic parameters.

**Stochastic sampling.** The branching operator defines a neighbourhood on $\mathbb{D}$ and supports
Markov chains that sample cuts without full enumeration (Section 10.6).

### 10.2. Extension 1: size-bounded enumeration

**Problem.** Enumerate all minimal cuts of cardinality at most $K$.

**Method.** For a multicut $Q$ before cleanup we know $|R| \le |Q|$ for every minimal cut
$R \subseteq Q$. If $|Q| \le K$ the frame is generated unconditionally; otherwise it is generated as
speculative and the resulting cut is kept only if $|P_{new}| \le K$.

**Correctness.** Every minimal cut of size at most $K$ is generated, because a cut of size at most
$K$ is contained in some candidate with $|Q| \le K$ or survives the speculative test.

### 10.3. Extension 2: $K$-best cuts by cardinality

**Problem.** Find the $K$ minimal cuts with the fewest edges.

**Method.** Apply Extension 1 with increasing bound $s = 1, 2, 3, \ldots$ until $K$ cuts have been
collected; cuts are obtained in nondecreasing cardinality. The extension does not apply to
$K$-best under arbitrary weights, which requires an external constrained minimum-cut solver.

### 10.4. Extension 3: Pareto-optimal cuts

**Problem.** Given $d \ge 1$ weight functions $w_1, \ldots, w_d: E \to \mathbb{R}_{\ge 0}$, find all
Pareto-optimal minimal cuts.

**Method.** A criterion vector is attached to every entry of the seen set. During duplicate checking,
Pareto dominance is tested: a new cut dominated by a seen cut is discarded, a cut dominating a seen
one replaces it, and incomparable cuts are both kept.

**Correctness.** The traversal of $\mathcal{G}$ is complete. A Pareto-optimal cut $P^\*$ is either
absent from the seen set when generated, or present as $P'$ with the same fingerprint. If
$W(P') \prec W(P^\*)$ then $P^\*$ is not Pareto-optimal, a contradiction; if $W(P^\*) \prec W(P')$
then $P^\*$ replaces $P'$.

**Implementation note.** Replacing a cut whose descendants were already processed requires additional
synchronisation.

### 10.5. Extension 4: incremental update

**Problem.** Maintain $\mathbb{D}$ under insertion or deletion of an edge $e^\*$.

**Definition 11 (Affected cuts).** A cut $P \in \mathbb{D}(G)$ is affected by $e^\*$ if $e^\*$ is
inserted and some path $S^\* \leadsto T^\*$ in the new graph contains $e^\*$ and no other edge of
$P$, or if $e^\*$ is deleted and $e^\* \in P$.

**Lemma 6.** If $P \in \mathbb{D}(G)$ is not affected by $e^\*$, then $P \in \mathbb{D}(G')$.

**Algorithm.**

1. Identify the affected cuts $Z \subseteq \mathbb{D}(G)$.
2. $\mathbb{D}_{keep} \leftarrow \mathbb{D}(G) \setminus Z$; these cuts are preserved by Lemma 6.
3. For every $P \in Z$, add $P$ to $\mathbb{D}_{keep}$ if it is still a minimal cut in $G'$.
4. Run APDFS on $G'$ with the seen set initialised to $\mathbb{D}_{keep}$ and with the cuts of $Z$
   as additional roots.

### 10.6. Extension 5: stochastic sampling

**Problem.** Construct a Markov chain on $\mathbb{D}$ with stationary distribution $\pi$.

**Definition 12 (Neighbourhood).** Cuts $P, P' \in \mathbb{D}$ are neighbours if
$\Phi(P, e) = P'$ for some $e \in P$ or $\Phi(P', e') = P$ for some $e' \in P'$.

**Lemma 7.** The neighbourhood relation is symmetric and connected on $\mathbb{D}$.

**Sampler.**

1. Start at $P \leftarrow P_0$.
2. At each step: choose $e \in P$ uniformly; compute $P' = \Phi(P, e)$ and stay at $P$ if
   $P' = \varnothing$; compute $n(P, P') = |\{\, e \in P \mid \Phi(P, e) = P' \,\}|$ and
   $n(P', P) = |\{\, e' \in P' \mid \Phi(P', e') = P \,\}|$; accept the transition with probability
   $$\alpha = \min\left(1, \frac{\pi(P')}{\pi(P)} \cdot \frac{n(P', P) \cdot |P|}{n(P, P') \cdot
   |P'|}\right).$$
3. Record the visited states.

**Theorem 6.** The chain is irreducible and aperiodic on $\mathbb{D}$ with stationary distribution
$\pi$.

**Complexity.** Computing $n(P, P')$ and $n(P', P)$ requires $O(|P| \cdot (n+m))$ operations per step;
caching transitions is a possible optimisation.

---

## 11. Conclusion

The APDFS algorithm enumerates the family of inclusion-minimal $(S,T)$-cuts by traversing a DAG of
cuts whose potential is the reachable vertex set. The proofs of soundness, monotonicity, completeness
and termination are self-contained and depend neither on hashing nor on parallelism; hashing affects
completeness only through a one-sided $2^{-128}$ error, and parallelism only affects throughput. The
algorithm has polynomial delay and output-polynomial total time, and its extensions cover the
variants most often required in practice: bounded size, cardinality ranking, Pareto optimality,
incremental recomputation and sampling.

---

## References

1. Tsukiyama, S., Ide, M., Ariyoshi, H., & Shirakawa, I. (1980). A new algorithm for generating all
   the minimal cuts in a directed graph. *Electronics and Communications in Japan*, 63(1), 1–8.
2. Picard, J. C., & Queyranne, M. (1980). On the structure of all minimum cuts in a network and
   applications. *Mathematical Programming Study*, 13, 8–16.
3. Karger, D. R. (1993). Global min-cuts in RNC, and other ramifications of a simple min-cut
   algorithm. *Proceedings of SODA '93*, 21–30.
4. Provan, J. S., & Shier, D. R. (1996). A paradigm for listing $(s,t)$-cuts in graphs.
   *Algorithmica*, 15(4), 351–372.
5. Klamt, S., & Gilles, E. D. (2004). Minimal cut sets in biochemical reaction networks.
   *Bioinformatics*, 20(2), 226–234.
6. Lawler, E. L. (1972). A procedure for computing the K best solutions to discrete optimization
   problems and its application to the shortest path problem. *Management Science*, 18(7), 401–405.
