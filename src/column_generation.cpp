#include "sa.h"

struct Node {
    int parent;
    int left;
    int right;
    int sibling;
    int label;
    int lp_index;
};

struct State {
    int nrof_leaves;
    Node* tree1;
    Node* tree2;
    double *penalties;
    pair<int, int> *leaves;
    double a;
    double b;
};

void read_tree(int nrof_leaves, Node *nodes, int lp_index, std::istream& in) {
    int current = 0;
    int next = 1;

    nodes[current].parent = -1;
    nodes[current].lp_index = lp_index++;

    while (1) {
        char c = in.get();
        switch (c) {
            case '(':
                nodes[next].parent = current;
                nodes[current].left = next;
                nodes[next].lp_index = lp_index++;
                current = next;
                next += 1;
                break;
            case ')':
                current = nodes[current].parent;
                break;
            case ',':
            {
                int p = nodes[current].parent;
                nodes[next].parent = p;
                nodes[p].right = next;
                nodes[next].lp_index = lp_index++;
                nodes[next].sibling = current;
                nodes[current].sibling = next;
                current = next;
                next += 1;
                break;
            }
            case ';':
                return;
            default: // integer
                in.unget();
                int label;
                in >> label;
                nodes[current].label = label;
                nodes[current].lp_index = 2*nrof_leaves - 3 + label;
                lp_index--;
                break;
        }
    }
}

constexpr auto max_size = std::numeric_limits<std::streamsize>::max();
State read_input(std::istream& in) {
    State state;
    state.a = 1;
    state.b = 0;
    int read_first_tree = 0;

    while (char c = in.get()) {
        if (c == '#') {
            char type = in.get();
            if (type == 'p') {
                int nrof_trees;
                in >> nrof_trees >> state.nrof_leaves;
                state.tree1 = (Node *) calloc(1, 4*state.nrof_leaves*sizeof(Node) + (2*state.nrof_leaves)*sizeof(double) + state.nrof_leaves*sizeof(pair<int, int>));
                state.tree2 = state.tree1 + 2*state.nrof_leaves;
                state.penalties = (double *) (void *) (state.tree2 + 2*state.nrof_leaves);
                state.leaves = (pair<int, int> *) (void *) (state.penalties + 2*state.nrof_leaves);
            } else if (type == 'a') {
                in >> state.a >> state.b;
            }
            in.ignore(max_size, '\n');
        } else if (!read_first_tree) {
            in.unget();
            read_tree(state.nrof_leaves, state.tree1, 0, in);
            read_first_tree = 1;
            in.ignore(max_size, '\n');
        } else {
            in.unget();
            read_tree(state.nrof_leaves, state.tree2, state.nrof_leaves-1, in);
            in.ignore(max_size, '\n');
            for (int i = 1; i <= state.nrof_leaves; i++) {
                int node1 = 0;
                int node2 = 0;
                for (int j = 0; j < 2*state.nrof_leaves; j++) {
                    if (state.tree1[j].label == i) {
                        node1 = j;
                    }
                    if (state.tree2[j].label == i) {
                        node2 = j;
                    }
                }
                state.leaves[i-1] = {node1, node2};
            }
            break;
        }
    }

    return state;
}



struct pairHash {
    size_t operator()(const pair<int,int>& p) const {
        return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
    }
};

// types: 0 -> no extra, 1 -> extra, 2 -> extra except first
void backtrack(State& state, const unordered_map<pair<int,int>, tuple<double, int, int>, pairHash>& dp, Column& column, pair<int,int> index, std::ostream& ss, std::vector<Column>& columns, int type) {
    auto [node1, node2] = index;
    if (state.tree1[node1].label && state.tree2[node2].label) {
        ss << state.tree1[node1].label;
        column.leaves.push_back(state.tree1[node1].label);
        column.connections += 1;
        return;
    }

    auto [score, child1, child2] = dp.find(index)->second;
    if (child2 == node2) { // non join
        column.internal_nodes.push_back(state.tree1[node1].lp_index);
        backtrack(state, dp, column, {child1, node2}, ss, columns, type ? 1 : 0);
    } else if (child1 == node1) { // non join
        column.internal_nodes.push_back(state.tree2[node2].lp_index);
        backtrack(state, dp, column, {node1, child2}, ss, columns, type ? 1 : 0);
    } else {  // join
        column.internal_nodes.push_back(state.tree1[node1].lp_index);
        column.internal_nodes.push_back(state.tree2[node2].lp_index);
        ss << "(";
        backtrack(state, dp, column, {child1, child2}, ss, columns, type ? 1 : 0);
        ss << ",";
        backtrack(state, dp, column, {state.tree1[child1].sibling, state.tree2[child2].sibling}, ss, columns, type ? 1 : 0);
        ss << ")";

        if (type == 1 && score < -0.00001) {
            Column column;
            column.connections = -1;
            std::stringstream ss;
            backtrack(state, dp, column, {node1, node2}, ss, columns, 0);
            ss << ";";
            column.solution = ss.str();
            columns.push_back(column);
        }
    }
}

