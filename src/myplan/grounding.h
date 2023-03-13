#pragma once
#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

#include "pddl/pddl.h"
#include "pddl/tree_visitor.h"
#include "pddl/visitable.h"
#include "task.h"

using namespace std;

std::vector<Operator> relevance_analysis(std::vector<Operator>& operators,
                                         std::unordered_set<std::string>& goals,
                                         bool verbose_logging) {
    /* This implements a relevance analysis of operators.
     * We start with all facts within the goal and iteratively compute
     * a fixpoint of all relevant effects.
     * Relevant effects are those that contribute to a valid path to the goal.
     */
    bool debug = true;
    std::set<std::string> relevant_facts;
    std::set<std::string> old_relevant_facts;
    std::unordered_set<Predicate> debug_pruned_op;
    bool changed = true;
    for (const auto& goal : goals) {
        relevant_facts.insert(goal);
    }

    while (changed) {
        // set next relevant facts to current facts
        // if we do not add anything in the following for loop
        // we have already found a fixpoint
        old_relevant_facts = relevant_facts;
        // compute cut of relevant facts with effects of all operators
        for (const auto& op : operators) {
            std::set<std::string> new_addlist;
            std::set_intersection(
                op.add_effects.begin(), op.add_effects.end(),
                relevant_facts.begin(), relevant_facts.end(),
                std::inserter(new_addlist, new_addlist.end()));
            std::set<std::string> new_dellist;
            std::set_intersection(
                op.del_effects.begin(), op.del_effects.end(),
                relevant_facts.begin(), relevant_facts.end(),
                std::inserter(new_dellist, new_dellist.end()));
            if (!new_addlist.empty() || !new_dellist.empty()) {
                // add all preconditions to relevant facts
                relevant_facts.insert(op.preconditions.begin(),
                                      op.preconditions.end());
            }
        }
        changed = old_relevant_facts != relevant_facts;
    }

    // delete all irrelevant effects
    std::unordered_set<Operator> del_operators;
    for (auto& op : operators) {
        // calculate new effects
        std::set<std::string> new_addlist;
        std::set_intersection(op.add_effects.begin(), op.add_effects.end(),
                              relevant_facts.begin(), relevant_facts.end(),
                              std::inserter(new_addlist, new_addlist.end()));
        std::set<std::string> new_dellist;
        std::set_intersection(op.del_effects.begin(), op.del_effects.end(),
                              relevant_facts.begin(), relevant_facts.end(),
                              std::inserter(new_dellist, new_dellist.end()));
        if (debug) {
            debug_pruned_op.insert(op.add_effects.begin(),
                                   op.add_effects.end());
            debug_pruned_op.insert(op.del_effects.begin(),
                                   op.del_effects.end());
        }
        // store new effects
        op.add_effects = new_addlist;
        op.del_effects = new_dellist;
        if (new_addlist.empty() && new_dellist.empty()) {
            if (verbose_logging) {
                std::cout << "Relevance analysis removed oparator " << op.name
                          << std::endl;
            }
            del_operators.insert(op);
        }
    }
    if (debug) {
        std::cout << "Relevance analysis removed " << debug_pruned_op.size()
                  << " facts" << std::endl;
    }
    // remove completely irrelevant operators
    std::vector<Operator> new_operators;
    new_operators.reserve(operators.size() - del_operators.size());
    for (const auto& op : operators) {
        if (!del_operators.count(op)) {
            new_operators.push_back(op);
        }
    }
    return new_operators;
}

std::vector<Predicate> _get_statics(const std::vector<Predicate>& predicates,
                                    const std::vector<Action>& actions) {
    std::vector<std::unordered_set<Predicate>> effects;
    std::transform(actions.begin(), actions.end(), back_inserter(effects),
                   [](const Action& a) {
                       return std::unordered_set<Predicate>{
                           a.effect.addlist.begin(), a.effect.addlist.end()};
                   });
    std::transform(actions.begin(), actions.end(), back_inserter(effects),
                   [](const Action& a) {
                       return std::unordered_set<Predicate>{
                           a.effect.dellist.begin(), a.effect.dellist.end()};
                   });
    std::unordered_set<Predicate> flattened_effects;
    for (const auto& es : effects) {
        flattened_effects.insert(es.begin(), es.end());
    }

    auto is_static = [&](const Predicate& pred) {
        return std::none_of(
            flattened_effects.begin(), flattened_effects.end(),
            [&](const Predicate& eff) { return pred.name == eff.name; });
    };

    std::vector<Predicate> statics;
    std::copy_if(predicates.begin(), predicates.end(), back_inserter(statics),
                 is_static);
    return statics;
}

