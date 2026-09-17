# Mathematical Contract (Framework Faithful)

This file is the *single source of truth* for what `mt_core` realizes
mathematically. Every code path that claims framework status must satisfy
the identities below. Symbols match `refer/this.txt`.

> **Convention lock.** Newton–Gregory uses the **rising factorial** basis
> $$
> \beta_k(h)\;=\;\frac{1}{k!}\prod_{j=0}^{k-1}(h+j),
> $$
> the same basis the framework uses in
> $\sum_{k=0}^{N-1}\beta_k(h)\nabla^k x_n$.
> Falling-factorial language is **forbidden** in code, comments, tests, and
> docs.

## 1. Piecewise-Taylor interstice representation

For a sample path $x_t(\tau)$ with deterministic event times
$0=\sigma_0(t)<\sigma_1(t)<\sigma_2(t)<\cdots\to\infty$ and Heaviside $H$:
$$
x_t(\tau)
=\sum_{n\ge 0} A_{\phi,n}(t)\,\tau^{n}
+\sum_{\ell\ge 1}H\!\bigl(\tau-\sigma_\ell(t)\bigr)
   \sum_{n\ge 0} A_{\pi,\ell,n}(t)\,\bigl(\tau-\sigma_\ell(t)\bigr)^{n}.
$$

The past-jet is the triple
$$
J_t^{-}\;=\;\Bigl((A_{\phi,n}(t))_{n\ge0},\ (\sigma_\ell(t))_{\ell\ge1},\ (A_{\pi,\ell,n}(t))_{\ell\ge1,\,n\ge0}\Bigr),
$$
and the forward formation is
$$
F_t[J_t^-](h)
=\sum_{n\ge 0} A_{\phi,n}(t)\,h^{n}
+\sum_{\ell\ge 1}H\!\bigl(h-\sigma_\ell(t)\bigr)
   \sum_{n\ge 0} A_{\pi,\ell,n}(t)\,\bigl(h-\sigma_\ell(t)\bigr)^{n}.
$$

**Causality clause.** Every $A_{\phi,n}(t)$, $\sigma_\ell(t)$,
$A_{\pi,\ell,n}(t)$ is measurable with respect to $H_t^{(\infty)}$. No
candle indexed $>t$ may enter the construction of $J_t^{-}$.

## 2. Phase-torus extraction

For analytic $F:\mathbb C^Q\to\mathbb C$, polyradius $\rho_m>0$,
multi-index $\alpha\in\mathbb Z_{\ge 0}^Q$:
$$
A^{(m)}_{F,\alpha}(z)
=\frac{1}{(2\pi)^Q}\int_{[0,2\pi)^Q}
F\!\bigl(z+\rho_m e^{i\psi}\bigr)\,e^{-i\alpha\cdot\psi}\,d^Q\psi
=\frac{\partial^{\alpha}F(z)}{\alpha!}\,\rho_m^{|\alpha|}.
$$

## 3. Jet normalization (theorem object)

$$
\boxed{\,J_\alpha[F](z)\;=\;\rho_m^{-|\alpha|}\,A^{(m)}_{F,\alpha}(z)
\;=\;\frac{1}{\alpha!}\,\partial^{\alpha} F(z).\,}
$$

No additional factorial may appear at any other call site.

## 4. Discrete derivation/integration operators on the jet

With $D_i=\varepsilon_m^{-1}\partial_{\eta_i}$ and
$I_i=\varepsilon_m\,I_i\,\Pi_{\eta_i,\perp}$,
$$
\boxed{\,D_i I_i=\Pi_{\eta_i,\perp},\qquad I_i D_i=\Pi_{\eta_i,\perp}.\,}
$$

Coefficient-wise on $J(\eta)=\sum_\alpha c_\alpha\,\eta^\alpha$:
$$
(D_i J)(\eta)=\varepsilon_m^{-1}\sum_{\alpha:\alpha_i\ge 1}\alpha_i\,c_\alpha\,\eta^{\alpha-e_i},
$$
$$
(I_i J)(\eta)=\varepsilon_m\sum_{\alpha}\frac{c_\alpha}{\alpha_i+1}\,\eta^{\alpha+e_i},
$$
$$
(\Pi_{\eta_i,\perp} J)(\eta)=\sum_{\alpha:\alpha_i>0} c_\alpha\,\eta^{\alpha}.
$$

