#include "operations.hpp"


Join::Join(Database& database, Table& left_table, Table& right_table, std::unique_ptr<Operator> left, std::unique_ptr<Operator> right, const Predicate* pred)
    : database_(database),left_table_(left_table), right_table_(right_table),  left_child_(std::move(left)), right_child_(std::move(right)), join_predicate_(pred)
{
    tables_.push_back(&left_table_);
    tables_.push_back(&right_table_);
}

void Join::reset()
{
    left_child_->reset();
    right_child_->reset();
    has_left_record_ = false;
}

void Join::set_key(const std::optional<Key>& key)
{
    left_child_->set_key(key);
    right_child_->set_key(key);
}

bool Join::next(Output &output)
{
    while (true) {
        // If we need a new left row
        if (!has_left_record_) {
            left_output_.tuples_.clear();  // Clear previous data
            if (!left_child_->next(left_output_)) {
                // No more left rows → join is done
                return false;
            }

            current_left_record_ = &left_output_.get_single_record(left_table_.name);
            left_value_ = const_cast<Record&>(*current_left_record_).get_token(
                left_table_.get_column_index(
                    std::get<ColumnOperand>(join_predicate_->left).column),
                left_table_
            );

            // Reset right child for the new left row
            Key k;
            k.bytes.resize(left_value_.size());

            std::string right_col = std::get<ColumnOperand>(join_predicate_->right).column;
            Type right_col_type = right_table_.get_column(right_col).type;
            // copy bytes in reverse order
            for (size_t i = 0; i < left_value_.size(); ++i) {   
                k.bytes[i] = static_cast<std::byte>(left_value_[left_value_.size() - 1 - i]);
            }

            right_child_->set_key_on_column(k, right_col);
            has_left_record_ = true;
        }

        // Try to fetch next right row
        Output right_output;
        if (right_child_->next(right_output)) {
            const Record &right_record = right_output.get_single_record(right_table_.name);
            std::string right_value = const_cast<Record&>(right_record).get_token(
                right_table_.get_column_index(
                    std::get<ColumnOperand>(join_predicate_->right).column),
                right_table_
            );

            if (left_value_ == right_value) {
                // Produce one joined tuple
                output.tuples_.clear();
                output.tuples_.push_back({*current_left_record_, &left_table_});
                output.tuples_.push_back({right_record, &right_table_});
                return true;
            }


            // No match: continue iterating right rows for the same left row
        } else {
            // Right exhausted → fetch next left row in next iteration
            has_left_record_ = false;
        }
    }
}