// reduced cost is negative in this function!!
double find_many_new_columns(State& state, std::vector<Column>& columns, double shadow_increase) {
    double sum = 0.0;

    priority_queue<pair<int,int>> pq;
    unordered_map<pair<int,int>, tuple<double, int, int>, pairHash> dp;

    for (int i = 0; i < state.nrof_leaves; i++) {
        auto leaf = state.leaves[i];
        pq.push(leaf);
        dp[leaf] = {0, 0, 0};
    }

    while(!pq.empty()) {
        auto [node1, node2] = pq.top();
        pq.pop();

        auto [current_value, child1, child2] = dp[{node1, node2}];
        if (child1 != node1 && child2 != node2) {
            if (current_value < -0.00001) {
                sum += current_value;
                Column column;
                column.connections = -1;
                std::stringstream ss;
                backtrack(state, dp, column, {node1, node2}, ss, columns, 0);
                ss << ";";
                column.solution = ss.str();
                columns.push_back(column);
            }
        }

        if (current_value > 1) continue;

        int parent1 = state.tree1[node1].parent;
        int parent2 = state.tree2[node2].parent;
        if (parent1 == -1 || parent2 == -1) {
            // no more merge ever possible, so we don't need to continue
            continue;
        }

        double penalty1 = state.penalties[state.tree1[parent1].lp_index];
        double penalty2 = state.penalties[state.tree2[parent2].lp_index];

        if (shadow_increase > 0.0) {
            penalty1 += shadow_increase;
            penalty2 += shadow_increase;
        }

        // parent 1
        tuple<double, int, int> new_value = {current_value + penalty1, node1, node2};
        auto [it, inserted] = dp.try_emplace({parent1, node2}, new_value);
        if (!inserted) {
            if (std::get<0>(new_value) < std::get<0>(it->second))  it->second = new_value;
        } else {
            pq.push({parent1, node2});
        }

        // parent 2
        std::get<0>(new_value) = current_value + penalty2;
        std::tie(it, inserted) = dp.try_emplace({node1, parent2}, new_value);
        if (!inserted) {
            if (std::get<0>(new_value) < std::get<0>(it->second))  it->second = new_value;
        } else {
            pq.push({node1, parent2});
        }

        // merge
        pair<int, int> sibling = {state.tree1[node1].sibling, state.tree2[node2].sibling};
        auto sibling_value = dp.find(sibling);
        if (sibling_value != dp.end()) {
            std::get<0>(new_value) += penalty1 + std::get<0>(sibling_value->second) - 1;
            std::tie(it, inserted) = dp.try_emplace({parent1, parent2}, new_value);
            if (!inserted) {
                if (std::get<0>(new_value) < std::get<0>(it->second))  it->second = new_value;
            } else {
                pq.push({parent1, parent2});
            }
        }
    }

    LOG(INFO) << "Improvement sum: " << sum;
    return sum;
}


struct RMP {
    std::unique_ptr<MPSolver> solver;
    std::vector<MPVariable*> slack_vars;
    std::vector<MPConstraint*> constraints;
    MPObjective* objective;
};

struct RMP build_rmp(State& state, int nrof_constraints) {
    struct RMP rmp;
    rmp.solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("glop"));
    // rmp.solver->SetSolverSpecificParametersAsString("use_dual_simplex: true");
    rmp.objective = rmp.solver->MutableObjective();
    rmp.objective->SetMaximization();

    rmp.constraints = std::vector<MPConstraint*>(nrof_constraints);
    rmp.slack_vars = std::vector<MPVariable*>(nrof_constraints);
    for (int i = 0; i < nrof_constraints; i++) {
        rmp.constraints[i] = rmp.solver->MakeRowConstraint(0.0, 1.0);

        rmp.slack_vars[i] = rmp.solver->MakeNumVar(-0.0000001, 0.0000001, "");
        rmp.constraints[i]->SetCoefficient(rmp.slack_vars[i], 1);
        rmp.objective->SetCoefficient(rmp.slack_vars[i], 0.25);
    }

    return rmp;
}

