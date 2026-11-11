"""Scratch check for the worked Viterbi example in answers.md (rate-1/2, K=3, generators 7,5)."""
INF = 10**9
states = [(0, 0), (0, 1), (1, 0), (1, 1)]      # (u[t-1], u[t-2])
idx = {s: i for i, s in enumerate(states)}

def step(s, u):
    a, b = s
    return (u, a), (u ^ a ^ b, u ^ b)

def encode(bits):
    s, out = (0, 0), []
    for u in bits:
        s, o = step(s, u)
        out.append(o)
    return out

msg = [1, 0, 1, 0, 0]                           # 3 data bits + 2 tail zeros
tx = encode(msg)
print("tx", tx)
rx = [list(p) for p in tx]
rx[2][0] ^= 1                                   # one channel bit error (first bit of 3rd pair)
rx = [tuple(p) for p in rx]
print("rx", rx)

pm = {s: (0 if s == (0, 0) else INF) for s in states}
surv = []
print("t=0", pm)
for t, r in enumerate(rx, 1):
    new, back = {s: INF for s in states}, {}
    for s in states:
        if pm[s] >= INF:
            continue
        for u in (0, 1):
            ns, o = step(s, u)
            bm = (o[0] != r[0]) + (o[1] != r[1])
            if pm[s] + bm < new[ns]:
                new[ns], back[ns] = pm[s] + bm, (s, u)
    pm = new
    surv.append(back)
    print(f"t={t}", {''.join(map(str, s)): v for s, v in pm.items()})

s = (0, 0)
bits = []
for back in reversed(surv):
    p, u = back[s]
    bits.append(u)
    s = p
print("decoded", bits[::-1], "metric", pm[(0, 0)])
