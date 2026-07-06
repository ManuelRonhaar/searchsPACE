#include "sa.h"

struct Node {
    // all are indexes into node array
    int parent; // -1 if root / 0 if unused
    int left;
    int right;
    int depth; // 0 if root/unused, increasing with depth
    int label; // 0 if non-leaf
    int solution_index; // to solution forest for nodes in input trees / to solution tree root for nodes in solution
    int lp_index;
};

struct State {
    int nrof_leaves;
    Node* tree1;
    Node* tree2;
    Node* solution;
};

double random_get_zero_to_one() {
    return ((double) rand() / (RAND_MAX));
}

void print_forest(Node *array, int nrof_nodes);
int disconnect_solution(State& state, int node);
void set_depth_and_root(Node *solution, int node, int depth, int root);

void sa_read_tree(int nrof_leaves, Node *nodes, int lp_index, std::istream& in) {
    int current = 2*nrof_leaves - 1;
    int next = current - 1;

    nodes[current].parent = -1;
    nodes[current].depth = 0;
    nodes[current].lp_index = lp_index++;

    while (1) {
        char c = in.get();
        switch (c) {
            case '(':
                nodes[next].parent = current;
                nodes[current].left = next;
                nodes[next].depth = nodes[current].depth + 1;
                nodes[next].lp_index = lp_index++;
                current = next;
                next -= 1;
                if (next == nrof_leaves)  next = 0;
                break;
            case ')':
                current = nodes[current].parent;
                break;
            case ',':
            {
                int p = nodes[current].parent;
                nodes[next].parent = p;
                nodes[p].right = next;
                nodes[next].depth = nodes[p].depth + 1;
                nodes[next].lp_index = lp_index++;
                current = next;
                next -= 1;
                if (next == nrof_leaves)  next = 0;
                break;
            }
            case ';':
                return;
            default: // integer
                in.unget();
                int label;
                in >> label;
                nodes[label].label = label;
                nodes[label].parent = nodes[current].parent;
                nodes[label].solution_index = label;
                if (nodes[nodes[label].parent].left == current) {
                    nodes[nodes[label].parent].left = label;
                } else {
                    nodes[nodes[label].parent].right = label;
                }
                nodes[label].depth = nodes[current].depth;
                next = current;
                current = label;
                lp_index--;
                break;
        }
    }
}

void make_forest(int nrof_leaves, Node *nodes) {
    for (int i = 1; i <= nrof_leaves; i++) {
        nodes[i].label = i;
        nodes[i].parent = -1;
        nodes[i].solution_index = i;
    }

    // The empty nodes are a singly linked list starting from 0
    nodes[0].left = nrof_leaves + 1;
    for (int i = nrof_leaves+1; i <= 2*nrof_leaves-2; i++) {
        nodes[i].left = i+1;
    }
}

int get_new_node(Node *solution) {
    int ret = solution[0].left;
    solution[0].left = solution[ret].left;
    return ret;
}

void remove_node(Node *solution, int node) {
    solution[node].parent = 0;
    solution[node].right = 0;
    solution[node].depth = 0;
    solution[node].solution_index = 0;
    solution[node].left = solution[0].left;
    solution[0].left = node;
}

constexpr auto max_size = std::numeric_limits<std::streamsize>::max();
State sa_read_input(std::istream& in) {
    State state;
    int read_first_tree = 0;

    while (char c = in.get()) {
        if (c == '#' && in.get() == 'p') {
            int nrof_trees;
            in >> nrof_trees >> state.nrof_leaves;
            state.tree1 = (Node *) calloc(1, 6*state.nrof_leaves*sizeof(Node));
            state.tree2 = state.tree1 + 2*state.nrof_leaves;
            state.solution = state.tree2 + 2*state.nrof_leaves;
            in.ignore(max_size, '\n');
        } else if (c == '#') {
            in.ignore(max_size, '\n');
        } else if (!read_first_tree) {
            in.unget();
            sa_read_tree(state.nrof_leaves, state.tree1, 0, in);
            read_first_tree = 1;
            in.ignore(max_size, '\n');
        } else {
            in.unget();
            sa_read_tree(state.nrof_leaves, state.tree2, state.nrof_leaves-1, in);
            in.ignore(max_size, '\n');
            break;
        }
    }

    make_forest(state.nrof_leaves, state.solution);
    return state;
}