std::unordered_map<Type, std::vector<std::string>> _create_type_map(
    const std::unordered_map<std::string, Type> objects) {
    std::unordered_map<Type, std::vector<std::string>> type_map;
    for (const auto& obj : objects) {
        std::string object_name = obj.first;
        Type object_type = obj.second;
        Type* parent_type = object_type.parent_ptr;

        while (true) {
            type_map[object_type].push_back(object_name);
            object_type = *parent_type;
            if (object_type.parent_ptr != nullptr) {
                parent_type = object_type.parent_ptr;
            } else {
                break;
            }
        }
    }
    return type_map;
}

std::unordered_set<std::string> _collect_facts(
    std::vector<Operator>& operators) {
    /*
    Collect all facts from grounded operators (precondition, add
    effects and delete effects).
    */
    std::unordered_set<std::string> facts;
    for (Operator& op : operators) {
        for (std::string p : op.preconditions) {
            facts.insert(p);
        }
        for (std::string p : op.add_effects) {
            facts.insert(p);
        }
        for (std::string p : op.del_effects) {
            facts.insert(p);
        }
    }
    return facts;
}

// Helper function to get grounded string
std::string _get_grounded_string(std::string name,
                                 std::vector<std::string> args) {
    std::string args_string = "";
    if (!args.empty()) {
        args_string += " ";
        for (auto arg : args) {
            args_string += arg + " ";
        }
    }
    return "(" + name + args_string + ")";
}

// Helper function to ground atom
std::string _ground_atom(
    const Predicate& atom,
    const std::unordered_map<std::string, std::string>& assignment) {
    std::vector<std::string> names;
    for (auto [name, types] : atom.signature) {
        if (assignment.count(name)) {
            names.push_back(assignment.at(name));
        } else {
            names.push_back(name);
        }
    }
    return _get_grounded_string(atom.name, names);
}

// Helper function to ground atoms
std::unordered_set<std::string> _ground_atoms(
    const set<Predicate>& atoms,
    const std::unordered_map<std::string, std::string>& assignment) {
    std::unordered_set<std::string> grounded_atoms;
    for (auto atom : atoms) {
        grounded_atoms.insert(_ground_atom(atom, assignment));
    }
    return grounded_atoms;
}

// Helper function to get fact string
std::string _get_fact(const Predicate& atom) {
    std::vector<std::string> args;
    for (auto [name, types] : atom.signature) {
        args.push_back(name);
    }
    return _get_grounded_string(atom.name, args);
}

// Helper function to get partial state
unordered_set<string> _get_partial_state(const vector<Predicate>& atoms) {
    unordered_set<string> partial_state;
    for (auto atom : atoms) {
        partial_state.insert(_get_fact(atom));
    }
    return partial_state;
}

bool _find_pred_in_init(string pred_name, string param, int sig_pos,
                        std::unordered_set<std::string> init) {
    /*
    This method is used to check whether an instantiation of the predicate
    denoted by pred_name with the parameter param at position sig_pos is
    present in the initial condition.
    Useful to evaluate static preconditions efficiently.
    */
    regex match_init;
    if (sig_pos == 0) {
        match_init = regex("\\(" + pred_name + " " + param + ".*");
    } else {
        string reg_ex = "\\(" + pred_name + "\\s+";
        for (int i = 0; i < sig_pos; i++) {
            reg_ex += "[\\w\\d-]+\\s+";
        }
        reg_ex += param + ".*";
        match_init = regex(reg_ex);
    }
    for (string str : init) {
        if (regex_match(str, match_init)) {
            return true;
        }
    }
    return false;
}

Operator* _create_operator(
    Action* action, std::unordered_map<std::string, std::string>& assignment,
    std::unordered_set<std::string>& statics,
    std::unordered_set<std::string>& init) {
    std::unordered_set<std::string> precondition_facts;
    for (Predicate precondition : action->precondition) {
        std::string fact = _ground_atom(precondition, assignment);
        std::string predicate_name = precondition.name;
        if (statics.count(predicate_name) > 0) {
            // Check if this precondition is false in the initial state
            if (init.count(fact) == 0) {
                // This precondition is never true -> Don't add operator
                return nullptr;
            }
        } else {
            // This precondition is not always true -> Add it
            precondition_facts.insert(fact);
        }
    }

    std::unordered_set<std::string> add_effects =
        _ground_atoms(action->effect.addlist, assignment);
    std::unordered_set<std::string> del_effects =
        _ground_atoms(action->effect.dellist, assignment);
    // If the same fact is added and deleted by an operator the STRIPS formalism
    // adds it.
    for (std::string fact : add_effects) {
        if (del_effects.count(fact) > 0) {
            del_effects.erase(fact);
        }
    }
    // If a fact is present in the precondition, we do not have to add it.
    // Note that if a fact is in the delete and in the add effects,
    // it has already been deleted in the previous step.
    for (std::string fact : precondition_facts) {
        if (add_effects.count(fact) > 0) {
            add_effects.erase(fact);
        }
    }
    std::vector<std::string> args;
    for (auto& [name, types] : action->signature) {
        args.push_back(assignment[name]);
    }
    std::string name = _get_grounded_string(action->name, args);
    return new Operator(name, precondition_facts, add_effects, del_effects);
}

