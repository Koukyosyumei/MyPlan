#include <gtest/gtest.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "myplan/pddl/lisp_iterators.h"
#include "myplan/pddl/lisp_parser.h"
#include "myplan/pddl/parser.h"

TEST(parser_pddl_comple, Action) {
    std::string test =
        "(:action pick-up :parameters (?x - block) :precondition (and (clear "
        "?x) (ontable ?x) (handempty)) :effect (and (not (ontable ?x)) (not "
        "(clear ?x)) (not (handempty)) (holding ?x)))";

    LispIterator iter = parse_lisp_iterator({test});
    ActionStmt action = parse_action_stmt(iter);
    ASSERT_EQ(action.name, "pick-up");
    ASSERT_EQ(action.parameters[0].name, "?x");
    ASSERT_EQ(action.parameters[0].types[0], "block");

    Formula pre = action.precond.formula;
    ASSERT_EQ(pre.key, "and");
    ASSERT_EQ(pre.children[0].key, "clear");
    ASSERT_EQ(pre.children[1].key, "ontable");
    ASSERT_EQ(pre.children[2].key, "handempty");
    ASSERT_EQ(pre.children[0].children[0].key, "?x");
    ASSERT_EQ(pre.children[1].children[0].key, "?x");
    ASSERT_EQ(pre.children[2].children.size(), 0);

    Formula eff = action.effect.formula;
    ASSERT_EQ(eff.key, "and");
    ASSERT_EQ(eff.children[0].key, "not");
    ASSERT_EQ(eff.children[1].key, "not");
    ASSERT_EQ(eff.children[2].key, "not");
    ASSERT_EQ(eff.children[3].key, "holding");
    ASSERT_EQ(eff.children[0].children[0].key, "ontable");
    for (Formula c : eff.children[0].children[0].children) {
        ASSERT_EQ(c.key, "?x");
    }
}

TEST(parser_pddl_complex, Predicates) {
    std::string test =
        "(:predicates (on ?x - block ?y - block) (ontable ?x - block) (clear "
        "?x - plane) (handempty) (holding ?x - block) )";
    LispIterator iter = parse_lisp_iterator({test});
    PredicatesStmt pred = parse_predicates_stmt(iter);
    std::vector<std::string> test_name_1 = {"on", "ontable", "clear",
                                            "handempty", "holding"};
    for (int i = 0; i < test_name_1.size(); i++) {
        ASSERT_EQ(pred.predicates[i].name, test_name_1[i]);
    }
    for (auto p : pred.predicates) {
        if (p.parameters.size() != 0) {
            ASSERT_EQ(p.parameters[0].name, "?x");
        }
    }
    std::vector<std::string> test_name_2 = {"block", "block", "plane", "block"};
    int tmp_i = -1;
    for (auto p : pred.predicates) {
        if (p.parameters.size() != 0) {
            tmp_i++;
            ASSERT_EQ(p.parameters[0].types[0], test_name_2[tmp_i]);
        }
    }
    for (auto p : pred.predicates) {
        if (p.parameters.size() > 1) {
            ASSERT_EQ(p.parameters[1].types[0], "block");
        }
    }
}

TEST(parser_pddl_complex, Types) {
    std::string test =
        "(:types truck airplane - vehicle package vehicle - physobj airport "
        "location - place city place physobj - object)";

    LispIterator iter = parse_lisp_iterator({test});
    std::vector<Type*> types = parse_types_stmt(iter);
    std::vector<std::string> test_names = {"truck",   "airplane", "package",
                                           "vehicle", "airport",  "location",
                                           "city",    "place",    "physobj"};
    std::vector<std::string> test_types = {
        "vehicle", "vehicle", "physobj", "physobj", "place",
        "place",   "object",  "object",  "object",
    };
    for (int i = 0; i < types.size(); i++) {
        ASSERT_EQ(types[i]->name, test_names[i]);
        ASSERT_EQ(types[i]->parent->name, test_types[i]);
    }
}

