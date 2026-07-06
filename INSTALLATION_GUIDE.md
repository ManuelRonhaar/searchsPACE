The build requires or-tools 9.12 as a dependency, compilation with cmake/g++.

CMakeLists.txt contains build procedure, resulting `heuristic` and `lower_bound` executables are in `build/bin/`.

The current docker_setup.sh uses dynamic linking, as static linking requires (as far as I know) building or-tools from source, which is non-trivial.