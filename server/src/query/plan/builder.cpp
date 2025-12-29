#include "builder.hpp"


//takes a path and turns each prediacte into a relational object

bool Pipeline::check_if_indexed(const Predicate &predicate)
{
    // Check if the path has any indexed predicates
    std::string table_name = std::get<ColumnOperand>(predicate.left).table;
    std::string column_name = std::get<ColumnOperand>(predicate.left).column;
    const Table &table = database_.tableMap.at(table_name);

    if (table.indexed_on_column(column_name))
    {
        return true;
    }
    return false;
}
//we fill the buckets gien our predicates
void Pipeline::build_buckets()
{
    //ALL scan predicates may also be filter predicates,
    //therefore we put them into scan first, then filter
    //later we move them to filter

    for (const auto& pred : path_.predicates)
    {
        //scans need to be: literal selection, and indexed 
        if (pred.predicate_kind == Predicate::PredicateKind::LiteralSelection)
        {
            if(check_if_indexed(pred))
            {
                scan_candidates_.push_back(&pred);
            }
            else
            {
                filter_candidates_.push_back(&pred);
            }
        }
        else if (pred.predicate_kind == Predicate::PredicateKind::ColumnSelection)
        {
            filter_candidates_.push_back(&pred);
        }
        else if (pred.predicate_kind == Predicate::PredicateKind::Join)
        {
            join_candidates_.push_back(&pred);
        }

    }
}

void Pipeline::print_buckets()
{
    std::cout << "Scan candidates: " << scan_candidates_.size() << std::endl;
    for (const auto& pred : scan_candidates_)
    {
        std::cout << "  Predicate on table "
                  << std::get<ColumnOperand>(pred->left).table
                  << " column "
                  << std::get<ColumnOperand>(pred->left).column
                  << std::endl;
    }

    std::cout << "Filter candidates: " << filter_candidates_.size() << std::endl;
    for (const auto& pred : filter_candidates_)
    {
        std::cout << "  Predicate on table "
                  << std::get<ColumnOperand>(pred->left).table
                  << " column "
                  << std::get<ColumnOperand>(pred->left).column
                  << std::endl;
    }

    std::cout << "Join candidates: " << join_candidates_.size() << std::endl;
    for (const auto& pred : join_candidates_)
    {
        std::cout << "  Predicate on table "
                  << std::get<ColumnOperand>(pred->left).table
                  << " column "
                  << std::get<ColumnOperand>(pred->left).column
                  << std::endl;
    }
}

void Pipeline::sort_buckets()
{
    //keep only first scan candidate for now
    //later pick a better one based on stats
    if (scan_candidates_.size() > 1)
    {
        for (size_t i = 1; i < scan_candidates_.size(); i++)
        {
            filter_candidates_.push_back(scan_candidates_[i]);
        }
        scan_candidates_.erase(scan_candidates_.begin() + 1, scan_candidates_.end());

    }
}


Predicate &pick_scan_predicate(Path &path)
{
    //for now just return the first one
    return path.predicates[0];
}

Pipeline::Pipeline(Path &path, Database& database) : database_(database), path_(path)
{
    build_buckets();
    sort_buckets();

    //print_buckets();

    const Predicate* scan_pred = nullptr;
    //build scan
    if (!scan_candidates_.empty())
    {
        assert (scan_candidates_.size() == 1);
        scan_pred = scan_candidates_[0];
        auto temp = std::make_unique<Scan>(database, database.get_table
            (std::get<ColumnOperand>(scan_pred->left).table), scan_pred);
        root = std::move(temp);
    }
    else
    {
        //full table scan
        //pick a table from the path
        assert(path_.tables.size() == 1); //for now only single table paths
        auto temp = std::make_unique<Scan>(database, database.get_table(path_.tables[0]), nullptr);
        root =  std::move(temp);
    }

    


    //build filter
    if(!filter_candidates_.empty())
    {
        auto temp = std::make_unique<Filter>(database, database.get_table(
            std::get<ColumnOperand>(filter_candidates_[0]->left).table), std::move(root));

        for (const auto& pred : filter_candidates_)
        {
            temp->add_predicate(pred);     
        }

        root = std::move (temp);
    }

    



}   




void Pipeline::Execute()
{
    Output output;
    output.table = root.get()->tables_[0];

    while (root->next(output))
    {

        std::cout << "| ";
        for (const auto& token : output.record.to_tokens(*output.table)) {
            std::cout << token << " | ";
        }
        std::cout << std::endl;
    }

}





//helpers