TEST(parser_pddl_complex, PredicatesLogistics) {
    std::string test =
        "(:predicates  (in-city ?loc - place ?city - city) (at ?obj - physobj "
        "?loc - place) (in ?pkg - package ?veh - vehicle))";

    LispIterator iter = parse_lisp_iterator({test});
    PredicatesStmt pred = parse_predicates_stmt(iter);
    std::vector<std::string> test_names = {"in-city", "at", "in"};
    std::vector<std::string> test_parameters_names = {"?loc", "?obj", "?pkg"};
    std::vector<std::string> test_parameters_types = {"place", "physobj",
                                                      "package"};
    for (int i = 0; i < pred.predicates.size(); i++) {
        ASSERT_EQ(pred.predicates[i].name, test_names[i]);
        ASSERT_EQ(pred.predicates[i].parameters[0].name,
                  test_parameters_names[i]);
        ASSERT_EQ(pred.predicates[i].parameters[0].types[0],
                  test_parameters_types[i]);
    }
}

TEST(parser_pddl_complex, DomainDef) {
    std::vector<std::string> test = {
        " (define (domain BLOCKS)",
        " (:requirements :strips :typing)",
        " (:types block)",
        " (:predicates (on ?x - block ?y - block)",
        "              (ontable ?x - block)",
        "              (clear ?x - block)",
        "              (handempty)",
        "              (holding ?x - block)",
        "              )",
        " (:action pick-up",
        "            :parameters (?x - block)",
        "            :precondition (and (clear ?x) (ontable ?x) (handempty))",
        "            :effect",
        "            (and (not (ontable ?x))",
        "                  (not (clear ?x))",
        "                  (not (handempty))",
        "                  (holding ?x)))",
        " (:action put-down",
        "            :parameters (?x - block)",
        "            :precondition (holding ?x)",
        "            :effect",
        "            (and (not (holding ?x))",
        "                  (clear ?x)",
        "                  (handempty)",
        "                  (ontable ?x)))",
        " (:action stack",
        "            :parameters (?x - block ?y - block)",
        "            :precondition (and (holding ?x) (clear ?y))",
        "            :effect",
        "            (and (not (holding ?x))",
        "                  (not (clear ?y))",
        "                  (clear ?x)",
        "                  (handempty)",
        "                  (on ?x ?y)))",
        " (:action unstack",
        "            :parameters (?x - block ?y - block)",
        "            :precondition (and (on ?x ?y) (clear ?x) (handempty))",
        "            :effect",
        "            (and (holding ?x)",
        "                  (clear ?y)",
        "                  (not (clear ?x))",
        "                  (not (handempty))",
        "                  (not (on ?x ?y)))))"};

    LispIterator iter = parse_lisp_iterator(test);
    DomainDef dom = parse_domain_def(iter);

    ASSERT_EQ(dom.name, "blocks");
    ASSERT_EQ(dom.requirements.keywords[0].name, "strips");
    ASSERT_EQ(dom.requirements.keywords[1].name, "typing");
    ASSERT_EQ(dom.types[0]->name, "block");

    PredicatesStmt pred = dom.predicates;
    std::vector<std::string> test_names_1 = {"on", "ontable", "clear",
                                             "handempty", "holding"};
    for (int i = 0; i < pred.predicates.size(); i++) {
        ASSERT_EQ(pred.predicates[i].name, test_names_1[i]);
        if (pred.predicates[i].parameters.size() != 0) {
            ASSERT_EQ(pred.predicates[i].parameters[0].name, "?x");
            ASSERT_EQ(pred.predicates[i].parameters[0].types[0], "block");
        }
        if (pred.predicates[i].parameters.size() > 1) {
            ASSERT_EQ(pred.predicates[i].parameters[1].types[0], "block");
        }
    }
    ASSERT_EQ(dom.actions.size(), 4);
    ActionStmt action = dom.actions[3];
    ASSERT_EQ(action.name, "unstack");
    ASSERT_EQ(action.parameters[0].name, "?x");
    ASSERT_EQ(action.parameters[0].types[0], "block");
    ASSERT_EQ(action.parameters[1].name, "?y");
    ASSERT_EQ(action.parameters[1].types[0], "block");

    Formula pre = action.precond.formula;
    ASSERT_EQ(pre.key, "and");
    std::vector<std::string> test_key_1 = {"on", "clear", "handempty"};
    for (int i = 0; i < test_key_1.size(); i++) {
        ASSERT_EQ(pre.children[i].key, test_key_1[i]);
    }
    ASSERT_EQ(pre.children[0].children[0].key, "?x");
    ASSERT_EQ(pre.children[0].children[1].key, "?y");
    ASSERT_EQ(pre.children[1].children[0].key, "?x");
    ASSERT_EQ(pre.children[2].children.size(), 0);

    Formula eff = action.effect.formula;
    ASSERT_EQ(eff.key, "and");
    std::vector<std::string> test_key_2 = {"holding", "clear", "not", "not",
                                           "not"};
    for (int i = 0; i < test_key_2.size(); i++) {
        ASSERT_EQ(eff.children[i].key, test_key_2[i]);
    }
}