void print_node(std::ostream& out, Node *array, int index) {
    if (array[index].label == 0) {
        out << "(";
        print_node(out, array, array[index].left);
        out << ",";
        print_node(out, array, array[index].right);
        out << ")";
    } else {
        out << array[index].label;
    }
}

void print_forest(std::ostream& out, Node *array, int nrof_nodes) {
    for (int i = 0; i < nrof_nodes; i++) {
        if (array[i].parent == -1) {
            print_node(out, array, i);
            out << ";\n";
        }
    }
}

// find the index, in the solution tree containing label1, that's on the path from label1 to label2 with the lowest depth, and similarly an index in the solution tree containing label2
std::pair<int, int> find_connecting_node(State state, Node *tree, int label1, int label2) {
    int solution1 = label1;
    int solution2 = label2;
    int node1 = label1;
    int node2 = label2;
    int solution_tree1 = state.solution[label1].solution_index;
    int solution_tree2 = state.solution[label2].solution_index;

    while (node1 != node2) {
        if (tree[node1].depth >= tree[node2].depth) {
            node1 = tree[node1].parent;
            if (state.solution[solution1].parent != -1 && tree[node1].solution_index != 0) {
                solution1 = tree[node1].solution_index;
            } else if (state.solution[tree[node1].solution_index].solution_index == solution_tree2) {
                // we have reached the solution tree of label2 while climbing from label1
                return {solution1, tree[node1].solution_index};
            }
        } else {
            node2 = tree[node2].parent;
            if (state.solution[solution2].parent != -1 && tree[node2].solution_index != 0) {
                solution2 = tree[node2].solution_index;
            } else if (state.solution[tree[node2].solution_index].solution_index == solution_tree1) {
                // we have reached the solution tree of label1 while climbing from label2
                return {tree[node2].solution_index, solution2};
            }
        }
    }

    return {solution1, solution2};
}

// update tree with the connection from label1 to label2
void connect_tree(State state, Node *tree, int label1, int label2) {
    int solution1 = label1;
    int solution2 = label2;
    int node1 = label1;
    int node2 = label2;

    int solution_tree = state.solution[label1].solution_index; // label1 and label2 are in the same solution tree already

    while (tree[node1].parent != tree[node2].parent) {
        if (tree[node1].depth >= tree[node2].depth) {
            node1 = tree[node1].parent;
            int sol_index = tree[node1].solution_index;

            if (sol_index == 0 || sol_index == solution1 || state.solution[sol_index].parent == solution1) {
                // do nothing
            } else if (state.solution[sol_index].solution_index == solution_tree) {
                solution1 = state.solution[solution1].parent;
            } else {
                disconnect_solution(state, sol_index);
            }

            tree[node1].solution_index = solution1;
        } else {
            node2 = tree[node2].parent;
            int sol_index = tree[node2].solution_index;

            if (sol_index == 0 || sol_index == solution2 || state.solution[sol_index].parent == solution2) {
                // do nothing
            } else if (state.solution[sol_index].solution_index == solution_tree) {
                solution2 = state.solution[solution2].parent;
            } else {
                disconnect_solution(state, sol_index);
            }

            tree[node2].solution_index = solution2;
        }
    }

    int root = tree[node1].parent;
    int old_solution = tree[root].solution_index;
    tree[root].solution_index = state.solution[solution1].parent;
    while (tree[root].parent != -1 && tree[tree[root].parent].solution_index == old_solution &&old_solution != 0) {
        root = tree[root].parent;
        tree[root].solution_index = state.solution[solution1].parent;
    }
}

int disconnect_tree(State state, Node *tree, int node) {
    // we don't store solution to tree indexes so updating the trees is a bit awkard..
    int node_child = node;
    while (state.solution[node_child].label == 0) {
        node_child = state.solution[node_child].left;
    }
    int tree_node = node_child;
    while (tree[tree_node].solution_index != node) {
        tree_node = tree[tree_node].parent;
    }
    // tree_node is now the deepest node with solution_index == node

    tree_node = tree[tree_node].parent;
    while (tree[tree_node].solution_index == node) {
        tree[tree_node].solution_index = 0;
        tree_node = tree[tree_node].parent;
    }
    // tree_node is now the deepest node with solution_index == state.solution[node].parent

    return tree_node;
}