void update_stabilization(RMP& rmp, State& state, int nrof_constraints) {
    for (int i = 0; i < nrof_constraints; i++) {
        rmp.objective->SetCoefficient(rmp.slack_vars[i], state.penalties[i]);
    }
}

int main() {
    auto program_start = std::chrono::high_resolution_clock::now();
    std::string data(
        std::istreambuf_iterator<char>(std::cin),
        std::istreambuf_iterator<char>());

    std::istringstream in1(data);
    std::istringstream in2(data);

    State state = read_input(in1);

    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
    absl::InitializeLog();
    int nrof_constraints = 2*state.nrof_leaves - 2;

    RMP rmp = build_rmp(state, nrof_constraints);

    double price_time = 0;
    double lp_time = 0;
    std::vector<Column> columns;
    int column_index = 0; // first non added column to the lp
    int upper_bound = state.nrof_leaves;
    int lower_bound = 0;
    string sa_forest;
    double shadow_increase = 0.1;

#ifdef LOWER_BOUND
    int column_generation_duration = 540;  // seconds
    int sa_iterations = nrof_constraints * nrof_constraints / 5;
    if (state.a < 1.005 && state.b < 2) {
        sa_iterations = 50000000;
    }
#else
    int column_generation_duration = 200;  // seconds
    int sa_iterations = 50000000;
#endif

    for (int iteration = 0; iteration < 2000; iteration++) {
        auto start = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double>(start - program_start).count() > column_generation_duration)  break;
        if (iteration == 0) {
            std::ostringstream ss;
            upper_bound = sa_main(in2, ss, &columns, sa_iterations);
            sa_forest = ss.str();
        } else {
            if (iteration < 20 && nrof_constraints > 1000) {
                shadow_increase *= 0.8;
                LOG(INFO) << "shadow increase: " << shadow_increase;
            } else {
                shadow_increase = -1;
            }
            double improvement_sum = find_many_new_columns(state, columns, shadow_increase);
            if (columns.size() == column_index && shadow_increase > 0) {
                auto end = std::chrono::high_resolution_clock::now();
                price_time += std::chrono::duration<double>(end - start).count();
                continue;
            }
            double new_lower_bound = ceil(state.nrof_leaves - rmp.objective->Value() + improvement_sum);
            if (shadow_increase < 0 && new_lower_bound > lower_bound) {
                lower_bound = new_lower_bound;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        price_time += std::chrono::duration<double>(end - start).count();
        LOG(INFO) << upper_bound << " > " << floor(state.a * lower_bound) + state.b;
        if (upper_bound <= floor(state.a * lower_bound) + state.b) {
            cout << sa_forest;
            return 0;
        } else if (columns.size() == column_index) {
            LOG(INFO) << "starting ILP solver";
            break;
        }

        if (iteration != 0)  update_stabilization(rmp, state, nrof_constraints);

        for (; column_index < columns.size(); column_index++) {
            columns[column_index].var = rmp.solver->MakeNumVar(0, 2, "");

            for (int node : columns[column_index].internal_nodes) {
                rmp.constraints[node]->SetCoefficient(columns[column_index].var, 1);
            }

            rmp.objective->SetCoefficient(columns[column_index].var, columns[column_index].connections);
        }

        start = std::chrono::high_resolution_clock::now();
        rmp.solver->SetTimeLimit(absl::Seconds(column_generation_duration+10 - std::chrono::duration<double>(start - program_start).count()));
        auto result_status = rmp.solver->Solve();
        end = std::chrono::high_resolution_clock::now();
        lp_time += std::chrono::duration<double>(end - start).count();
        if (result_status != MPSolver::OPTIMAL && result_status != MPSolver::FEASIBLE || std::chrono::duration<double>(start - program_start).count() > column_generation_duration) {
            break;
        }
        LOG(INFO) << "Iteration " << iteration << ", objective: " << state.nrof_leaves - rmp.objective->Value() << ", total_columns: " << columns.size();
        for (int j = 0; j < nrof_constraints; j++) {
            state.penalties[j] = rmp.constraints[j]->dual_value();
        }

        LOG(INFO) << "price_time=" << price_time << " lp_time=" << lp_time;
    }

    double goal = floor(state.a * lower_bound) + state.b;

#ifdef LOWER_BOUND
    { // bounded ILP solve
        struct RMP rmp;
        rmp.solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("cbc"));
        auto _ = rmp.solver->SetNumThreads(1);
        if (!rmp.solver) {
            LOG(INFO) << "cbc solver unavailable.";
            return 1;
        }

        rmp.constraints = std::vector<MPConstraint*>(nrof_constraints);
        for (int j = 0; j < nrof_constraints; j++) {
            rmp.constraints[j] = rmp.solver->MakeRowConstraint(0.0, 1.0);
        }

        auto constraint = rmp.solver->MakeRowConstraint(state.nrof_leaves - goal, 10000000000);

        for (int i = 0; i < columns.size(); i++) {
            columns[i].var = rmp.solver->MakeIntVar(0, 1, "");

            for (int node : columns[i].internal_nodes) {
                rmp.constraints[node]->SetCoefficient(columns[i].var, 1);
            }

            constraint->SetCoefficient(columns[i].var, columns[i].connections);
        }
        LOG(INFO) << "Staring ILP solver with " << columns.size() << " columns";

        auto start = std::chrono::high_resolution_clock::now();
        auto result_status = rmp.solver->Solve();
        auto end = std::chrono::high_resolution_clock::now();
        LOG(INFO) << "IP solve time: " << std::chrono::duration<double>(end - start).count();
        if (result_status != MPSolver::OPTIMAL && result_status != MPSolver::FEASIBLE) {
            // our integer solution is not good enough, so we try to get a better solution with SA
            for (int iterations = 1000000; upper_bound > goal; iterations *= 2) {
                std::istringstream in2(data);
                std::ostringstream ss;
                upper_bound = sa_main(in2, ss, &columns, iterations);
                sa_forest = ss.str();
            }

            cout << sa_forest;
            return 0;
        }

        auto covered = std::vector<bool>(state.nrof_leaves);
        for (int i = 0; i < column_index; i++) {
            if (columns[i].var->solution_value() > 0.5001) {
                std::cout << columns[i].solution << "\n";
                for (auto leaf : columns[i].leaves) {
                    covered[leaf-1] = true;
                }
            }
        }

        for (int i = 0; i < state.nrof_leaves; i++) {
            if (!covered[i])  std::cout << i+1 << ";\n";
        }
    }
#else
    { // ILP optimization
        struct RMP rmp;
        rmp.solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("cbc"));
        auto _ = rmp.solver->SetNumThreads(1);
        if (!rmp.solver) {
            LOG(INFO) << "cbc solver unavailable.";
            return 1;
        }
        rmp.constraints = std::vector<MPConstraint*>(nrof_constraints);
        for (int j = 0; j < nrof_constraints; j++) {
            rmp.constraints[j] = rmp.solver->MakeRowConstraint(0.0, 1.0);
        }
        rmp.objective = rmp.solver->MutableObjective();
        rmp.objective->SetMaximization();

        for (int i = 0; i < columns.size(); i++) {
            columns[i].var = rmp.solver->MakeIntVar(0, 1, "");

            for (int node : columns[i].internal_nodes) {
                rmp.constraints[node]->SetCoefficient(columns[i].var, 1);
            }

            rmp.objective->SetCoefficient(columns[i].var, columns[i].connections);
        }
        LOG(INFO) << "Staring ILP solver with " << columns.size() << " columns";

        auto start = std::chrono::high_resolution_clock::now();
        rmp.solver->SetTimeLimit(absl::Seconds(290 - std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - program_start).count()));
        auto result_status = rmp.solver->Solve();
        auto end = std::chrono::high_resolution_clock::now();
        LOG(INFO) << "IP solve time: " << std::chrono::duration<double>(end - start).count();
        if (result_status != MPSolver::OPTIMAL && result_status != MPSolver::FEASIBLE) {
            LOG(INFO) << "No ILP solution found.";
            cout << sa_forest;
            return 0;
        }

        LOG(INFO) << "Total cost = " << state.nrof_leaves - rmp.objective->Value();

        if (state.nrof_leaves - rmp.objective->Value() < upper_bound) {
            auto covered = std::vector<bool>(state.nrof_leaves);
            for (int i = 0; i < column_index; i++) {
                if (columns[i].var->solution_value() > 0.5001) {
                    std::cout << columns[i].solution << "\n";
                    for (auto leaf : columns[i].leaves) {
                        covered[leaf-1] = true;
                    }
                }
            }

            for (int i = 0; i < state.nrof_leaves; i++) {
                if (!covered[i])  std::cout << i+1 << ";\n";
            }
        } else {
            cout << sa_forest;
        }
    }
#endif

    return 0;
}