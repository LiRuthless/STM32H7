#!/usr/bin/env python3
"""Convert this report's prose/table mathematics into $LaTeX$.

This is a document-specific, conservative migration, not a general mathematical
parser. Longer, reviewed expressions take precedence over a finite vocabulary of
symbols. Existing math, code, links, headings and chapters 17/18 are preserved.
Default mode is a read-only dry run; --write explicitly updates the input file.
The caller can also import convert_text(text) to compose with display conversion.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import sys


# The left side is copied literally from report.md. Never change an equation's
# sign, coordinate convention, initial condition or reported number here.
EXPRESSIONS = {
    "q⊗[0,u_b]⊗q*": r"q\otimes[0,u_b]\otimes q^{*}",
    "R_nb=R_z(ψ)R_y(θ)R_x(φ)": r"R_{nb}=R_z(\psi)R_y(\theta)R_x(\phi)",
    "g_n=(0,0,−g)": r"g_n=(0,0,-g)",
    "g=9.80665": r"g=9.80665",
    "t_(k+1)−t_k": r"t_{k+1}-t_k",
    "δ=true−nominal": r"\delta=\mathrm{true}-\mathrm{nominal}",
    "ψ=0": r"\psi=0",
    "v_0=0": r"v_0=0", "p_0=0": r"p_0=0",
    "f^b=R_nbᵀ(a^n−g^n)": r"f^b=R_{nb}^{\mathsf T}(a^n-g^n)",
    "(0,0,+g)": r"(0,0,+g)",
    "d=ω̄Δt": r"d=\bar\omega\,\Delta t",
    "|d|": r"\lVert d\rVert", "d/2": r"d/2",
    "(v_k+v_(k+1))Δt/2": r"\frac{v_k+v_{k+1}}{2}\Delta t",
    "f_b=(1,0,g)": r"f_b=(1,0,g)",
    "q=单位四元数": r"q=\text{单位四元数}",
    "v_0=p_0=0": r"v_0=p_0=0",
    "Δt=0.01 s": r"\Delta t=0.01\,\mathrm{s}",
    "a_n=(1,0,0)": r"a_n=(1,0,0)",
    "v_x=0.01 m/s": r"v_x=0.01\,\mathrm{m/s}",
    "p_x=0.00005 m": r"p_x=0.00005\,\mathrm m",
    "v_x=1 m/s": r"v_x=1\,\mathrm{m/s}",
    "p_x=0.5 m": r"p_x=0.5\,\mathrm m",
    "sinc(u)=sin(u)/u": r"\operatorname{sinc}(u)=\frac{\sin u}{u}",
    "sinc(0)=1": r"\operatorname{sinc}(0)=1",
    "b(T)": r"b(T)", "M(T)": r"M(T)",
    "R_hat≈(I+[ε_θ]×)R_true": r"\widehat R\approx(I+[\varepsilon_\theta]_\times)R_{\mathrm{true}}",
    "[u]×v=u×v": r"[u]_\times v=u\times v",
    "R_true f_true≈(0,0,g)": r"R_{\mathrm{true}}f_{\mathrm{true}}\approx(0,0,g)",
    "gε_tilt": r"g\varepsilon_{\mathrm{tilt}}",
    "E[n(t)n(t′)]=qδ(t−t′)": r"\mathbb E[n(t)n(t')]=q\delta(t-t')",
    "σ_v=√(q_a t)": r"\sigma_v=\sqrt{q_a t}",
    "σ_p=√(q_a t³/3)": r"\sigma_p=\sqrt{q_a t^3/3}",
    "σ_θ=√(q_g t)": r"\sigma_\theta=\sqrt{q_g t}",
    "σ_p≈g√(q_g t⁵/20)": r"\sigma_p\approx g\sqrt{q_g t^5/20}",
    "σ_sample²=q/Δt": r"\sigma_{\mathrm{sample}}^2=q/\Delta t",
    "σ_sample²": r"\sigma_{\mathrm{sample}}^2",
    "v=1 m/s": r"v=1\,\mathrm{m/s}",
    "v=2 m/s": r"v=2\,\mathrm{m/s}",
    "ω=0.5 rad/s": r"\omega=0.5\,\mathrm{rad/s}",
    "vΔτ": r"v\Delta\tau", "ωΔτ": r"\omega\Delta\tau",
    "b_hat=mean(ω_m)": r"\widehat b=\operatorname{mean}(\omega_m)",
    "σ²/N": r"\sigma^2/N",
    "b_i=(f_i,+ + f_i,−)/2": r"b_i=\frac{f_{i,+}+f_{i,-}}{2}",
    "m_i=(f_i,+−f_i,−)/(2g)": r"m_i=\frac{f_{i,+}-f_{i,-}}{2g}",
    "(f_i−b_i)/m_i": r"\frac{f_i-b_i}{m_i}",
    "C=M_a⁻¹": r"C=M_a^{-1}",
    "a_n≈0": r"a_n\approx0",
    "φ_hat_k=α(φ_hat_(k−1)+ω_xΔt)+(1−α)φ_acc,k":
        r"\widehat\phi_k=\alpha(\widehat\phi_{k-1}+\omega_x\Delta t)+(1-\alpha)\phi_{\mathrm{acc},k}",
    "α≈exp(−Δt/τ_f)": r"\alpha\approx\exp(-\Delta t/\tau_f)",
    "|f|≈g": r"\lVert f\rVert\approx g",
    "v_f=e_xᵀRᵀv": r"v_f=e_x^{\mathsf T}R^{\mathsf T}v",
    "(v_R−v_L)/B": r"\frac{v_R-v_L}{B}",
    "v tanδ/L": r"v\tan\delta/L",
    "m^b≈Rᵀm^n": r"m^b\approx R^{\mathsf T}m^n",
    "p+R l_G": r"p+R l_G",
    "v+R(ω×l_G)": r"v+R(\omega\times l_G)",
    r"ρ_i=\|p+Rl_sensor−a_i\|": r"\rho_i=\lVert p+Rl_{\mathrm{sensor}}-a_i\rVert",
    "ρ_i=|p+Rl_sensor−a_i|": r"\rho_i=\lVert p+Rl_{\mathrm{sensor}}-a_i\rVert",
    "ψ_mid=ψ_k+(u_g−b_g)Δt/2": r"\psi_{\mathrm{mid}}=\psi_k+\frac{(u_g-b_g)\Delta t}{2}",
    "a_mid=R(ψ_mid)(u_a−b_a)": r"a_{\mathrm{mid}}=R(\psi_{\mathrm{mid}})(u_a-b_a)",
    "η=[n_ax,n_ay,n_g,w_bg,w_bax,w_bay]ᵀ":
        r"\eta=[n_{ax},n_{ay},n_g,w_{bg},w_{bax},w_{bay}]^{\mathsf T}",
    "true=measurement−bias−noise": r"\mathrm{true}=\mathrm{measurement}-\mathrm{bias}-\mathrm{noise}",
    "E[η(t)η(t′)ᵀ]=Q_c δ(t−t′)": r"\mathbb E[\eta(t)\eta(t')^{\mathsf T}]=Q_c\delta(t-t')",
    "Φ≈I+FΔt": r"\Phi\approx I+F\Delta t",
    "Q_d≈GQ_cGᵀΔt": r"Q_d\approx GQ_cG^{\mathsf T}\Delta t",
    "q_a[[Δt³/3,Δt²/2],[Δt²/2,Δt]]":
        r"q_a\begin{bmatrix}\Delta t^3/3&\Delta t^2/2\\\Delta t^2/2&\Delta t\end{bmatrix}",
    "ψ=2°": r"\psi=2^\circ", "b_g=0.5°/s": r"b_g=0.5\,{}^\circ/\mathrm s",
    "σ_f=0.05 m/s": r"\sigma_f=0.05\,\mathrm{m/s}",
    "σ_lat=0.03 m/s": r"\sigma_{\mathrm{lat}}=0.03\,\mathrm{m/s}",
    "[v_by,−v_bx]ᵀ": r"[v_{by},-v_{bx}]^{\mathsf T}",
    "v=0": r"v=0",
    "v_axis^b=Rᵀv_IMU+ω^b×l^b": r"v_{\mathrm{axis}}^b=R^{\mathsf T}v_{\mathrm{IMU}}+\omega^b\times l^b",
    "z_G=p+R l_G+ν_G": r"z_G=p+R l_G+\nu_G",
    "J R l_G": r"J R l_G",
    "H_G=[I,0,…]": r"H_G=[I,0,\ldots]",
    "r_ψ=wrap(z_ψ−ψ_hat)": r"r_\psi=\operatorname{wrap}(z_\psi-\widehat\psi)",
    "ω_m−b_g": r"\omega_m-b_g",
    "K=0.8": r"K=0.8",
    "v_by=0": r"v_{by}=0", "a_by=vω": r"a_{by}=v\omega",
    "z_ω=ω_m": r"z_\omega=\omega_m", "h=b_g": r"h=b_g",
    "z_v=0": r"z_v=0", "h=v": r"h=v",
    "v_by≈0": r"v_{by}\approx0", "v_bz≈0": r"v_{bz}\approx0",
    "ω≈0": r"\omega\approx0",
    "HF²": r"HF^2",
    "p_d(s)": r"p_d(s)",
    "ψ_d=atan2(y′_d,x′_d)": r"\psi_d=\operatorname{atan2}(y'_d,x'_d)",
    "κ_d=(x′y″−y′x″)/(x′²+y′²)^(3/2)": r"\kappa_d=\frac{x'y''-y'x''}{(x'^2+y'^2)^{3/2}}",
    "(v_ref,ω_ref)": r"(v_{\mathrm{ref}},\omega_{\mathrm{ref}})",
    "(v_ref,δ_ref)": r"(v_{\mathrm{ref}},\delta_{\mathrm{ref}})",
    "(p_hat,v_hat,ψ_hat)": r"(\widehat p,\widehat v,\widehat\psi)",
    "Rᵀ(p_goal−p_hat)": r"R^{\mathsf T}(p_{\mathrm{goal}}-\widehat p)",
    "(x_L,y_L)": r"(x_L,y_L)",
    "L_d=√(x_L²+y_L²)>0": r"L_d=\sqrt{x_L^2+y_L^2}>0",
    "L_d=clip(L_0+T_L|v|,L_min,L_max)":
        r"L_d=\operatorname{clip}(L_0+T_L\lvert v\rvert,L_{\min},L_{\max})",
    "x_L=0.8 m": r"x_L=0.8\,\mathrm m", "y_L=0.2 m": r"y_L=0.2\,\mathrm m",
    "L_d²=0.68 m²": r"L_d^2=0.68\,\mathrm{m^2}",
    "κ_ref=0.588235 m⁻¹": r"\kappa_{\mathrm{ref}}=0.588235\,\mathrm{m^{-1}}",
    "v_ref=0.6 m/s": r"v_{\mathrm{ref}}=0.6\,\mathrm{m/s}",
    "ω_ref=0.352941 rad/s": r"\omega_{\mathrm{ref}}=0.352941\,\mathrm{rad/s}",
    "vτ+v²/(2a_brake)": r"v\tau+\frac{v^2}{2a_{\mathrm{brake}}}",
    "v_ref=0.8 m/s": r"v_{\mathrm{ref}}=0.8\,\mathrm{m/s}",
    "ω_ref=0.3 rad/s": r"\omega_{\mathrm{ref}}=0.3\,\mathrm{rad/s}",
    "B=0.3 m": r"B=0.3\,\mathrm m", "L=0.5 m": r"L=0.5\,\mathrm m",
    "v_L=0.755": r"v_L=0.755", "v_R=0.845 m/s": r"v_R=0.845\,\mathrm{m/s}",
    "δ≈10.6197°": r"\delta\approx10.6197^\circ",
    "ψ_ref=−179°": r"\psi_{\mathrm{ref}}=-179^\circ",
    "ψ_hat=179°": r"\widehat\psi=179^\circ",
    "Ψ_k=Ψ_(k−1)+wrap(ψ_k−ψ_(k−1))":
        r"\Psi_k=\Psi_{k-1}+\operatorname{wrap}(\psi_k-\psi_{k-1})",
    "v>0": r"v>0",
    "ė_y≈v e_ψ": r"\dot e_y\approx v e_\psi",
    "ė_ψ≈−K_ψe_ψ−K_ye_y": r"\dot e_\psi\approx-K_\psi e_\psi-K_y e_y",
    "λ²+K_ψλ+vK_y": r"\lambda^2+K_\psi\lambda+vK_y",
    "δ_ref=e_ψ+atan2(k e_y,v+v_soft)":
        r"\delta_{\mathrm{ref}}=e_\psi+\operatorname{atan2}(k e_y,v+v_{\mathrm{soft}})",
    "e_y=+0.05 m": r"e_y=+0.05\,\mathrm m", "e_ψ=+3°": r"e_\psi=+3^\circ",
    "K_ψ=2 s⁻¹": r"K_\psi=2\,\mathrm{s^{-1}}",
    "K_y=3 (m·s)⁻¹": r"K_y=3\,(\mathrm{m\cdot s})^{-1}",
    "ω_ref≈0.10472+0.15=0.25472 rad/s": r"\omega_{\mathrm{ref}}\approx0.10472+0.15=0.25472\,\mathrm{rad/s}",
    "v_ref=0.5 m/s": r"v_{\mathrm{ref}}=0.5\,\mathrm{m/s}",
    "v_L≈0.46179": r"v_L\approx0.46179", "v_R≈0.53821 m/s": r"v_R\approx0.53821\,\mathrm{m/s}",
    "v_ref=0": r"v_{\mathrm{ref}}=0",
    "e_Ψ=Ψ_target−Ψ_hat": r"e_\Psi=\Psi_{\mathrm{target}}-\widehat\Psi",
    "|ω|τ": r"\lvert\omega\rvert\tau",
    "v_ref≤√(2a_brake max(e_D−d_margin,0))":
        r"v_{\mathrm{ref}}\le\sqrt{2a_{\mathrm{brake}}\max(e_D-d_{\mathrm{margin}},0)}",
    "e_D<0": r"e_D<0",
    "|e_D|<ε_D": r"\lvert e_D\rvert<\varepsilon_D",
    "|e_y|<ε_y": r"\lvert e_y\rvert<\varepsilon_y",
    "|e_ψ|<ε_ψ": r"\lvert e_\psi\rvert<\varepsilon_\psi",
    "|v_hat|<ε_v": r"\lvert\widehat v\rvert<\varepsilon_v",
    "0.1+1/(2×0.5)=1.1 m": r"0.1+\frac{1}{2\times0.5}=1.1\,\mathrm m",
    "c=min(1,v_w,max/max(|v_L|,|v_R|))":
        r"c=\min\!\left(1,\frac{v_{w,\max}}{\max(\lvert v_L\rvert,\lvert v_R\rvert)}\right)",
    "|κ|≤tanδ_max/L": r"\lvert\kappa\rvert\le\tan\delta_{\max}/L",
    "dt=0.01 s": r"\mathrm{d}t=0.01\,\mathrm s",
    "τ_m=0.15 s": r"\tau_m=0.15\,\mathrm s",
    "ω_command=clip(3e_Ψ−0.6ω,±1.2 rad/s)":
        r"\omega_{\mathrm{command}}=\operatorname{clip}(3e_\Psi-0.6\omega,\pm1.2\,\mathrm{rad/s})",
    r"\|ω\|≤0.02 rad/s": r"\lvert\omega\rvert\le0.02\,\mathrm{rad/s}",
    "p_hat−p_true": r"\widehat p-p_{\mathrm{true}}", "ψ_hat−ψ_true": r"\widehat\psi-\psi_{\mathrm{true}}",
    "p_target−p_hat": r"p_{\mathrm{target}}-\widehat p", "ψ_target−ψ_hat": r"\psi_{\mathrm{target}}-\widehat\psi",
    "D_hat=1.05D_true": r"\widehat D=1.05D_{\mathrm{true}}",
    "D_hat=10 m": r"\widehat D=10\,\mathrm m", "D_true=9.523810 m": r"D_{\mathrm{true}}=9.523810\,\mathrm m",
    "δω≈K_ψδe_ψ+K_yδe_y": r"\delta\omega\approx K_\psi\delta e_\psi+K_y\delta e_y",
    "I_new=u_previous−u_ff−K_Pe_new": r"I_{\mathrm{new}}=u_{\mathrm{previous}}-u_{\mathrm{ff}}-K_P e_{\mathrm{new}}",
    "q_i≥0": r"q_i\ge0",
    "y_c=Σy_iq_i/Σq_i": r"y_c=\frac{\sum_i y_iq_i}{\sum_i q_i}",
    "[p_x,p_y,ψ]": r"[p_x,p_y,\psi]",
    "[−n_dx,−n_dy,−l_x]": r"[-n_{dx},-n_{dy},-l_x]",
    "l_2−l_1": r"l_2-l_1",
    "(y_c2−y_c1)/(l_2−l_1)": r"\frac{y_{c2}-y_{c1}}{l_2-l_1}",
    "v=0.5 m/s": r"v=0.5\,\mathrm{m/s}",
    "½vβ_g t²≤0.05": r"\tfrac12 v\beta_g t^2\le0.05",
    "t≤10.70 s": r"t\le10.70\,\mathrm s",
    "[δp₃,δv₃,δθ₃,δb_g₃,δb_a₃]":
        r"[\delta p_3,\delta v_3,\delta\theta_3,\delta b_{g3},\delta b_{a3}]",
    "v_n,z=0": r"v_{n,z}=0", "v cosθ": r"v\cos\theta",
    "θ<0": r"\theta<0", "f_x=−g sinθ": r"f_x=-g\sin\theta",
    "θ=−10°": r"\theta=-10^\circ",
    "a_IMU=a_axis+R[α×l+ω×(ω×l)]": r"a_{\mathrm{IMU}}=a_{\mathrm{axis}}+R[\alpha\times l+\omega\times(\omega\times l)]",
    "a_by≈vω": r"a_{by}\approx v\omega",
    "M/b(T)": r"M/b(T)",
    "θ_err=acos[clip((tr(R_trueᵀR_hat)−1)/2,−1,1)]":
        r"\theta_{\mathrm{err}}=\arccos\!\left[\operatorname{clip}\!\left(\frac{\operatorname{tr}(R_{\mathrm{true}}^{\mathsf T}\widehat R)-1}{2},-1,1\right)\right]",
    r"√mean(\|estimate−truth\|²)": r"\sqrt{\operatorname{mean}(\lVert\mathrm{estimate}-\mathrm{truth}\rVert^2)}",
    "√mean(|estimate−truth|²)": r"\sqrt{\operatorname{mean}(\lVert\mathrm{estimate}-\mathrm{truth}\rVert^2)}",
    "E_ij=(T_true,i⁻¹T_true,j)⁻¹(T_hat,i⁻¹T_hat,j)":
        r"E_{ij}=(T_{\mathrm{true},i}^{-1}T_{\mathrm{true},j})^{-1}(\widehat T_i^{-1}\widehat T_j)",
    "NEES=δxᵀP⁻¹δx": r"\mathrm{NEES}=\delta x^{\mathsf T}P^{-1}\delta x",
    "v_max=0.8 m/s": r"v_{\max}=0.8\,\mathrm{m/s}",
    "a_max=0.2 m/s²": r"a_{\max}=0.2\,\mathrm{m/s^2}",
    "π/180": r"\pi/180",
    "s、b、n": r"s,b,n",
    "0.025 rad≈1.43°": r"0.025\,\mathrm{rad}\approx1.43^\circ",
    "=0.1+1/(2×0.5)=1.1 m": r"=0.1+\frac{1}{2\times0.5}=1.1\,\mathrm m",
    "≤1°": r"\le1^\circ",
    "≤路程×1.8%": r"\le\text{路程}\times1.8\%",
    "<1.8%": r"<1.8\%",
}


GREEK = {
    "φ": r"\phi", "θ": r"\theta", "ψ": r"\psi", "ω": r"\omega",
    "α": r"\alpha", "β": r"\beta", "ε": r"\varepsilon", "δ": r"\delta",
    "σ": r"\sigma", "τ": r"\tau", "η": r"\eta", "ν": r"\nu",
    "ρ": r"\rho", "κ": r"\kappa", "λ": r"\lambda", "π": r"\pi",
    "Φ": r"\Phi", "Ψ": r"\Psi", "Δ": r"\Delta",
}

# Explicit symbols that occur in prose/table cells. A vocabulary prevents names
# such as robot_localization, source_manifest and author names from being read as
# mathematical subscripts. Longest-match and Latin/Greek boundaries stop prefixes.
SYMBOL_NAMES = """
R_nb R_bs q_nb R_z R_y R_x p_n v_n g_n b_g b_a t_k q_0 v_0 p_0 M_g M_a
a_k a_(k+1) q_k v_k v_(k+1) f_b a_n v_x p_x v_y N_rev β_g β_a ε_θ ε_tilt
ε_tilt,0 β_g,tilt β_g,z ε_ψ,0 q_a q_g σ_v σ_p σ_θ b_hat ω_m b_i m_i f_i
f̄_j b_g(T) b_a(T) ψ_0 φ_acc φ_acc,k u_i w_i K_P K_I τ_f r_dir J_dir
v_f e_x l_G l_sensor a_i ρ_i R_z u_g u_a ψ_mid a_mid P_0 Q_c Q_d
σ_f σ_lat v_enc v_by v_bx v_bz l^b H_G H_ψ r_ψ z_ψ ψ_hat ω_odo
z_ω z_v b_cal(T) b_residual HF e_y e_ψ ψ_ref ΔΨ_ref p_d ψ_d κ_d
v_ref ω_ref δ_ref p_hat v_hat x_L y_L L_d L_0 T_L L_min L_max
κ_ref a_lat,max a_brake ε_κ v_task B_f v_L v_R ω_hat e_D ε_D ε_y ε_ψ
ε_v T_hold K_ψ K_y v_soft Ψ_k Ψ_(k−1) ψ_k ψ_(k−1) e_Ψ Ψ_target Ψ_hat
K_Ψ K_ω α_brake d_margin K_Pv K_Iv K_aw u_actual v_w,max δ_max
τ_m ω_command D_hat D_true p_true ψ_true p_target ψ_target
I_new u_previous u_ff e_new l_x y_i q_i y_c n_dx n_dy y_c2 y_c1
l_2 l_1 p_y v_n,z a_IMU a_axis θ_err E_ij T_i P₀ v_max a_max
R_hat R_true f_true T_hat T_true n_ax n_ay n_g n_a w_bg w_bax w_bay
Δt Δτ Δq Δψ Δs δx ω̄
x y z n b p v q f g d r t u w l C M B L N T F G H P Q R S I E J D e k c
""".split()


def _subscript(part: str) -> str:
    part = part.replace("−", "-").replace("₀", "0")
    if part.startswith("(") and part.endswith(")"):
        part = part[1:-1]
    if part in GREEK:
        return GREEK[part]
    # Mathematical components stay italic; descriptive qualifiers are upright.
    descriptive = {
        "rev", "tilt", "true", "acc", "dir", "lat", "enc",
        "residual", "ref", "min", "max", "brake", "task", "soft", "hold",
        "margin", "aw", "actual", "command", "target", "previous", "ff",
        "new", "sensor", "IMU", "axis", "err", "mid",
    }
    parts = part.split(",")
    return ",".join(GREEK.get(p, (r"\mathrm{" + p + "}") if p in descriptive else p)
                    for p in parts)


def _symbol_tex(name: str) -> str:
    suffix = ""
    if name.endswith("(T)"):
        name, suffix = name[:-3], "(T)"
    if name.endswith("^b"):
        name, suffix = name[:-2], "^b" + suffix
    base, sep, sub = name.partition("_")
    base = {"f̄": r"\bar f", "ω̄": r"\bar\omega", "P₀": "P_0"}.get(
        base, " ".join(GREEK.get(ch, ch) for ch in base)
        if any(ch in GREEK for ch in base) else base
    )
    if sub.startswith("hat"):
        base = r"\widehat{" + base + "}"
        sub = sub[3:].lstrip("_")
    if sub:
        base += "_{" + _subscript(sub) + "}"
    return base + suffix


SYMBOLS = {name: _symbol_tex(name) for name in SYMBOL_NAMES}
SYMBOLS.update(GREEK)
SYMBOLS.update({
    "Rᵀ": r"R^{\mathsf T}", "P⁻¹": r"P^{-1}", "Gᵀ": r"G^{\mathsf T}",
    "s⁻¹": r"\mathrm{s^{-1}}", "s⁻²": r"\mathrm{s^{-2}}",
    "m⁻¹": r"\mathrm{m^{-1}}", "m²": r"\mathrm{m^2}",
    "m/s²/√Hz": r"\mathrm{m/s^2}/\sqrt{\mathrm{Hz}}",
    "rad/s/√Hz": r"\mathrm{rad/s}/\sqrt{\mathrm{Hz}}",
    "m/s²/√s": r"\mathrm{m/s^2}/\sqrt{\mathrm s}",
    "rad/s/√s": r"\mathrm{rad/s}/\sqrt{\mathrm s}",
    "m²/s³": r"\mathrm{m^2/s^3}", "rad²/s³": r"\mathrm{rad^2/s^3}",
    "rad²/s": r"\mathrm{rad^2/s}", "m²/s⁵": r"\mathrm{m^2/s^5}",
    "rad/s²": r"\mathrm{rad/s^2}", "m/s²": r"\mathrm{m/s^2}",
    "(m/s)²": r"(\mathrm{m/s})^2", "(m·s)⁻¹": r"(\mathrm{m\cdot s})^{-1}",
    "1/m": r"1/\mathrm m", "s/m": r"\mathrm{s/m}",
    "rad/°": r"\mathrm{rad}/{}^\circ",
})

# Finite, reviewed strings, never arbitrary underscore tokens. Checking only
# alphabetic adjacency permits Chinese prose to touch a variable without spaces.
_ALPHA = r"A-Za-z0-9Α-Ωα-ω"
_REPLACEMENTS = {**SYMBOLS, **EXPRESSIONS}
_REPLACE_RE = re.compile(
    r"(?<![" + _ALPHA + r"_])(?:"
    + "|".join(re.escape(s) for s in sorted(_REPLACEMENTS, key=len, reverse=True))
    + r")(?![" + _ALPHA + r"_])"
)

# Preserve complete Markdown links/images, inline code, and existing $ math.
# URLs may contain parentheses, hence a small balanced scanner rather than a
# single greedy regex; URLs and labels are left wholly unchanged.
def _protected_end(text: str, start: int) -> int | None:
    if text[start] == "`":
        end_run = start
        while end_run < len(text) and text[end_run] == "`":
            end_run += 1
        closer = text.find(text[start:end_run], end_run)
        return len(text) if closer < 0 else closer + end_run - start
    if text[start] == "$" and (start == 0 or text[start - 1] != "\\"):
        width = 2 if text.startswith("$$", start) else 1
        end = start + width
        while end < len(text):
            end = text.find("$" * width, end)
            if end < 0:
                return len(text)
            if text[end - 1] != "\\":
                return end + width
            end += width
    bracket = start + 1 if text.startswith("![", start) else start
    if text[bracket:bracket + 1] == "[":
        label_depth, close = 1, bracket + 1
        while close < len(text) and label_depth:
            if text[close] == "\\":
                close += 2
                continue
            if text[close] == "[":
                label_depth += 1
            elif text[close] == "]":
                label_depth -= 1
            close += 1
        # Only the bracket's own closing delimiter may begin a link. Do not
        # search forward into a later [Sxx](URL) after a plain vector [x,y].
        if label_depth == 0 and text[close:close + 1] == "(":
            depth, pos = 1, close + 1
            while pos < len(text) and depth:
                if text[pos] == "\\":
                    pos += 2
                    continue
                if text[pos] == "(":
                    depth += 1
                elif text[pos] == ")":
                    depth -= 1
                pos += 1
            if depth == 0:
                return pos
    if re.match(r"<(?:https?://|/?[A-Za-z][A-Za-z0-9-]*(?:\s|/?>))", text[start:]):
        end = text.find(">", start + 1)
        if end >= 0:
            return end + 1
    return None


def _transform_segment(segment: str, stats: Counter) -> str:
    def substitute(match: re.Match) -> str:
        source = match.group(0)
        stats["expressions" if source in EXPRESSIONS else "symbols_units"] += 1
        stats["total"] += 1
        return "$" + _REPLACEMENTS[source] + "$"

    return _REPLACE_RE.sub(substitute, segment)


def _line_convert(line: str, stats: Counter) -> str:
    chunks, plain_start, cursor = [], 0, 0
    while cursor < len(line):
        end = _protected_end(line, cursor)
        if end is not None:
            chunks.append(_transform_segment(line[plain_start:cursor], stats))
            chunks.append(line[cursor:end])
            cursor = plain_start = end
        else:
            cursor += 1
    chunks.append(_transform_segment(line[plain_start:], stats))
    return "".join(chunks)


def convert_text(text: str) -> tuple[str, dict[str, int]]:
    """Return converted Markdown and counts; no files are read or written."""
    stats: Counter = Counter()
    output = []
    fence: str | None = None
    display = False
    appendix = False
    for line in text.splitlines(keepends=True):
        if re.match(r"^##\s+(?:17|18)\.", line):
            appendix = True
        if appendix:
            output.append(line)
            continue
        marker = re.match(r"^\s*(`{3,}|~{3,})", line)
        if marker:
            kind = marker.group(1)[0]
            if fence is None:
                fence = kind
            elif fence == kind:
                fence = None
            output.append(line)
            continue
        if fence is not None:
            output.append(line)
            continue
        # Block converter may run first or after this migration. Both are safe.
        if line.strip().startswith("$$"):
            if line.strip().count("$$") == 1:
                display = not display
            output.append(line)
            continue
        if display or re.match(r"^\s*#{1,6}\s", line):
            output.append(line)
            continue
        converted = _line_convert(line, stats)
        stats["changed_lines"] += converted != line
        output.append(converted)
    return "".join(output), dict(stats)


def main() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=Path,
                        default=Path(__file__).resolve().parents[1] / "report.md")
    parser.add_argument("--write", action="store_true", help="update input; default only audits")
    parser.add_argument("--output", type=Path, help="save converted text to another file")
    parser.add_argument("--show-diff", action="store_true", help="print changed lines after JSON summary")
    args = parser.parse_args()
    # newline='' preserves source CRLF and avoids the dry run changing line ends.
    with args.input.open("r", encoding="utf-8", newline="") as stream:
        original = stream.read()
    converted, stats = convert_text(original)
    again, second_stats = convert_text(converted)
    if again != converted:
        raise RuntimeError("inline math migration is not idempotent")
    stats["idempotent"] = True
    stats["second_pass_total"] = second_stats.get("total", 0)
    if args.write and args.output:
        parser.error("choose either --write or --output")
    destination = args.input if args.write else args.output
    if destination is not None:
        with destination.open("w", encoding="utf-8", newline="") as stream:
            stream.write(converted)
    print(json.dumps(stats, ensure_ascii=False, indent=2))
    if args.show_diff:
        for before, after in zip(original.splitlines(), converted.splitlines()):
            if before != after:
                print("- " + before)
                print("+ " + after)


if __name__ == "__main__":
    main()
