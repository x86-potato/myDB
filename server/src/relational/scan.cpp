#include "operations.hpp"


//scan expects, first predicate
//if empty string is given, brute force via primary leaves

//for now assume predicate is indexed always






Key make_index_key(const std::string& literal, Type columnType)
{
    Key k;

    switch (columnType)
    {
        case Type::INTEGER:
        {
            int32_t v = std::stoi(literal);
            uint32_t big_endian = htonl(static_cast<uint32_t>(v));

            k.bytes.resize(4);
            memcpy(k.bytes.data(), &big_endian, sizeof(big_endian));

            break;
        }

        case Type::CHAR8:
        case Type::CHAR16:
        case Type::CHAR32:
        case Type::TEXT:
        {
            std::string s = strip_quotes(literal);
            k.bytes.resize(s.size());
            memcpy(k.bytes.data(), s.data(), s.size());
            break;
        }

        default:
            throw std::runtime_error("Unsupported key type");
    }

    return k;
}

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


Scan::Scan(Database& database,Table &table, const Predicate *predicate) 
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


bool Scan::next(Output &output)
{
    if (mode_ == ScanMode::INDEX_SCAN)
    {
        if (cursor_->next())
        {
            std::string literal = strip_quotes(std::get<LiteralOperand>(pred_->right).literal);

            if(!in_range(cursor_->get_key(), *pred_))
            {
                return false;
            }

            off_t record_location = cursor_->get_value();
            output.record = database_.file->get_record(record_location, table_);

            //std::cout << "Record located at: " << record_location << "by Scanner\n";


            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (cursor_->next())
        {
            off_t record_location = cursor_->get_value();
            output.record = database_.file->get_record(record_location, table_);



            return true;
        }

    }

}