## 5. Fixed-point form

$$
N(U)=0
\iff
U=U_0+\mathfrak I_\phi(\sigma)H_N(U)
\iff
J[N(U)]=0
\iff
\forall N,\ \mathfrak N_N[N(U)]_n(h)=0.
$$

## 6. Evolution equality (the four-way identity)

$$
\boxed{\,E_h[F]_n\;=\;J_h[F]_n\;=\;N_h[b]_n\;=\;M_h[Z_n].\,}
$$

In the realized code these are:

| Symbol | Carrier | Code object |
|---|---|---|
| $E_h[F]_n$ | continuous formation | `EvolutionEquality::U_continuous` (= $F_t[J_t^-](h)$) |
| $J_h[F]_n$ | jet semigroup | `EvolutionEquality::U_jet` (= $(e^{hL_{\mathrm{jet},\kappa}}J_n^-)_0$) |
| $N_h[b]_n$ | Newton–Gregory | `EvolutionEquality::U_newton` |
| $M_h[Z_n]$ | matrix propagator | `EvolutionEquality::U_matrix` (companion orbit $R\,M_*^h Z_n$) |

In the finite-dimensional closure regime the corollaries
$$
M_h[Z_n]\;=\;[z^h]\frac{P_{r-1}(z)}{Q_r(z)}\;=\;\sum_{j,s}\gamma_{j,s}\binom{h}{s}\lambda_j^{h-s}
$$
must also hold:

| Symbol | Carrier | Code object |
|---|---|---|
| rational | $[z^h]P/Q$ | `EvolutionEquality::U_rational` |
| spectral | mode sum | `EvolutionEquality::U_spectral` |

Acceptance: `assert_evolution_equality` returns true iff
$$
\max_h\bigl|U_{\mathrm{continuous}}-U_{\mathrm{jet}}\bigr|<\varepsilon,
\quad
\max_h\bigl|U_{\mathrm{jet}}-U_{\mathrm{newton}}\bigr|<\varepsilon,
\quad
\max_h\bigl|U_{\mathrm{newton}}-U_{\mathrm{matrix}}\bigr|<\varepsilon,
$$
and (when finite-dim closure is detected)
$$
\max_h\bigl|U_{\mathrm{matrix}}-U_{\mathrm{rational}}\bigr|<\varepsilon,
\qquad
\max_h\bigl|U_{\mathrm{rational}}-U_{\mathrm{spectral}}\bigr|<\varepsilon.
$$

## 7. Newton–Gregory + matrix expansion law

$$
\sum_{k=0}^{N-1}\frac{\partial^{k}F(n,\kappa)}{k!}h^{k}
=\sum_{k=0}^{N-1}\beta_k(h)\,\nabla^{k}x_n
=\sum_{k=0}^{N-1}\frac{1}{k!}\!\left(\prod_{j=0}^{k-1}(h+j)\right)\!\sum_{m=0}^{k}(-1)^{m}\binom{k}{m}x_{n-m}.
$$

$$
x_{n+h}=\sum_{k=0}^{N-1}\beta_k(h)\,\nabla^{k}x_n
=\sum_{\mu=1}^{R}\sum_{s=0}^{m_\mu-1}\Gamma_{\mu,s}(n)\,h^{s}e^{\lambda_\mu h}.
$$

## 8. Finite-dimensional closure: rational + spectral

$$
\mathcal R_{U_0}(n,z)=\sum_{h\ge0}\mathscr U_{U_0}(n,h)z^{h}
=R(I-zM_*)^{-1}Z_n
=\frac{R\,\operatorname{adj}(I-zM_*)\,Z_n}{\det(I-zM_*)},
$$
$$
\det(I-zM_*)=\prod_{\lambda\in\sigma(M_*)}(1-\lambda z)^{m_\lambda},
\qquad
u_h=\sum_{j,s}\gamma_{j,s}\binom{h}{s}\lambda_j^{h-s}.
$$