void replace_solution_index(Node *tree, int node, int new_index) {
    int tree_node = node;
    int old_index = tree[tree_node].solution_index;

    while (tree[tree_node].solution_index == old_index) {
        tree[tree_node].solution_index = new_index;
        tree_node = tree[tree_node].parent;
    }
}

// remove edge node-parent from the solution tree and update trees accordingly
int disconnect_solution(State& state, int node) {
    int parent1 = disconnect_tree(state, state.tree1, node);
    int parent2 = disconnect_tree(state, state.tree2, node);

    int parent = state.solution[node].parent;

    int sibling = (state.solution[parent].right == node)
        ? state.solution[parent].left
        : state.solution[parent].right;

    // update grandparent of node with sibling of node becoming a child
    if (state.solution[parent].parent != -1) {
        int grandparent = state.solution[parent].parent;

        if (state.solution[grandparent].left == parent) {
            state.solution[grandparent].left = sibling;
        } else {
            state.solution[grandparent].right = sibling;
        }

        set_depth_and_root(state.solution, sibling,
                           state.solution[parent].depth,
                           state.solution[parent].solution_index);

        replace_solution_index(state.tree1, parent1, sibling);
        replace_solution_index(state.tree2, parent2, sibling);
    } else {
        disconnect_tree(state, state.tree1, sibling);
        disconnect_tree(state, state.tree2, sibling);

        state.tree1[parent1].solution_index = 0;
        state.tree2[parent2].solution_index = 0;

        set_depth_and_root(state.solution, sibling, 0, sibling);
    }

    state.solution[sibling].parent = state.solution[parent].parent;
    remove_node(state.solution, parent);

    state.solution[node].parent = -1;
    set_depth_and_root(state.solution, node, 0, node);

    return sibling;
}

void set_depth_and_root(Node *solution, int node, int depth, int root) {
    solution[node].depth = depth;
    solution[node].solution_index = root;

    if (solution[node].left != 0) {
        set_depth_and_root(solution, solution[node].left, depth + 1, root);
        set_depth_and_root(solution, solution[node].right, depth + 1, root);
    }
}

std::pair<int,int> find_deepest_common_ancestor(State& state, int node1, int node2) {
    int old_node1 = node1;

    while (node1 != node2) {
        if (state.solution[node1].depth >= state.solution[node2].depth) {
            old_node1 = node1;
            node1 = state.solution[node1].parent;
        } else {
            node2 = state.solution[node2].parent;
        }
    }

    return {node1, old_node1};
}

void cut_siblings_from_path(State& state, int from, int to) {
    while (from != to) {
        int parent = state.solution[from].parent;

        int sibling = (state.solution[parent].right == from)
            ? state.solution[parent].left
            : state.solution[parent].right;

        disconnect_solution(state, sibling);

        if (parent == to) break;
    }
}

void cut_impossible_siblings(State state, int label, int node1, int node2) {
    if (node1 == node2) {
        return;
    }

    if (state.solution[node2].depth < state.solution[node1].depth) {
        int temp = node1;
        node1 = node2;
        node2 = temp;
    }

    auto [label_node1_ancestor, c1] = find_deepest_common_ancestor(state, label, node1);
    auto [label_node2_ancestor, c2] = find_deepest_common_ancestor(state, label, node2);
    auto [node1_node2_ancestor, prev] = find_deepest_common_ancestor(state, node2, node1);

    if (node1_node2_ancestor == node1) {  // node2 is a descendant of node1
        if (label_node1_ancestor == node1 && label_node2_ancestor != node2) {
            // label attaches on a vertex on the path between node1 to node2 or at node1
            cut_siblings_from_path(state, c2, node1);
        } else {
            // label is outside the path, so we just need a contraction
            cut_siblings_from_path(state, node2, node1_node2_ancestor);
        }

        return;
    }
    // nodes are not descendants of each other

    if (state.solution[label_node1_ancestor].depth < state.solution[node1_node2_ancestor].depth) {
        // random choice of which branch to cut
        if (random_get_zero_to_one() < 0.5) {
            cut_siblings_from_path(state, node1, node1_node2_ancestor);
        } else {
            cut_siblings_from_path(state, node2, node1_node2_ancestor);
        }

        return;
    }

    // label is below the common ancestor
    if (state.solution[label_node1_ancestor].depth < state.solution[label_node2_ancestor].depth) {
        // label is is on the side of node2 from ancestor
        if (node2 == label_node2_ancestor) {
            cut_siblings_from_path(state, node2, node1_node2_ancestor);
        } else {
            cut_siblings_from_path(state, c2, node1_node2_ancestor);
        }
    } else {
        // label is is on the side of node1 from ancestor
        if (node1 == label_node1_ancestor) {
            cut_siblings_from_path(state, node1, node1_node2_ancestor);
        } else {
            cut_siblings_from_path(state, c1, node1_node2_ancestor);
        }
    }
}