TEST(parser_pddl_complex, predList2) {
    std::vector<std::string> test = {
        "(:predicates (at ?x - (either person aircraft) ?c - city)",
        "     (in ?p - person ?a - aircraft)",
        "     (fuel-level ?a - aircraft ?l - flevel)",
        "     (next ?l1 ?l2 - flevel))"};
    LispIterator iter = parse_lisp_iterator(test);
    PredicatesStmt pred = parse_predicates_stmt(iter);
    std::vector<std::string> test_name_1 = {"at", "in", "fuel-level", "next"};
    for (int i = 0; i < pred.predicates.size(); i++) {
        ASSERT_EQ(pred.predicates[i].name, test_name_1[i]);
    }
    std::vector<std::string> test_name_2 = {"?x", "?p", "?a", "?l1"};
    for (int i = 0; i < pred.predicates.size(); i++) {
        ASSERT_EQ(pred.predicates[i].parameters[0].name, test_name_2[i]);
    }
    std::vector<std::string> test_name_3 = {"person", "person", "aircraft",
                                            "flevel"};
    for (int i = 0; i < pred.predicates.size(); i++) {
        ASSERT_EQ(pred.predicates[i].parameters[0].types[0], test_name_3[i]);
    }
}

TEST(parser_pddl_complex, ObjectStmt) {
    std::vector<std::string> test = {
        "(:objects",
        "apn1 - airplane",
        "apt2 apt1 - airport",
        "pos2 pos1 - location",
        "cit2 cit1 - city",
        "tru2 tru1 - truck",
        "obj23 obj22 obj21 obj13 obj12 obj11 - package)"};
    LispIterator iter = parse_lisp_iterator(test);
    std::vector<Object> objects = parse_objects_stmt(iter);
    std::vector<std::string> test_names = {
        "apn1", "apt2",  "apt1",  "pos2",  "pos1",  "cit2",  "cit1", "tru2",
        "tru1", "obj23", "obj22", "obj21", "obj13", "obj12", "obj11"};
    for (int i = 0; i < objects.size(); i++) {
        ASSERT_EQ(objects[i].name, test_names[i]);
    }
}

TEST(parser_pddl_complex, InitStmt) {
    std::vector<std::string> test = {
        "(:INIT (CLEAR C) (CLEAR A H) (CLEAR B) (CLEAR D) (ONTABLE C) (ONTABLE "
        "A)",
        "(ONTABLE B) (ONTABLE D) (HANDEMPTY))"};
    LispIterator iter = parse_lisp_iterator(test);
    InitStmt init = parse_init_stmt(iter);
    std::vector<std::string> test_names = {"clear",   "clear",   "clear",
                                           "clear",   "ontable", "ontable",
                                           "ontable", "ontable", "handempty"};
    for (int i = 0; i < init.predicates.size(); i++) {
        ASSERT_EQ(init.predicates[i].name, test_names[i]);
    }
    std::vector<std::vector<std::string>> test_params = {
        {"c"}, {"a", "h"}, {"b"}, {"d"}, {"c"}, {"a"}, {"b"}, {"d"}, {}};
    for (int i = 0; i < init.predicates.size(); i++) {
        for (int j = 0; j < init.predicates[i].parameters.size(); j++) {
            ASSERT_EQ(init.predicates[i].parameters[j], test_params[i][j]);
        }
    }
}

TEST(parser_pddl_complex, GoalStmt) {
    std::vector<std::string> test = {
        "(:goal (AND (ON D C) (ON C B) (ON B A)))"};
    LispIterator iter = parse_lisp_iterator(test);

    GoalStmt goal = parse_goal_stmt(iter);
    Formula f = goal.formula;
    ASSERT_EQ(f.key, "and");
    std::vector<std::string> test_key_1 = {"on", "on", "on"};
    std::vector<std::string> test_key_2 = {"d", "c", "b"};
    std::vector<std::string> test_key_3 = {"c", "b", "a"};
    for (int i = 0; i < f.children.size(); i++) {
        ASSERT_EQ(f.children[i].key, test_key_1[i]);
    }

    std::vector<std::string> c2_0;
    std::vector<std::string> c2_1;
    std::vector<std::vector<Formula>> children;
    for (int i = 0; i < f.children.size(); i++) {
        ASSERT_EQ(f.children[i].key, test_key_1[i]);
        children.push_back(f.children[i].children);
    }
    for (int i = 0; i < children.size(); i++) {
        ASSERT_EQ(children[i][0].key, test_key_2[i]);
        ASSERT_EQ(children[i][1].key, test_key_3[i]);
    }
}