The numerator construction is exact: with
$U(z)=\sum_{n\ge 0}u_n z^n$ and $Q(z)=1+\sum_{j=1}^{r}q_j z^j$,
$$
P(z)=Q(z)\,U(z)\bmod z^{r}
\quad\Longleftrightarrow\quad
P_n=u_n+\sum_{j=1}^{n}q_j\,u_{n-j},\quad 0\le n\le r-1.
$$

## 9. Hankel rank / annihilating polynomial

$$
H_N=(u_{i+j})_{0\le i,j\le N},\qquad \operatorname{rank}H_N\le d,
$$
$$
r=\min\{m\ge 1:\det H_m=0\},\qquad
H_{r-1}c=-v_r,\qquad c=-H_{r-1}^{-1}v_r,
$$
$$
q_r(\lambda)=\lambda^{r}+c_{r-1}\lambda^{r-1}+\cdots+c_0,
\qquad
Q_r(z)=z^{r}q_r(z^{-1}).
$$

## 10. Interstice transport (parameterized line)

$$
\delta(s)=e^{u\,\theta(s)},\qquad
\gamma'(s)=\delta(s),\qquad
\delta'(s)=u\,\kappa(s)\,\delta(s).
$$

Discrete deterministic integrator (fixed step $\Delta s$):
$$
\delta_{j+1}=\delta_j\,\exp\bigl(u\,\kappa_j\,\Delta s\bigr),
\qquad
\gamma_{j+1}=\gamma_j+\delta_j\,\Delta s.
$$

Field along the line:
$$
\mathcal I_f^{(m)}(s,h)=f\!\bigl(\gamma(s)+\delta(s)h\bigr),
\qquad
\partial_s\mathcal I_f^{(m)}(s,h)=\bigl(1+u\,\kappa(s)\,h\bigr)\,\partial_h\mathcal I_f^{(m)}(s,h).
$$

Trajectory carrier:
$$
\mathcal T_t^{\mathrm{int}}(s,h)
=\mathfrak F_{U_0}\!\bigl(\gamma(s)+\delta(s)h\bigr)
=\sum_{j=1}^{J}\sum_{m=0}^{\mu_j-1}
\widetilde\Gamma_{j,m}^{\mathrm{int}}(n,s)\binom{h}{m}\Lambda_j(s)^{h-m},
$$
$$
\Lambda_j(s)=\lambda_j^{\delta(s)},
\qquad
q_r^{\mathrm{int}}(n,s,S_h)\,\mathcal T_t^{\mathrm{int}}(s,h)=0.
$$

Derived layers:
$$
\mathcal L_t^{\mathrm{int}}(s,h):=\Pi_3\,\mathcal T_t^{\mathrm{int}}(s,h),\quad
\mathcal P_t^{\mathrm{int}}(s,h):=e^{\mathcal L_t^{\mathrm{int}}(s,h)},\quad
\mathcal R_t^{\mathrm{int}}(s,h):=\mathcal L_t^{\mathrm{int}}(s,h)-\mathcal L_t^{\mathrm{int}}(s,0).
$$

## 11. Forecast energy (framework risk carrier)

$$
P_{t,V}^{(N)}(\tau)
=\Bigl|(\partial_V E)(\xi_t+\eta,\cdot)+\partial_V\mathfrak X_t^{(N)}(\tau,\cdot)\Bigr|^{2}.
$$

In code, `RiskField::energy_framework[i]` realizes this; any practical
proxies (`energy_proxy`, `drawdown_proxy`, `curvature`, `uncertainty_proxy`)
must be named explicitly as proxies and never relabelled "framework
energy".

## 12. Deterministic acceptance ledger

Every authoritative run must satisfy:

