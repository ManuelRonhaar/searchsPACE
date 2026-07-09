# PACE 2026 submission

## Approach
The solver combines simulated annealing and column generation, a description can be found in `solver_description.pdf`.

## Code structure
 * `sa.cpp` contains all code for the simulated annealing
 * `column_generation` contains the code for column generation calling simulated annealing as an initial solution