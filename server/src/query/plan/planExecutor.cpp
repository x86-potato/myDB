#include "planExecutor.hpp"

PlanExecutor::PlanExecutor(Database& db, const Plan& plan)
    : database(db), plan(plan)
{
    // constructor body (init if needed)
}

std::vector<off_t> PlanExecutor::execute()
{
    std::vector<off_t> results;

    //by now assume all tables are valid and exist
    //for now onlyt support single table selects and
    //only indexed literal selections
    
    //we use a cursor per single table given, the cursor is assigned to most selective index
    //right now we do not optimize, we pick cursor based on the first literal selection predicate we find


    BPlusTreeCursor<MyBtree32> cursor32;
    BPlusTreeCursor<MyBtree16> cursor16;
    BPlusTreeCursor<MyBtree8> cursor8;
    BPlusTreeCursor<MyBtree4> cursor4;

    for (auto& path : plan.paths)
    {
        for (auto& predicate : path.predicates)
        {
            switch (predicate.predicate_kind)
            {
                case Predicate::PredicateKind::LiteralSelection:
                {
                    auto col = std::get<ColumnOperand>(predicate.left);
                    auto lit = std::get<LiteralOperand>(predicate.right);

                    if(database.get_table(col.table).indexed_on_column(col.column))
                    {
                        //use index to execute selection
                        switch (database.get_table(col.table).get_column(col.column).type)
                        {
                            case Type::CHAR32:
                                database.index_tree32.root_node = database.file->load_node<typename MyBtree32::NodeType>(
                                    database.get_table(col.table).get_column(col.column).indexLocation);
                                cursor32.set(&database.index_tree32, lit.literal);
                                break;
                            case Type::CHAR16:
                                database.index_tree16.root_node = database.file->load_node<typename MyBtree16::NodeType>(
                                    database.get_table(col.table).get_column(col.column).indexLocation);
                                cursor16.set(&database.index_tree16, lit.literal);
                                break;
                            case Type::CHAR8:
                                database.index_tree8.root_node = database.file->load_node<typename MyBtree8::NodeType>(
                                    database.get_table(col.table).get_column(col.column).indexLocation);
                                cursor8.set(&database.index_tree8, lit.literal);
                                break;
                            case Type::INTEGER:
                                database.index_tree4.root_node = database.file->load_node<typename MyBtree4::NodeType>(
                                    database.get_table(col.table).get_column(col.column).indexLocation);
                                cursor4.set(&database.index_tree4, lit.literal);
                                break;
                            default:
                                std::cout << "Error: Unsupported index type for column " << col.column << " in table " << col.table << "\n";
                                break;
                        
                        }


                        
                    }
                    else
                    {
                        //full table scan needed
                        std::cout << "Full table scan needed for column " << col.column << " in table " << col.table << "\n";
                    }
                    break;
                }
                case Predicate::PredicateKind::ColumnSelection:
                    //execute column selection
                    break;
                case Predicate::PredicateKind::Join:
                    //execute join
                    break;
            }
        }
    }

    return results;
    
}


PathExecutor::PathExecutor(Database& db, const Path& path)
    : database(db), path(path)
{
    // constructor body (init cursors / state)
}

bool PathExecutor::next(off_t& out)
{
    // TODO: iterate over predicates in path
    //       use LiteralSelection / ColumnSelection / Join
    return false; // placeholder
}
