#include "operations.hpp"


//scan expects, first predicate
//if empty string is given, brute force via primary leaves

//for now assume predicate is indexed always









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
        cursor_->key = strip_quotes(key);
        //cursor_->set(key);
        //right now b tree cursor is set on first next call
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

            if (!cursor_->key_equals(literal))
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