void make_siblings(State& state, int node1, int node2) {
    int node = get_new_node(state.solution);
    state.solution[node].parent = -1;
    state.solution[node].depth = 0;
    state.solution[node].solution_index = node;
    assert(state.solution[node1].parent == -1 || state.solution[node2].parent == -1);

    if (state.solution[node1].parent != -1) {
        int parent = state.solution[node1].parent;
        if (state.solution[parent].left == node1) {
            state.solution[parent].left = node;
        } else {
            state.solution[parent].right = node;
        }
        state.solution[node].parent = parent;
        state.solution[node].depth = state.solution[node1].depth;
        state.solution[node].solution_index = state.solution[node1].solution_index;
    } else if (state.solution[node2].parent != -1) {
        int parent = state.solution[node2].parent;
        if (state.solution[parent].left == node2) {
            state.solution[parent].left = node;
        } else {
            state.solution[parent].right = node;
        }
        state.solution[node].parent = parent;
        state.solution[node].depth = state.solution[node2].depth;
        state.solution[node].solution_index = state.solution[node2].solution_index;
    }

    state.solution[node1].parent = node;
    state.solution[node2].parent = node;
    state.solution[node].left = node1;
    state.solution[node].right = node2;

    set_depth_and_root(state.solution, node, state.solution[node].depth, state.solution[node].solution_index);
}

void connect(State& state, int label1, int label2) {
    if (state.solution[label1].solution_index == state.solution[label2].solution_index) {
        // already in same solution tree
        return;
    }

    auto [tree1_node1, tree1_node2] = find_connecting_node(state, state.tree1, label1, label2);
    auto [tree2_node1, tree2_node2] = find_connecting_node(state, state.tree2, label1, label2);

    cut_impossible_siblings(state, label1, tree1_node1, tree2_node1);
    cut_impossible_siblings(state, label2, tree1_node2, tree2_node2);

    auto [node1, node2] = find_connecting_node(state, state.tree1, label1, label2);

    make_siblings(state, node1, node2);
    connect_tree(state, state.tree1, label1, label2);
    connect_tree(state, state.tree2, label1, label2);
}

void array_add_if_unique(std::vector<int>* vector, int item) {
    if (std::find(vector->begin(), vector->end(), item) == vector->end()) {
        vector->push_back(item);
    }
}

bool array_find(std::vector<int>* vector, int item) {
    return std::find(vector->begin(), vector->end(), item) != vector->end();
}

