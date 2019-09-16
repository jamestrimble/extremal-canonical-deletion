include "globals.mzn";

int: a;   % Max degree
int: b;   % Min degree minus 1
int: n_targets;

set of int: Targets = 1..n_targets;

array[1..a, 1..b, 1..b] of var Targets: v_target;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% SYMMETRY BREAKING
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

constraint forall(y in 1..b, z in 1..b) (
  v_target[1, y, z] = (y-1) * b + z
);

constraint forall(x in 1..a, y in 1..(b-1)) (
  v_target[x, y, 1] < v_target[x, y+1, 1]
);

constraint forall(x in 1..(a-1)) (
  lex_lesseq([v_target[x,   1, z] | z in 1..b],
           [v_target[x+1, 1, z] | z in 1..b])
);

constraint forall(x in 1..a, y in 1..b) (
  increasing([v_target[x, y, z] | z in 1..b])
);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

constraint forall(x in 1..a, y in 1..b) (
  alldifferent([v_target[x, y, z] | y in 1..b, z in 1..b])
);

constraint forall(x1 in 1..(a-1), x2 in (x1+1)..a, y1 in 1..b, y2 in 1..b) (
  sum(z1 in 1..b, z2 in 1..b)(v_target[x1, y1, z1] = v_target[x2, y2, z2]) <= 1
);

constraint forall(t in Targets) (
  sum(x in 1..a, y in 1..b, z in 1..b)(v_target[x, y, z] = t) <= a
);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

solve satisfy;
%%% solve :: int_search([v_target[x, y, z] | x in 1..a, y in 1..b, z in 1..b], dom_w_deg, indomain_split, complete) satisfy;

