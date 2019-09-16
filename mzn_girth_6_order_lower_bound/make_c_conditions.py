import sys

lines = [line for line in sys.stdin.readlines()]

for line in lines:
    tokens = [int(tok) for tok in line.strip().split()[:3]]
    print 'if (min_deg=={min_deg} && max_deg=={max_deg} && {min_deg}*{max_deg} + 1 + {a} >= n) return false;'.format(
            min_deg=tokens[0], max_deg=tokens[1], a=tokens[2])