std::vector<Operator> _ground_action(Action action, TypeMap type_map,
                                     std::vector<std::string> statics,
                                     std::unordered_set<std::string> init) {
    std::vector<Operator> operators;
    std::unordered_map<std::string, std::unordered_set<std::string>>
        param_to_objects;

    for (auto& [param_name, param_types] : action.signature) {
        // List of sets of objects for this parameter
        std::vector<std::unordered_set<std::string>> objects;
        for (auto& type : param_types) {
            objects.push_back(type_map[type]);
        }
        // Combine the sets into one set
        std::unordered_set<std::string> objects_set;
        for (auto& object_set : objects) {
            objects_set.insert(object_set.begin(), object_set.end());
        }
        param_to_objects[param_name] = objects_set;
    }

    // For each parameter that is not constant,
    // remove all invalid static preconditions
    int remove_debug = 0;
    for (auto& [param, objects] : param_to_objects) {
        for (auto& pred : action.precondition) {
            // if a static predicate is present in the precondition
            if (std::find(statics.begin(), statics.end(), pred.name) !=
                statics.end()) {
                int sig_pos = -1;
                int count = 0;
                // check if there is an instantiation with the current parameter
                for (auto& [var, _] : pred.signature) {
                    if (var == param) {
                        sig_pos = count;
                    }
                    count++;
                }
                if (sig_pos != -1) {
                    // remove if no instantiation present in initial state
                    std::unordered_set<std::string> obj_copy(objects);
                    for (auto& o : obj_copy) {
                        if (!_find_pred_in_init(pred.name, o, sig_pos, init)) {
                            // if (verbose_logging) {
                            //    remove_debug++;
                            //}
                            objects.erase(o);
                        }
                    }
                }
            }
        }
    }
    // if (verbose_logging) {
    //     std::cout << "Static precondition analysis removed " << remove_debug
    //               << " possible objects\n";
    // }

    // save a list of possible assignment tuples (param_name, object)
    std::vector<std::vector<std::pair<std::string, std::string>>> domain_lists;
    for (auto& [name, objects] : param_to_objects) {
        std::vector<std::pair<std::string, std::string>> tuples;
        for (auto& object : objects) {
            tuples.emplace_back(name, object);
        }
        domain_lists.push_back(tuples);
    }

    // Calculate all possible assignments
    auto assignments = cartesian_product(domain_lists);

    // Create a new operator for each possible assignment of parameters
    for (auto& assign : assignments) {
        Operator op = _create_operator(action, assign, statics, init);
        if (op) {
            operators.push_back(op);
        }
    }

    return operators;
}

std::vector<Operator> _ground_actions(
    std::vector<Action> actions,
    std::unordered_map<Type, std::vector<std::string>> type_map,
    std::vector<string> statics, std::unordered_set<std::string> init) {
    /*
    Ground a list of actions and return the resulting list of operators.
    @param actions: List of actions
    @param type_map: Mapping from type to objects of that type
    @param statics: Names of the static predicates
    @param init: Grounded initial state
    */
    std::vector<std::vector<Operator>> op_lists;
    for (Action action : actions) {
        op_lists.push_back(_ground_action(action, type_map, statics, init));
    }
    std::vector<Operator> operators;
    for (std::vector<Operator> op_list : op_lists) {
        operators.insert(operators.end(), op_list.begin(), op_list.end());
    }
    return operators;
}

Task ground(const Problem& problem,
            bool remove_statics_from_initial_state = true,
            bool remove_irrelevant_operators = true) {
    // Objects
    std::unordered_map<std::string, Type> objects = problem.objects;
    for (const auto& constant : problem.domain.constants) {
        objects.insert({constant.first, constant.second});
    }

    // Get the names of the static predicates
    auto statics =
        _get_statics(problem.domain.predicates, problem.domain.actions);

    // Create a map from types to objects
    auto type_map = _create_type_map(objects);

    // Transform initial state into a specific state
    auto init = _get_partial_state(problem.init);

    // Ground actions
    auto operators =
        _ground_actions(problem.domain.actions, type_map, statics, init);

    // Ground goal
    // TODO: Remove facts that can only become true and are true in the
    //       initial state
    // TODO: Return simple unsolvable problem if goal contains a fact that can
    //       only become false and is false in the initial state
    auto goals = _get_partial_state(problem.goal);

    // Collect facts from operators and include the ones from the goal
    auto facts = _collect_facts(operators);
    facts.insert(goals.begin(), goals.end());

    // Remove statics from initial state
    if (remove_statics_from_initial_state) {
        init &= facts;
    }

    // Perform relevance analysis
    if (remove_irrelevant_operators) {
        operators = relevance_analysis(operators, goals);
    }

    return Task(problem.name, facts, init, goals, operators);
}