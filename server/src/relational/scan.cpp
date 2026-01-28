#include "operations.hpp"


//scan expects, first predicate
//if empty string is given, brute force via primary leaves

//for now assume predicate is indexed always







bool Scan::in_range(const Key& key, const Predicate& pred)
{
    const ColumnOperand& col_op = std::get<ColumnOperand>(pred.left);
    std::string literal_str = strip_quotes(std::get<LiteralOperand>(pred.right).literal);

    const Type columnType = table_.get_column(col_op.column).type;

    if (columnType == Type::INTEGER) {
        // Interpret bytes as int32_t (big endian)
        int32_t key_val;
        memcpy(&key_val, key.bytes.data(), sizeof(int32_t));
        key_val = ntohl(key_val); // convert from big-endian to host

        int32_t literal_val = std::stoi(literal_str);

        switch (pred.op)
        {
            case AST::Op::EQ:  return key_val == literal_val;
            case AST::Op::LT:  return key_val < literal_val;
            case AST::Op::LTE: return key_val <= literal_val;
            case AST::Op::GT:  return key_val > literal_val;
            case AST::Op::GTE: return key_val >= literal_val;
            default: throw std::runtime_error("Unsupported operation in in_range");
        }
    } else {
        // CHAR/TEXT columns: compare bytes
        Key literal_key = make_index_key(literal_str, columnType);
        int cmp = memcmp(key.bytes.data(), literal_key.bytes.data(), literal_key.bytes.size());

        switch (pred.op)
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


Scan::Scan(Database& database,const Table &table, const Predicate *predicate)
: database_(database), table_(table), pred_(predicate) {
    tables_.push_back(&table_);

    if (pred_ == nullptr)
    {
        mode_ = ScanMode::FULL_SCAN;
    }
    else
    {
        mode_ = ScanMode::INDEX_SCAN;
    }

    //assume for literal predicates the left side is always the column name
    if (mode_ == ScanMode::INDEX_SCAN)
    {
        std::string indexedColumnName = std::get<ColumnOperand>(pred_->left).column;
        std::string key = std::get<LiteralOperand>(pred_->right).literal;

        off_t index_location = table.get_column(indexedColumnName).indexLocation;

        assert(index_location != -1);

        switch (table.get_column(indexedColumnName).type)
        {
            case Type::CHAR32:
                cursor_ = std::make_unique<BPlusTreeCursor<MyBtree32>>(&database.index_tree32);
                break;
            case Type::CHAR16:
                cursor_ = std::make_unique<BPlusTreeCursor<MyBtree16>>(&database.index_tree16);
                break;
            case Type::CHAR8:
                cursor_ = std::make_unique<BPlusTreeCursor<MyBtree8>>(&database.index_tree8);
                break;
            case Type::INTEGER:
                cursor_ = std::make_unique<BPlusTreeCursor<MyBtree4>>(&database.index_tree4);
                break;
            default:
                throw std::runtime_error("Unsupported data type for indexed column");
        }

        cursor_->tree_root = index_location;
        cursor_->db = &database_;

        switch (pred_->op)
        {
            case AST::Op::EQ:
                //set cursor to start at the literal given
                cursor_->key = make_index_key(key, table.get_column(indexedColumnName).type);
                break;
            case AST::Op::LT:
                cursor_->key = std::nullopt;
                break;
            case AST::Op::LTE:
                cursor_->key = std::nullopt;
                break;
            case AST::Op::GT:
                cursor_->key = make_index_key(key, table.get_column(indexedColumnName).type);
                cursor_->skip_equals = true;
                break;
            case AST::Op::GTE:
                cursor_->key = make_index_key(key, table.get_column(indexedColumnName).type);
                break;
            default:
                throw std::runtime_error("Unsupported operation for index scan");
        }
    }
    else if (mode_ == ScanMode::FULL_SCAN)
    {
        //get primary index location
        off_t index_location = table.get_column(0).indexLocation;
        switch (table.get_column(0).type)
        {
            case Type::CHAR32:
                cursor_ = std::make_unique<BPlusTreeCursor<MyBtree32>>(&database.index_tree32);
                break;
            case Type::CHAR16:
                cursor_ = std::make_unique<BPlusTreeCursor<MyBtree16>>(&database.index_tree16);
                break;
            case Type::CHAR8:
                cursor_ = std::make_unique<BPlusTreeCursor<MyBtree8>>(&database.index_tree8);
                break;
            case Type::INTEGER:
                cursor_ = std::make_unique<BPlusTreeCursor<MyBtree4>>(&database.index_tree4);
                break;
            default:
                throw std::runtime_error("Unsupported data type for indexed column");
        }

        cursor_->tree_root = index_location;
        cursor_->db = &database_;
        cursor_->key = std::nullopt;
        //cursor_->set(key);
        //right now b tree cursor is set on first next call

    }


}

void Scan::reset()
{
    if (cursor_)
    {
        cursor_->set(std::nullopt);
    }
}

void Scan::set_key(const std::optional<Key>& key)
{
    if (cursor_)
    {
        cursor_->set(key);
        cursor_->set_externally = true;
    }
}

void Scan::set_key_on_column(const std::optional<Key>& key, const std::string& column_name)
{
    if (!cursor_ || !key.has_value()) return;

    // Find the column
    int col_idx = table_.get_column_index(column_name);
    const Column& col = table_.get_column(col_idx);

    mode_ = ScanMode::INDEX_SCAN;
    set_by_join = true;
    cursor_->set_externally = true;
    index_key_ = key.value();


    if (col.indexLocation == -1) {
        throw std::runtime_error("Column " + column_name + " is not indexed");
    }

    // Switch to the correct index for this column
    switch (col.type) {
        case Type::CHAR32:
            cursor_ = std::make_unique<BPlusTreeCursor<MyBtree32>>(&database_.index_tree32);
            break;
        case Type::CHAR16:
            cursor_ = std::make_unique<BPlusTreeCursor<MyBtree16>>(&database_.index_tree16);
            break;
        case Type::CHAR8:
            cursor_ = std::make_unique<BPlusTreeCursor<MyBtree8>>(&database_.index_tree8);
            break;
        case Type::INTEGER:
            cursor_ = std::make_unique<BPlusTreeCursor<MyBtree4>>(&database_.index_tree4);
            break;
        default:
            throw std::runtime_error("Unsupported data type for indexed column");
    }

    cursor_->tree_root = col.indexLocation;
    cursor_->db = &database_;
    cursor_->set(key);
}

bool Scan::next(Output& output)
{
    output.tuples_.clear();

    if(mode_ == ScanMode::INDEX_SCAN)
    {
        while (true)
        {
            // 1. Still iterating a posting list
            if (posting_block_index_ != -1)
            {
                if (posting_block_index_ < current_posting_block_.size)
                {
                    off_t record_location =
                        current_posting_block_.entries[posting_block_index_++];

                    output.tuples_.push_back({
                        .record = database_.file->get_record(record_location, table_),
                        .location = record_location,
                        .table_ = &table_
                    });

                    return true;
                }
                if(current_posting_block_.next == 0)
                {
                    //end of posting list
                    posting_block_index_ = -1;
                    continue;
                }
                else
                {
                    //load next posting block
                    current_posting_block_ =
                        database_.file->load_posting_block(current_posting_block_.next);
                    posting_block_index_ = 0;
                    continue;
                }
            }

            // 2. Advance index cursor
            if (!cursor_->next())
                return false;

            // 3. Range / join predicate checks
            if (set_by_join)
            {
                if (!cursor_->key_equals(index_key_))
                    return false;
            }
            else if (!in_range(cursor_->get_key(), *pred_))
            {
                return false;
            }

            off_t value = cursor_->get_value();

            // 4. Posting list entry
            if (value % 4096 == 0)
            {
                Posting_Block block =
                    database_.file->load_posting_block(value);

                current_posting_block_ = block;
                posting_block_index_ = 0;

                continue;   // do not emit yet
            }

            // 5. Normal single record
            output.tuples_.push_back({
                .record = database_.file->get_record(value, table_),
                .location = value,
                .table_ = &table_
            });

            return true;
        }
    }
    else
    {
        //full scan mode
        while (true)
        {
            if (!cursor_->next())
                return false;

            off_t value = cursor_->get_value();

            output.tuples_.push_back({
                .record = database_.file->get_record(value, table_),
                .location = value,
                .table_ = &table_
            });

            return true;
        }
    }
}