std::pair<int, int> find_connection_and_removal(
    State state,
    Node *tree,
    int label1,
    int label2,
    std::vector<int>* to_remove
) {
    int solution1 = label1;
    int solution2 = label2;
    int node1 = label1;
    int node2 = label2;
    int solution_tree1 = state.solution[label1].solution_index;
    int solution_tree2 = state.solution[label2].solution_index;

    while (node1 != node2) {
        if (tree[node1].depth >= tree[node2].depth) {
            node1 = tree[node1].parent;
            if (state.solution[solution1].parent != -1 && tree[node1].solution_index != 0) {
                solution1 = tree[node1].solution_index;
            } else if (state.solution[tree[node1].solution_index].solution_index == solution_tree2) {
                // we have reached the solution tree of label2 while climbing from label1
                return {solution1, tree[node1].solution_index};
            } else if (tree[node1].solution_index != 0) {
                // third solution tree encountered
                int node = tree[node1].solution_index;
                int parent = state.solution[node].parent;
                if (parent != -1 && state.solution[parent].parent == -1) {
                    // we add extra duplicate so we can remove duplicates better...
                    array_add_if_unique(to_remove, parent);
                }
                array_add_if_unique(to_remove, node);
            }
        } else {
            node2 = tree[node2].parent;
            if (state.solution[solution2].parent != -1 && tree[node2].solution_index != 0) {
                solution2 = tree[node2].solution_index;
            } else if (state.solution[tree[node2].solution_index].solution_index == solution_tree1) {
                return {tree[node2].solution_index, solution2};
            } else if (tree[node2].solution_index != 0) {
                int node = tree[node2].solution_index;
                int parent = state.solution[node].parent;
                if (parent != -1 &&
                    state.solution[parent].parent == -1) {
                    array_add_if_unique(to_remove, parent);
                }
                array_add_if_unique(to_remove, node);
            }
        }
    }

    return {solution1, solution2};
}

int count_solution_tree_distance(State state, int node1, int node2) {
    if (node1 == node2) return 0;

    if (state.solution[node2].depth < state.solution[node1].depth) {
        int temp = node1;
        node1 = node2;
        node2 = temp;
    }
    // if one of the nodes is a descendant of the other node2 is descendant

    auto [node1_node2_ancestor, _] = find_deepest_common_ancestor(state, node2, node1);

    if (node1_node2_ancestor == node1) {
        return state.solution[node2].depth - state.solution[node1].depth;
    }

    return state.solution[node2].depth + state.solution[node1].depth - 2 * state.solution[node1_node2_ancestor].depth - 1;
}

int forest_size_increase(State state, int label1, int label2) {
    static std::vector<int> to_remove;
    to_remove.clear();

    if (state.solution[label1].solution_index == state.solution[label2].solution_index) {
        return 1000;
    }

    auto [tree1_node1, tree1_node2] = find_connection_and_removal(state, state.tree1, label1, label2, &to_remove);
    auto [tree2_node1, tree2_node2] = find_connection_and_removal(state, state.tree2, label1, label2, &to_remove);
    int remove_count = static_cast<int>(to_remove.size());

    // If multiple edges are cut they may result is just one disconnection:
    // - if both the left and right child of a root are cut, there is just one disconnection, so we need to adjust
    // - if all tree edges of a node are cut, there are just two disconnections, again needing post adjustment
    // The first one only gets checked correctly as we also add the parent if its a root specifically for this purpose
    // TODO: performance: sort and binary search for n log n maybe
    for (int node : to_remove) {
        if (state.solution[node].parent == -1 &&
                (array_find(&to_remove, state.solution[node].right) ||
                 array_find(&to_remove, state.solution[node].left))) {
            remove_count -= 1;
        }

        if (array_find(&to_remove, state.solution[node].right) &&
                array_find(&to_remove, state.solution[node].left)) {
            remove_count -= 1;
        }
    }

    remove_count += count_solution_tree_distance(state, tree1_node1, tree2_node1);
    remove_count += count_solution_tree_distance(state, tree1_node2, tree2_node2);

    return remove_count - 1;
}

std::pair<int, int> get_somewhat_random_leaves(State state) {
    Node *tree = state.tree1;
    if (random_get_zero_to_one() < 0.5) {
        tree = state.tree2;
    }

    int left;
    int right;
    do {
        int nrof_non_leaves = state.nrof_leaves - 1;
        left = (int(rand()) % nrof_non_leaves) + nrof_non_leaves + 2;
        right = tree[left].right;
        left = tree[left].left;
    } while (tree[left].solution_index != 0 && tree[left].solution_index == tree[right].solution_index);

    while (tree[left].solution_index == 0) {
        if (random_get_zero_to_one() < 0.5) {
            left = tree[left].left;
        } else {
            left = tree[left].right;
        }
    }

    while (tree[left].label == 0) {
        if (random_get_zero_to_one() < 0.5 && tree[tree[left].left].solution_index == tree[left].solution_index) {
            left = tree[left].left;
        } else if (tree[tree[left].right].solution_index == tree[left].solution_index) {
            left = tree[left].right;
        } else {
            left = tree[left].left;
        }
    }

    while (tree[right].solution_index == 0) {
        if (random_get_zero_to_one() < 0.5) {
            right = tree[right].left;
        } else {
            right = tree[right].right;
        }
    }

    while (tree[right].label == 0) {
        if (random_get_zero_to_one() < 0.5 && tree[tree[right].left].solution_index == tree[right].solution_index) {
            right = tree[right].left;
        } else if (tree[tree[right].right].solution_index == tree[right].solution_index) {
            right = tree[right].right;
        } else {
            right = tree[right].left;
        }
    }

    return {left, right};
}

