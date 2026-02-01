#include "builder.hpp"
#include <iomanip>
#include <iterator>
#include <variant>


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

bool Pipeline::check_if_filter_needed(const std::string &table_name)
{
    for (const auto &pred : filter_candidates_)
    {
        if (std::get<ColumnOperand>(pred->left).table == table_name)
        {
            return true;
        }
    }
    return false;
}

void Pipeline::populate_filter(
    const std::string &table_name,
    Filter &Filter)
{
    for (const auto &pred : filter_candidates_)
    {
        if (std::get<ColumnOperand>(pred->left).table == table_name)
        {
            Filter.add_predicate(pred);
        }
    }
}


Predicate *Pipeline::pick_scan_predicate(const std::string &table_name)
{
    for (const auto &pred : scan_candidates_)
    {
        //by now we can promise its a literal predicate
        assert (pred->predicate_kind == Predicate::PredicateKind::LiteralSelection);
        if (std::get<ColumnOperand>(pred->left).table == table_name)
        {
            return const_cast<Predicate*>(pred);
        }
    }
    return nullptr;


}


void Pipeline::build_forest()
{
    for (const auto& table_name : path_.tables)
    {
        Predicate* scan_predicate = pick_scan_predicate(table_name);
        const Table &table = database_.get_table(table_name);
        auto scan_op = std::make_unique<Scan>(database_, table, scan_predicate);

        //also build filter if needed
        if (check_if_filter_needed(table_name))
        {
            auto filter_op = std::make_unique<Filter>
            (database_, table, std::move(scan_op));

            populate_filter(table_name, *filter_op);

            forest.push_back(std::move(filter_op));

        }
        else
            forest.push_back(std::move(scan_op));
    }
}

int get_index_of_table_in_forest(
    const std::vector<std::unique_ptr<Operator>> &forest,
    const std::string &table_name)
{
    for (size_t i = 0; i < forest.size(); i++)
    {
        for (size_t j = 0; j < forest[i]->tables_.size(); j++)
        {
            if (forest[i]->tables_[j]->name == table_name)
            {
                return i;
            }
        }
    }
    return -1;
}

void Pipeline::compress_forest()
{
    //compress tree into subtrees
    //assume joins are given
    assert (join_candidates_.size() == forest.size() - 1);
    while(forest.size() > 1)
    {
        //use first join cnadidate,
        //then pop front of vector
        const Predicate* join_pred = join_candidates_.front();
        int get_index_left = get_index_of_table_in_forest(
            forest,
            std::get<ColumnOperand>(join_pred->left).table);
        int get_index_right = get_index_of_table_in_forest(
            forest,
            std::get<ColumnOperand>(join_pred->right).table);

        assert (get_index_left != -1 && get_index_right != -1);

        auto sub_tree = std::make_unique<Join>(
            database_,
            const_cast<Table&>(*(forest[get_index_left]->tables_[0])),
            const_cast<Table&>(*(forest[get_index_right]->tables_[0])),
            std::move(forest[get_index_left]),
            std::move(forest[get_index_right]),
            join_pred);
        //erase the idnex left, right from forest, and pop first join candidate
        int i = get_index_left;
        int j = get_index_right;

        if (i > j) std::swap(i, j);   // erase higher index first

        forest.erase(forest.begin() + j);
        forest.erase(forest.begin() + i);

        forest.push_back(std::move(sub_tree));
    }
}

Pipeline::Pipeline(Path &path, Database& database) :  path_(path), database_(database)
{
    build_buckets();
    sort_buckets();

    print_buckets();


    build_forest();
    assert (forest.size() != 0);

    compress_forest();

    assert (forest.size() == 1);
    root = std::move(forest[0]);
    forest.clear(); // Clear the forest to avoid any potential issues with moved-from unique_ptrs

}


void Pipeline::ExecuteDelete()
{
    Output output;
    int deleted_count = 0;

    root->delete_on_match = true;

    const Table &table = database_.get_table(path_.tables[0]);

    while (root->next(output))
    {
        if(database_.file->delete_record(output.tuples_[0].record,output.tuples_[0].location, table) != 0) {
            break;
        }
        deleted_count++;

        root->reset();
        //std::cout << "Deleted " << deleted_count << " records." << std::endl;
    }
    std::cout << "Deleted " << deleted_count << " records." << std::endl;
}


void Pipeline::ExecuteUpdate(std::vector<AST::UpdateArg> &update_args)
{
    //for now assume only one update arg is given
    Table table = database_.get_table(update_args[0].tableName);
    int index = table.get_column_index(update_args[0].column);


    Output output;
    int modified_count = 0;
    while (root->next(output))
    {
        switch(table.columns[index].type)
        {
            case Type::INTEGER:
                if(auto arg = std::get_if<AST::Literal>(update_args[0].value.get()))
                {
                    std::string value;

                    int32_t number = std::stoi(arg->value);
                    //uint32_t big_endian = htonl(static_cast<uint32_t>(number));

                    value.append(reinterpret_cast<const char*>(&number), sizeof(number));


                    database_.file->update_record(
                        output.tuples_[0].record,
                        output.tuples_[0].location,
                        index, value);

                }
                else {
                    std::cerr << "only = literal supported right now";
                }

                break;
            default:
                std::cout << "Unsupported type" << std::endl;
                //throw std::runtime_error("Unsupported type");
        }
        modified_count++;
    }

    std::cout << "Updated " << modified_count << " records." << std::endl;
}


void Pipeline::Execute()
{
    // Calculate column widths based on type
    std::vector<int> column_widths;
    std::vector<std::string> column_headers;

    for (const auto& table_name : path_.tables)
    {
        const Table& tbl = database_.get_table(table_name);
        for (const auto& col : tbl.columns) {
            std::string header = table_name + "." + col.name;
            column_headers.push_back(header);

            int width;
            switch (col.type) {
                case Type::INTEGER:
                    width = 10;  // Fits ~2 billion
                    break;
                case Type::CHAR32:
                    width = 32;
                    break;
                case Type::CHAR16:
                    width = 16;
                    break;
                case Type::CHAR8:
                    width = 8;
                    break;
                case Type::BOOL:
                    width = 5;  // "true" or "false"
                    break;
                case Type::TEXT:
                    width = 32;  // Default for TEXT
                    break;
                default:
                    width = 10;
            }
            // Ensure width is at least as wide as the header
            width = std::max(width, static_cast<int>(header.length()));
            column_widths.push_back(width);
        }
    }

    // Helper lambda to print separator line
    auto print_separator = [&column_widths]() {
        std::cout << "+";
        for (size_t i = 0; i < column_widths.size(); ++i) {
            std::cout << std::string(column_widths[i] + 2, '-') << "+";
        }
        std::cout << "\n";
    };

    // Print top border
    print_separator();

    // Print header
    std::cout << "| ";
    for (size_t i = 0; i < column_headers.size(); ++i) {
        std::cout << std::left << std::setw(column_widths[i]) << column_headers[i] << " | ";
    }
    std::cout << "\n";

    // Print separator after header
    print_separator();

    Output output;
    int i = 0;
    while (root->next(output))
    {
        i++;
        std::cout << "| ";
        size_t col_idx = 0;
        for (const auto& tuple : output.tuples_)
        {
            for (const auto& token : tuple.record.to_tokens(*tuple.table_))
            {
                std::cout << std::left << std::setw(column_widths[col_idx]) << token << " | ";
                col_idx++;
            }
        }
        std::cout << "\n";
    }

    // Print bottom border
    print_separator();

    std::cout << "Returned " << i << " records." << std::endl;
}






//helpers
