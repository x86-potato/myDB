#include "physical.hpp"
#include "../../server/session.hpp"
#include <iomanip>
#include <iterator>
#include <variant>


//takes a path and turns each prediacte into a relational object

bool PhysicalPlan::check_if_indexed(const Predicate &predicate)
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
void PhysicalPlan::build_predicate_buckets()
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

void PhysicalPlan::trim_scan_candidates()
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

bool PhysicalPlan::check_if_filter_needed(const std::string &table_name)
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

void PhysicalPlan::populate_filter(
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


Predicate *PhysicalPlan::pick_scan_predicate(const std::string &table_name)
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


void PhysicalPlan::build_forest()
{
    for (const auto& table_name : path_.tables)
    {
        Predicate* scan_predicate = pick_scan_predicate(table_name);
        const Table &table = database_.get_table(table_name);
        auto scan_op = std::make_unique<Scan>(database_, table, scan_predicate, txn);

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

void PhysicalPlan::compress_forest_into_root()
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

        //notify inner scan
        forest[get_index_right]->set_to_inner();

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

PhysicalPlan::PhysicalPlan(Path &path, Database& database, Transaction* txn) 
:  path_(path), database_(database), txn(txn)
{
    build_predicate_buckets();
    trim_scan_candidates();
    build_forest();
    assert (forest.size() != 0);
    compress_forest_into_root();
    assert (forest.size() == 1);
    root = std::move(forest[0]);
    forest.clear();
}


void PhysicalPlan::run_delete()
{
    Output output;
    int deleted_count = 0;

    root->delete_on_match = true;

    const Table &table = database_.get_table(path_.tables[0]);

    while (root->next(output))
    {
        if(database_.file->delete_record(output.tuples_[0].record,output.tuples_[0].location, table, 0) != 0) {
            break;
        }
        deleted_count++;

        root->reset();
        //std::cout << "Deleted " << deleted_count << " records." << "\n";
    }
    std::cout << "Deleted " << deleted_count << " records." << "\n";
}


void PhysicalPlan::run_update(std::vector<AST::UpdateArg> &update_args)
{
    //for now assume only one update arg is given
    Table table = database_.get_table(update_args[0].tableName);
    int index = table.get_column_index(update_args[0].column);


    Output output;
    int modified_count = 0;
    while (root->next(output))
    {

        if(auto arg = std::get_if<AST::Literal>(update_args[0].value.get()))
        {
            database_.file->update_record(
                output.tuples_[0].record,
                output.tuples_[0].location,
                index,arg->value, 
                &table, 0);
        }
        modified_count++;
    }

    std::cout << "Updated " << modified_count << " records." << "\n";
}


// -----------------------------------------------------------------------
// Helpers shared by all select paths
// -----------------------------------------------------------------------

static int col_display_width(const Column& col)
{
    switch (col.type) {
        case Type::INTEGER: return 10;
        case Type::CHAR32:  return 32;
        case Type::CHAR16:  return 16;
        case Type::CHAR8:   return 8;
        case Type::BOOL:    return 5;
        case Type::TEXT:    return 32;
        default:            return 10;
    }
}

std::vector<PhysicalPlan::ColMapping>
PhysicalPlan::build_col_map(const AST::SelectQuery* query) const
{
    std::vector<ColMapping> col_map;

    if (query->select_all)
    {
        for (const auto& table_name : path_.tables)
        {
            const Table& tbl = database_.get_table(table_name);
            for (size_t ci = 0; ci < tbl.columns.size(); ci++)
            {
                std::string header = table_name + "." + tbl.columns[ci].name;
                int width = std::max(col_display_width(tbl.columns[ci]),
                                     static_cast<int>(header.length()));
                col_map.push_back({table_name, ci, header, width});
            }
        }
    }
    else
    {
        for (const auto& sc : query->selected_columns)
        {
            std::string tname = sc.table.empty() ? path_.tables[0] : sc.table;
            const Table& tbl = database_.get_table(tname);
            for (size_t ci = 0; ci < tbl.columns.size(); ci++)
            {
                if (tbl.columns[ci].name == sc.column)
                {
                    std::string header = tname + "." + tbl.columns[ci].name;
                    int width = std::max(col_display_width(tbl.columns[ci]),
                                         static_cast<int>(header.length()));
                    col_map.push_back({tname, ci, header, width});
                    break;
                }
            }
        }
    }

    return col_map;
}

// -----------------------------------------------------------------------
// Aggregate path: COUNT / SUM / MAX / MIN
// -----------------------------------------------------------------------

void PhysicalPlan::run_aggregate(const AST::SelectQuery* query, Session* session)
{
    bool send_packets = (session != nullptr && !session->is_admin());

    struct AggState {
        long long value   = 0;
        bool      has_rows = false;
        int       col_idx  = -1;
        std::string tname;
    };

    const size_t n = query->aggregates.size();
    std::vector<AggState> states(n);

    for (size_t i = 0; i < n; i++)
    {
        const auto& agg = query->aggregates[i];
        states[i].tname = agg.column.table.empty() ? path_.tables[0] : agg.column.table;
        if (agg.type != AST::AggregateType::COUNT)
            states[i].col_idx = database_.get_table(states[i].tname)
                                         .get_column_index(agg.column.column);
    }

    Output output;
    long long row_count = 0;

    while (root->next(output))
    {
        row_count++;
        for (size_t i = 0; i < n; i++)
        {
            const auto& agg = query->aggregates[i];
            if (agg.type == AST::AggregateType::COUNT) continue;
            AggState& st = states[i];
            for (const auto& tuple : output.tuples_)
            {
                if (tuple.table_->name != st.tname) continue;
                long long val = std::stoll(tuple.record.get_token(st.col_idx, *tuple.table_));
                switch (agg.type)
                {
                    case AST::AggregateType::SUM: st.value += val; break;
                    case AST::AggregateType::MAX: if (!st.has_rows || val > st.value) st.value = val; break;
                    case AST::AggregateType::MIN: if (!st.has_rows || val < st.value) st.value = val; break;
                    default: break;
                }
                st.has_rows = true;
                break;
            }
        }
        if (query->limit > 0 && row_count >= query->limit) break;
    }

    for (size_t i = 0; i < n; i++)
    {
        const auto& agg  = query->aggregates[i];
        AggState&   st   = states[i];
        bool        last = (i == n - 1);

        long long   result;
        std::string label;
        AggregateKind kind;

        switch (agg.type)
        {
            case AST::AggregateType::COUNT:
                result = row_count; label = "count(*)"; kind = AggregateKind::COUNT; break;
            case AST::AggregateType::SUM:
                result = st.value; label = "sum(" + agg.column.column + ")"; kind = AggregateKind::SUM; break;
            case AST::AggregateType::MAX:
                result = st.value; label = "max(" + agg.column.column + ")"; kind = AggregateKind::MAX; break;
            case AST::AggregateType::MIN:
                result = st.value; label = "min(" + agg.column.column + ")"; kind = AggregateKind::MIN; break;
            default:
                result = 0; label = "?"; kind = AggregateKind::COUNT; break;
        }

        std::string result_str = std::to_string(result);

        if (send_packets)
            session->send_aggregate(kind, label, result_str, last);
        else
            std::cout << label << "\n" << std::string(label.size(), '-') << "\n" << result_str << "\n";
    }
}

// -----------------------------------------------------------------------
// Row path: TCP packet stream to client
// -----------------------------------------------------------------------

void PhysicalPlan::run_select_packets(const AST::SelectQuery* query,
                                      const std::vector<ColMapping>& col_map,
                                      Session* session)
{
    auto to_col_type = [](Type t) -> ColumnType {
        switch (t) {
            case Type::INTEGER: return ColumnType::INT;
            case Type::BOOL:    return ColumnType::BOOL;
            default:            return ColumnType::TEXT;
        }
    };

    std::vector<TableDescriptor> descriptors;
    for (const auto& table_name : path_.tables)
    {
        TableDescriptor td;
        td.name = table_name;
        const Table& tbl = database_.get_table(table_name);
        for (const auto& cm : col_map)
        {
            if (cm.table_name == table_name)
                td.columns.push_back({tbl.columns[cm.col_idx].name,
                                      to_col_type(tbl.columns[cm.col_idx].type)});
        }
        if (!td.columns.empty())
            descriptors.push_back(std::move(td));
    }

    Output current_output;
    if (!root->next(current_output))
    {
        session->send_metadata(descriptors, false);
        return;
    }
    session->send_metadata(descriptors, true);

    int rows_sent = 0;
    while (true)
    {
        std::vector<std::string> values;
        for (const auto& cm : col_map)
        {
            for (const auto& tuple : current_output.tuples_)
            {
                if (tuple.table_->name == cm.table_name)
                {
                    values.push_back(tuple.record.get_token(cm.col_idx, *tuple.table_));
                    break;
                }
            }
        }
        rows_sent++;
        bool limit_reached = (query->limit > 0 && rows_sent >= query->limit);
        Output next_output;
        bool has_next = !limit_reached && root->next(next_output);
        session->send_row_values(values, !has_next);
        if (!has_next) break;
        current_output = std::move(next_output);
    }
}

// -----------------------------------------------------------------------
// Row path: pretty-printed stdout (admin session)
// -----------------------------------------------------------------------

void PhysicalPlan::run_select_stdout(const AST::SelectQuery* query,
                                     const std::vector<ColMapping>& col_map)
{
    auto print_separator = [&col_map]() {
        std::cout << "+";
        for (const auto& cm : col_map)
            std::cout << std::string(cm.display_width + 2, '-') << "+";
        std::cout << "\n";
    };

    print_separator();
    std::cout << "| ";
    for (const auto& cm : col_map)
        std::cout << std::left << std::setw(cm.display_width) << cm.header << " | ";
    std::cout << "\n";
    print_separator();

    Output output;
    int rows = 0;
    while (root->next(output))
    {
        rows++;
        std::cout << "| ";
        for (const auto& cm : col_map)
        {
            for (const auto& tuple : output.tuples_)
            {
                if (tuple.table_->name == cm.table_name)
                {
                    std::string val = tuple.record.get_token(cm.col_idx, *tuple.table_);
                    std::cout << std::left << std::setw(cm.display_width) << val << " | ";
                    break;
                }
            }
        }
        std::cout << "\n";
        if (query->limit > 0 && rows >= query->limit) break;
    }

    print_separator();
    std::cout << "Returned " << rows << " records.\n";
}

// -----------------------------------------------------------------------
// run_select: dispatch to the right execution path
// -----------------------------------------------------------------------

void PhysicalPlan::run_select(const AST::SelectQuery* query, Session* session)
{
    if (!query->aggregates.empty())
    {
        run_aggregate(query, session);
        return;
    }

    auto col_map = build_col_map(query);

    if (session != nullptr && !session->is_admin())
        run_select_packets(query, col_map, session);
    else
        run_select_stdout(query, col_map);
}

//helpers