void add_internal_nodes(Node* nodes, int index, int oindex, Column& column) {
    while (nodes[index].solution_index != oindex) {
        index = nodes[index].parent;
        column.internal_nodes.push_back(nodes[index].lp_index);
    }
}

// types: 0 -> no extra, 1 -> extra, 2 -> extra except first
void backtrack(State& state, Column& column, int index, int oindex, std::ostream& ss, std::vector<Column>* columns, int type) {
    if (state.solution[index].label == 0) {
        ss << "(";
        backtrack(state, column, state.solution[index].left, oindex, ss, columns, type ? 1 : 0);
        ss << ",";
        backtrack(state, column, state.solution[index].right, oindex, ss, columns, type ? 1 : 0);
        ss << ")";
        if (type == 1) {
            Column column;
            column.connections = -1;
            std::stringstream ss;
            backtrack(state, column, index, index, ss, columns, 0);
            ss << ";";
            column.solution = ss.str();
            columns->push_back(column);
        }
    } else {
        ss << state.solution[index].label;
        add_internal_nodes(state.tree1, state.solution[index].label, oindex, column);
        add_internal_nodes(state.tree2, state.solution[index].label, oindex, column);
        column.leaves.push_back(state.solution[index].label);
        column.connections += 1;
    }
}

void collect_columns(State& state, std::vector<Column>* columns) {
    for (int i = state.nrof_leaves+1; i < 2*state.nrof_leaves; i++) {
        if (state.solution[i].parent == -1 && state.solution[i].label == 0) {
            Column column;
            column.connections = -1;
            std::stringstream ss;
            backtrack(state, column, i, i, ss, columns, 2);
            ss << ";";
            column.solution = ss.str();
            columns->push_back(column);
        }
    }
}

int sa_main(std::istream& in, std::ostream& out, std::vector<Column>* columns, int nrof_iterations) {
    State state = sa_read_input(in);
    int current_size = state.nrof_leaves;
    int best = state.nrof_leaves;
    Node* answer = (Node *) calloc(1, 6*state.nrof_leaves*sizeof(Node));
    memcpy(answer, state.tree1, 6*state.nrof_leaves*sizeof(Node));
    srand (time(NULL));

    const int GEOMETRIC_CONST = 500;
    const double TEMP_START = 1;
    const double TEMP_END = 0.01;
    //const int nrof_iterations = 50000000;
    double t_alpha = exp(log(TEMP_END / TEMP_START) / GEOMETRIC_CONST);
    double temp = TEMP_START;
    int ti = 0;

    for (int i = 0; i < nrof_iterations; i++) {
        ti += 1;
        if (ti > nrof_iterations / GEOMETRIC_CONST) {
            ti = 0;
            temp *= t_alpha;
        }
        auto [x, y] = get_somewhat_random_leaves(state);
        int expected_change = forest_size_increase(state, x, y);
        double rnd = random_get_zero_to_one();
        if (expected_change <= 0 || rnd < exp(-expected_change / temp)) {
            connect(state, x, y);
            current_size += expected_change;

            if (current_size < best) {
                best = current_size;
                memcpy(answer, state.tree1, 6*state.nrof_leaves*sizeof(Node));
            }
        }
    }

    memcpy(state.tree1, answer, 6*state.nrof_leaves*sizeof(Node));
    print_forest(out, state.solution, 2*state.nrof_leaves);

    collect_columns(state, columns);

    return best;
}