1. $u_n=F_t[J_t^-](n)$ uses **only** bars indexed $\le t$.
2. $J_\alpha[F]=\rho_m^{-|\alpha|}A^{(m)}_{F,\alpha}$ pointwise.
3. $D_i I_i = I_i D_i = \Pi_{\eta_i,\perp}$ on every truncated jet.
4. $\max_h|U_{\mathrm{continuous}}-U_{\mathrm{jet}}|<\varepsilon$.
5. $\max_h|U_{\mathrm{jet}}-U_{\mathrm{newton}}|<\varepsilon$.
6. $\max_h|U_{\mathrm{newton}}-U_{\mathrm{matrix}}|<\varepsilon$.
7. $\max_h|U_{\mathrm{matrix}}-U_{\mathrm{rational}}|<\varepsilon$ (closed regime).
8. $\max_h|U_{\mathrm{rational}}-U_{\mathrm{spectral}}|<\varepsilon$ (closed regime).
9. $\partial_s\mathcal T_t^{\mathrm{int}}(s,h)=(1+u\kappa(s)h)\,\partial_h\mathcal T_t^{\mathrm{int}}(s,h)$ at sampled grid points.
10. $q_r^{\mathrm{int}}(n,s,S_h)\,\mathcal T_t^{\mathrm{int}}(s,h)=0$ at sampled grid points.
11. Walk-forward contains zero lookahead (training window strictly $<$ anchor).
12. All six CLI apps execute concrete pipelines (no scaffold prints):
   `mt_ingest`, `mt_extract`, `mt_trajectorize`, `mt_backtest`,
   `mt_benchmark`, `mt_verify_authority`.

If any clause fails, the run is **not** authoritative.

## 13. Framework-faithfulness clauses (binding on every theorem object)

These are *additional* binding clauses introduced when the implementation
was made framework-faithful. Earlier debug artifacts written before they
were enforced (see `artifacts/_archive/`) are not authoritative and must
not be revived.

13.1 **Channel cardinality.** Anchor extraction must run with $Q=5$
canonical OHLCV channels $x_0=\log\mathrm{open}$, $x_1=\log\mathrm{high}$,
$x_2=\log\mathrm{low}$, $x_3=\log\mathrm{close}$,
$x_4=\log(1+\mathrm{vol}/\mathrm{vol\_norm})$. Single-axis $x_3$-only
extraction is forbidden in any path that claims framework status.

13.2 **Separable extractor.** When the anchor field is built from
per-channel polynomial carriers, the production extractor uses
$F(z)=\sum_{q=1}^{Q} G_q(z_q)$ and the per-axis 1-D Cauchy ring
quadrature in `extract_phase_torus_separable_cpu`. Mixed multi-indices
$\alpha$ with $|\{q:\alpha_q>0\}|>1$ are exactly zero by construction.

13.3 **Newton–Gregory anchoring.** All four carriers in §6 must be built
from the same causal anchor $F_t[J_t^-](\cdot)$ with identical horizon
indexing and `hg.h[0] = 0`. The Newton path is fed the past sample vector
$u_{\mathrm{past}}[j] = F_t[J_t^-](-j)$ for $j=0,\dots,K{-}1$, reversed,
then evaluated as $\sum_{k=0}^{K-1}\beta_k(h)\nabla^k\!\bigl(\mathrm{rev}\bigr)_{K-1}$
at $\eta=h$. Any $\eta=h-(K-1)$ shift is forbidden.

13.4 **Closure non-triviality.** A finite-dimensional closure with
$r=1$ and a single mode is rejected as authoritative whenever the
event/spectral structure is non-trivial (any of: non-empty event
boundaries, $>1$ event packets, non-empty $\sigma$). Such runs fail the
`recurrence.nontrivial_closure` clause.

13.5 **Genuine packet dynamics.** Event packets are authoritative only
when derived from real packet dynamics (`event_boundaries_from_genuine_packets = true`)
or when the section is verified single-sector smooth. Synthetic
max-transition fallback boundaries flip this provenance flag to `false`
and the run fails the `event_state.genuine_packet_dynamics` clause.

13.6 **Framework-energy authority.** `RiskField::energy_framework[i]`
must be the realized $P_{t,V}^{(N)}$ field (§11). When the trajectory
does not carry a framework-energy channel, `energy_framework_present`
is `false` and `energy_framework[i]=0`; the silent fallback to
$|T_i|^2$ or $|R_i|^2$ that earlier debug artifacts relied on is
forbidden.

13.7 **Signal authority.** Production signals must consume the framework
energy field and project across the full $s$-grid (averaging the field
log over $s$, not slicing $s_{\mathrm{ref}}=0$). Heuristic / proxy
routing is allowed only under `research_non_authoritative = true` and
must never be promoted to production.

13.8 **Implementation authority.** `gpu_mode = "cpu_authoritative"` and
`implementation_authoritative = false` until CUDA parity is established
against the CPU theorem path with the tolerances in clauses 4–10.
