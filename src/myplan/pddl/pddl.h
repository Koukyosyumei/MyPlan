#pragma once
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

class Type {
   public:
    Type(string name_ = "<NULL>", Type* parent_ = nullptr)
        : name(name_), parent(parent_) {
        transform(name.begin(), name.end(), name.begin(), ::tolower);
    }
    bool operator==(const Type& other) const { return name == other.name; }

    string name;
    // string parent;
    Type* parent;
};

class Predicate {
   public:
    Predicate(){};
    Predicate(string name, vector<pair<string, vector<Type*>>> signature)
        : name(name), signature(signature) {}

    string name;
    vector<pair<string, vector<Type*>>> signature;

    bool operator<(const Predicate& rhs) const {
        if (name != rhs.name) {
            return name < rhs.name;
        }
        if (signature.size() != rhs.signature.size()) {
            return signature.size() < rhs.signature.size();
        }
        for (int i = 0; i < signature.size(); i++) {
            if (signature[i].first < rhs.signature[i].first) {
                return true;
            }
        }
        return false;
    }
    bool operator==(const Predicate& rhs) const {
        if (name != rhs.name) {
            return false;
        }
        if (signature.size() != rhs.signature.size()) {
            return false;
        } else {
            for (int i = 0; i < signature.size(); i++) {
                if (signature[i].first != rhs.signature[i].first) {
                    return false;
                }
            }
        }
        return true;
    }
};

class Effect {
   public:
    Effect(){};
    void addToAddList(Predicate& pred) { addlist.insert(pred); }
    void addToDelList(Predicate& pred) { dellist.insert(pred); }

    set<Predicate> addlist;
    set<Predicate> dellist;
};

class Action {
   public:
    Action(){};
    Action(string name, vector<pair<string, vector<Type*>>> signature,
           vector<Predicate> precondition, Effect effect)
        : name(name),
          signature(signature),
          precondition(precondition),
          effect(effect) {}

    string name;
    vector<pair<string, vector<Type*>>> signature;
    vector<Predicate> precondition;
    Effect effect;
};

class Domain {
   public:
    Domain(){};
    Domain(string name_, unordered_map<string, Type*>& types_,
           vector<Predicate> predicates_, vector<Action> actions_,
           unordered_map<string, Type*>& constants_)
        : name(name_),
          types(types_),
          predicates(predicates_),
          actions(actions_),
          constants(constants_) {}

    string name;
    unordered_map<string, Type*> types;
    vector<Predicate> predicates;
    unordered_map<string, Predicate> predicates_dict;
    vector<Action> actions;
    unordered_map<string, Action> actions_dict;
    unordered_map<string, Type*> constants;
};

class Problem {
   public:
    Problem(){};
    Problem(std::string name, Domain* domain,
            std::unordered_map<std::string, Type*> objects,
            std::vector<Predicate> init, std::vector<Predicate> goal)
        : name(name),
          domain(domain),
          objects(objects),
          init(init),
          goal(goal) {}

    std::string name;
    Domain* domain;
    std::unordered_map<std::string, Type*> objects;
    std::vector<Predicate> init;
    std::vector<Predicate> goal;
};
