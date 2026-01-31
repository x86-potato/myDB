#include "operations.hpp"

Filter::Filter(Database& database, const Table& table, std::unique_ptr<Operator> child)
    :  database_(database), table_(table), child_(std::move(child))
{
    tables_.push_back(&table_);
}


void Filter::add_predicate(const Predicate* pred)
{
    predicates_.push_back(pred);
}

bool Filter::in_range(Output& to_check)
{
    for (const auto& pred : predicates_)
    {
        std::string column_name = std::get<ColumnOperand>(pred->left).column;

        const Type columnType = table_.get_column(column_name).type;

        int column_index = table_.get_column_index(column_name);

        //here assume only single output tuple in output
        assert (to_check.tuples_.size() == 1);

        std::string token = to_check.tuples_[0].record.get_token(column_index, table_);

        if (columnType == Type::INTEGER) {
            int32_t val;
            val = stoi(token);  // convert from big-endian if needed

            int32_t literal_val = std::stoi(strip_quotes(std::get<LiteralOperand>(pred->right).literal));

            switch (pred->op)
            {
                case AST::Op::EQ:
                    if (!(val == literal_val)) return false;
                    break;
                case AST::Op::LT:
                    if (!(val < literal_val)) return false;
                    break;
                case AST::Op::LTE:
                    if (!(val <= literal_val)) return false;
                    break;
                case AST::Op::GT:
                    if (!(val > literal_val)) return false;
                    break;
                case AST::Op::GTE:
                    if (!(val >= literal_val)) return false;
                    break;
                default:
                    throw std::runtime_error("Unsupported operation in in_range");
            }
        }
        else {
            // CHAR/TEXT columns: compare strings
            std::string literal_val = strip_quotes(std::get<LiteralOperand>(pred->right).literal);
            Key literal_key = make_index_key(literal_val, columnType);
            Key compare_key = make_index_key(token, columnType);

            int cmp = token.compare(literal_val);

            switch (pred->op)
            {
                case AST::Op::EQ:  return cmp == 0;
                case AST::Op::LT:  return cmp < 0;
                case AST::Op::LTE: return cmp <= 0;
                case AST::Op::GT:  return cmp > 0;
                case AST::Op::GTE: return cmp >= 0;
                default: throw std::runtime_error("Unsupported operation in in_range");
            }
        }
    }

    return true;

}

void Filter::reset()
{
    child_->reset_and_skip();
}
void Filter::set_key(const Key& key)
{
    child_->set_key(key);
}
void Filter::set_key_on_column(const Key& key, const std::string& column_name)
{
    child_->set_key_on_column(key, column_name);
}
void Filter::reset_and_skip()
{
}


bool Filter::next(Output &output)
{
    int calls = 0;
    while (child_->next(output))
    {
        if(in_range(output))
        {
            calls_since_output = calls; 
            //std::cout << "Filter passing after " << calls << " calls\n";
            return true;
        }
        calls++;
    }

    return false;
}
