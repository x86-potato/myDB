#include "planner.hpp"


Plan::Plan(const AST::SelectQuery& query)
{
    //TODO: multi table join later

    if (!query.has_where || !query.condition.root)
    {
        Path initial_path;
        initial_path.tables.push_back(query.tableName);
        paths.push_back(initial_path);
        return;
    }

    paths.push_back(Path{.tables = {query.tableName}, .predicates = {},}); // start with one path
    build_paths(query.condition.root.get(), paths);
}

void Plan::build_paths(AST::Expr* expr, Paths& paths)
{
    if (!expr) return;

    auto logicalPtr = std::get_if<std::unique_ptr<AST::LogicalExpr>>(expr);

    if (logicalPtr)
    {
        auto& node = *logicalPtr->get();

        if (node.op == AST::Op::AND)
        {
            build_paths(node.left.get(), paths);
            build_paths(node.right.get(), paths);
            return;
        }

        if (node.op == AST::Op::OR)
        {
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
        for (auto& path : paths)
        {
            path.predicates.push_back(p);
        }
        return;
    }
}

Predicate Plan::make_predicate(AST::Expr* expr)
{
    auto logicalPtr = std::get_if<std::unique_ptr<AST::LogicalExpr>>(expr);
    if (!logicalPtr || !*logicalPtr)
    {
        throw std::runtime_error("Expected comparison expression");
    }

    auto& node = *logicalPtr->get();
    //assume parser will always put the column on left
    auto col = std::get<AST::ColumnRef>(*node.left);

    //case literal  
    if (auto *lit = std::get_if<AST::Literal>(&(*node.right)))
    {
        //column == literal
        return Predicate{
            .op = node.op,
            .left = ColumnOperand{
                .table = col.table,
                .column = col.column
            },
            .right = LiteralOperand{
                .literal = lit->value
            },
            .predicate_kind = Predicate::PredicateKind::LiteralSelection
        };
    }
    else if (auto *col2 = std::get_if<AST::ColumnRef>(&(*node.right)))
    {
        //same table column == column
        if (col.table == col2->table)
        {
            return Predicate{
                .op = node.op,
                .left = ColumnOperand{
                    .table = col.table,
                    .column = col.column
                },
                .right = ColumnOperand{
                    .table = col2->table,
                    .column = col2->column
                },
                .predicate_kind = Predicate::PredicateKind::ColumnSelection
            };
        }
        //different table column == column
        return Predicate{
            .op = node.op,
            .left = ColumnOperand{
                .table = col.table,
                .column = col.column
            },
            .right = ColumnOperand{
                .table = col2->table,
                .column = col2->column
            },
            .predicate_kind = Predicate::PredicateKind::Join
        };
    }

}
void Plan::debug_print_predicate(const Predicate& p)
{
    struct OperandPrinter {
        void operator()(const ColumnOperand& op) const {
            std::cout << op.table << "." << op.column;
        }
        void operator()(const LiteralOperand& op) const {
            std::cout  << op.literal;
        }
        void operator()(std::monostate) const {
            std::cout << "[EMPTY]";
        }
    };

    std::visit(OperandPrinter{}, p.left);
    std::cout << " " << AST::op_to_string(p.op) << " ";
    std::visit(OperandPrinter{}, p.right);
}

void Plan::debug_print_path(const Path& path, size_t index)
{
    std::cout << "Path " << index << ": ";

    for (size_t i = 0; i < path.predicates.size(); ++i)
    {
        debug_print_predicate(path.predicates[i]);
        if (i + 1 < path.predicates.size())
            std::cout << " AND ";
    }

    std::cout << "\n";
}

void Plan::debug_print_plan(const Plan& plan)
{
    std::cout << "Execution Plan\n";
    std::cout << "--------------\n";

    for (size_t i = 0; i < plan.paths.size(); ++i)
        debug_print_path(plan.paths[i], i);
}


void Plan::execute()
{

}