TEST(parser_pddl_complex, Constants) {
    std::string test =
        "(:constants north south - direction light medium heavy - airplanetype "
        "seg_pp_0_60 seg_ppdoor_0_40 seg_tww1_0_200 seg_twe1_0_200 "
        "seg_tww2_0_50 seg_tww3_0_50 seg_tww4_0_50 seg_rww_0_50 seg_rwtw1_0_10 "
        "seg_rw_0_400 seg_rwe_0_50 seg_twe4_0_50 seg_rwte1_0_10 seg_twe3_0_50 "
        "seg_twe2_0_50 seg_rwte2_0_10 seg_rwtw2_0_10 - segment airplane_CFBEG "
        "- airplane)";
    LispIterator iter = parse_lisp_iterator({test});
    std::vector<Object> constants = parse_constants_stmt(iter);
    std::vector<std::string> nameList = {
        "north",          "south",          "light",           "medium",
        "heavy",          "seg_pp_0_60",    "seg_ppdoor_0_40", "seg_tww1_0_200",
        "seg_twe1_0_200", "seg_tww2_0_50",  "seg_tww3_0_50",   "seg_tww4_0_50",
        "seg_rww_0_50",   "seg_rwtw1_0_10", "seg_rw_0_400",    "seg_rwe_0_50",
        "seg_twe4_0_50",  "seg_rwte1_0_10", "seg_twe3_0_50",   "seg_twe2_0_50",
        "seg_rwte2_0_10", "seg_rwtw2_0_10", "airplane_CFBEG"};
    for (int i = 0; i < constants.size(); i++) {
        std::string tmp_name = nameList[i];
        std::transform(tmp_name.begin(), tmp_name.end(), tmp_name.begin(),
                       ::tolower);
        ASSERT_EQ(constants[i].name, tmp_name);
    }
    std::vector<std::string> typeList = {"direction", "direction",
                                         "airplanetype", "airplanetype",
                                         "airplanetype"};
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(constants[i].typeName, typeList[i]);
    }
}

TEST(parser_pddl_complex, ProblemDef) {
    std::vector<std::string> test = {
        "(define (problem logistics-4-1)",
        "(:domain logistics)",
        "(:objects",
        "  apn1 - airplane",
        "  apt2 apt1 - airport",
        "  pos2 pos1 - location",
        "  cit2 cit1 - city",
        "  tru2 tru1 - truck",
        "  obj23 obj22 obj21 obj13 obj12 obj11 - package)",
        "(:init (at apn1 apt2) (at tru1 pos1) (at obj11 pos1)",
        " (at obj12 pos1) (at obj13 pos1) (at tru2 pos2) (at obj21 pos2)",
        " (at obj22 pos2) (at obj23 pos2) (in-city pos1 cit1) (in-city apt1 "
        "cit1)",
        " (in-city pos2 cit2) (in-city apt2 cit2))",
        "(:goal (and (at obj12 apt2) (at obj13 apt1) (at obj21 apt2)",
        "            (at obj11 pos2)))",
        ")    "};
    LispIterator iter = parse_lisp_iterator(test);
    ProblemDef prob = parse_problem_def(iter);
    ASSERT_EQ(prob.name, "logistics-4-1");
    ASSERT_EQ(prob.domainName, "logistics");
    std::vector<std::string> test_objects = {
        "apn1", "apt2",  "apt1",  "pos2",  "pos1",  "cit2",  "cit1", "tru2",
        "tru1", "obj23", "obj22", "obj21", "obj13", "obj12", "obj11"};
    for (int i = 0; i < prob.objects.size(); i++) {
        ASSERT_EQ(prob.objects[i].name, test_objects[i]);
    }
    ASSERT_EQ(prob.init.predicates.size(), 13);
}
