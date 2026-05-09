from scipy.signal import iirfilter
import numpy as np
import math

fs = 16000

# High-pass 80Hz, 2nd order (1 Biquad stage)
sos_hp = iirfilter(2, 80, btype='high', fs=fs, output='sos')

# Low-pass 7.2kHz, 6th order (3 Biquad stages)
sos_lp = iirfilter(6, 7200, btype='low', fs=fs, output='sos')

# Combine into 4 stages
sos = np.vstack([sos_hp, sos_lp])

print("=" * 60)
print("  IIR Biquad Filter Coefficient Calculator (Q31)")
print("=" * 60)

print("\nSOS coefficients (float):")
for i, s in enumerate(sos):
    print(f"  Stage {i+1}: b0={s[0]:.10f}, b1={s[1]:.10f}, b2={s[2]:.10f}, "
          f"a1={s[4]:.10f}, a2={s[5]:.10f}")

# CMSIS Biquad format: [b0, b1, b2, -a1, -a2] (a0=1 not stored)
coeffs = []
for s in sos:
    b0, b1, b2 = s[0], s[1], s[2]
    a1_neg = -s[4]  # -a1
    a2_neg = -s[5]  # -a2
    coeffs.extend([b0, b1, b2, a1_neg, a2_neg])

# Check max coefficient magnitude
max_coeff = max(abs(c) for c in coeffs)
print(f"\nMax coefficient magnitude: {max_coeff:.10f}")

# Determine required postShift for Q31
# Q31 range: -2147483648 to 2147483647, represents -1.0 to ~0.999999999
# Need: |coeff| / 2^postShift <= 1.0
required_shift = max(0, math.ceil(math.log2(max_coeff)))
postShift = required_shift
scale = 2 ** postShift

print(f"Required postShift: {postShift} (coefficients divided by {scale})")

# Convert to Q31
q31 = np.round(np.array(coeffs) / scale * 2147483648.0).astype(np.int64)

# Clamp to int32 range
q31 = np.clip(q31, -2147483648, 2147483647).astype(np.int32)

print(f"\nQ31 coefficients (CMSIS format: b0, b1, b2, -a1, -a2):")
print(f"  postShift = {postShift}")
print()

# Verify
print("Overflow check:")
overflow = False
for i, c in enumerate(q31):
    if c < -2147483648 or c > 2147483647:
        print(f"  coeff[{i}] = {c} OVERFLOW!")
        overflow = True
if not overflow:
    print("  All coefficients within Q31 (int32) range. OK")

# Quantization error analysis
print("\nQuantization error analysis:")
for i in range(4):
    for j in range(5):
        idx = i * 5 + j
        original = coeffs[idx]
        stored_float = q31[idx] / 2147483648.0 * scale
        error = abs(original - stored_float)
        error_pct = error / abs(original) * 100 if original != 0 else 0
        labels = ["b0", "b1", "b2", "-a1", "-a2"]
        print(f"  Stage {i+1} {labels[j]}: orig={original:>12.8f}, "
              f"stored={stored_float:>12.8f}, err={error:.2e} ({error_pct:.6f}%)")

# Print C code
print(f"\n{'='*60}")
print(f"  C Code Output (Q31, postShift={postShift})")
print(f"{'='*60}")
print()
print(f"/* Biquad coefficients in Q31 format (postShift={postShift}) */")
print(f"/* Each stage: b0, b1, b2, -a1, -a2 */")
print(f"static const q31_t filterCoeffs[FILTER_BIQUAD_STAGES * 5] = {{")
for i in range(4):
    stage = q31[i*5:(i+1)*5]
    if i == 0:
        desc = "High-pass fc=80Hz, 2nd order"
    else:
        desc = f"Low-pass fc=7.2kHz, section {i} of 3"
    print(f"    /* Stage {i+1} - {desc} */")
    entries = ", ".join(f"{int(v):>12d}" for v in stage)
    if i < 3:
        print(f"    {entries},")
    else:
        print(f"    {entries}")
print("};")
print()
print(f"/* postShift = {postShift} */")
print(f"arm_biquad_cascade_df1_init_q31(&biquadInstance, FILTER_BIQUAD_STAGES,")
print(f"    (q31_t*)filterCoeffs, filterState, {postShift});")