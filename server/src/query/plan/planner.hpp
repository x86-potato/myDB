#include "../../config.h"
#include "../ast.hpp"
#include <memory>
#include <variant>

//this creates the 1d array plan
//for first iteration lets just build it to
// only support single path

struct Predicate;

struct Boolean
{
    AST::Op op; 
    Predicate* next_predicate = nullptr;
    
};

struct Predicate
{
    AST::Op op;
    std::string left;
    std::string right; 

    Boolean* next_bool = nullptr; 

};



struct Path {
    std::vector<Predicate> predicates;
};



class Plan
{
public:
    using Paths = std::vector<Path>; 
    Paths paths;


    Plan(const AST::SelectQuery& query) {
        if (!query.has_where || !query.condition.root) return;

        paths.push_back(Path{}); // start with one path
        build_paths(query.condition.root.get(), paths);

        //debug_print_plan(*this);
    }


    void build_paths(AST::Expr* expr, Paths& paths) {
        if (!expr) return;

        auto logicalPtr = std::get_if<std::unique_ptr<AST::LogicalExpr>>(expr);

        // Leaf comparison
        if (logicalPtr) {
            auto& node = *logicalPtr->get();

            if (node.op == AST::Op::AND) {
                build_paths(node.left.get(), paths);
                build_paths(node.right.get(), paths);
                return;
            }

            if (node.op == AST::Op::OR) {
                Paths left_paths = paths;
                Paths right_paths = paths;

                build_paths(node.left.get(), left_paths);
                build_paths(node.right.get(), right_paths);

                paths.clear();
                paths.insert(paths.end(), left_paths.begin(), left_paths.end());
                paths.insert(paths.end(), right_paths.begin(), right_paths.end());
                return;
            }


            Predicate p = make_predicate(expr);
            for (auto& path : paths) {
                path.predicates.push_back(p);
            }
            return;
        }


        auto& node = *logicalPtr->get();

        if (node.op == AST::Op::AND) {
            build_paths(node.left.get(), paths);
            build_paths(node.right.get(), paths);
        }
        else if (node.op == AST::Op::OR) {
            Paths left_paths = paths;
            Paths right_paths = paths;

            build_paths(node.left.get(), left_paths);
            build_paths(node.right.get(), right_paths);

            paths.clear();
            paths.insert(paths.end(), left_paths.begin(), left_paths.end());
            paths.insert(paths.end(), right_paths.begin(), right_paths.end());
        }
    }

    Predicate make_predicate(AST::Expr* expr) {
        auto logicalPtr = std::get_if<std::unique_ptr<AST::LogicalExpr>>(expr);
        if (!logicalPtr || !*logicalPtr) {
            throw std::runtime_error("Expected comparison expression");
        }

        auto& node = *logicalPtr->get();

        auto col = std::get<AST::ColumnRef>(*node.left);
        auto lit = std::get<AST::Literal>(*node.right);

        return Predicate{
            node.op,
            col.name,
            lit.value
        };
    }

    inline void debug_print_predicate(const Predicate& p) {
        std::cout
            << p.left << " "
            << AST::op_to_string(p.op) << " "
            << p.right;
    }

    inline void debug_print_path(const Path& path, size_t index) {
        std::cout << "Path " << index << ": ";

        for (size_t i = 0; i < path.predicates.size(); ++i) {
            debug_print_predicate(path.predicates[i]);

            if (i + 1 < path.predicates.size()) {
                std::cout << " AND ";
            }
        }

        std::cout << "\n";
    }

    inline void debug_print_plan(const Plan& plan) {
        std::cout << "Execution Plan\n";
        std::cout << "--------------\n";

        for (size_t i = 0; i < plan.paths.size(); ++i) {
            debug_print_path(plan.paths[i], i);
        }
    }

};
