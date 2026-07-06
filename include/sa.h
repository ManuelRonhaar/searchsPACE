#include <vector>
#include <utility>
#include <unordered_map>
#include <string>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <memory>
#include <queue>
#include <chrono>
#include <csignal>

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "ortools/base/init_google.h"
#include "ortools/base/logging.h"
#include "ortools/linear_solver/linear_solver.h"
#include "ortools/base/init_google.h"


using namespace operations_research;
using namespace std;

struct Column {
    int connections;
    std::string solution;
    std::vector<int> internal_nodes;
    std::vector<int> leaves;
    const MPVariable* var;
};

int sa_main(std::istream& in, std::ostream& out, std::vector<Column>* columns, int nrof_iterations);