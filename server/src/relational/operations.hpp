#pragma once
#include <memory>
#include <optional>
#include <arpa/inet.h>
#include "../storage/record.hpp"

#include "../core/table.hpp"
#include "../core/database.hpp"
#include "../config.h"
#include "../query/plan/planner.hpp"
#include "../core/cursor.hpp"
//parent class of al relational operations

struct OutputTuple
{

    Record record;

    const Table* table_;
};

class Output
{
public:
    Output() = default;
    std::vector<OutputTuple> tuples_;

    const Record &get_single_record(const std::string &table_name)
    {
        for (const auto& tuple : tuples_) {
            if (tuple.table_->name == table_name) {
                return tuple.record;
            }
        }
        throw std::runtime_error("Table not found in output");
    }

    void set_record(const Record &record, const Table* table)
    {
        tuples_.clear();
        tuples_.push_back(OutputTuple{.record = record, .table_ = table});
    }
};


class Operator
{
public:
    virtual ~Operator() = default;

    std::vector<const Table*> tables_;
    virtual bool next(Output &output) = 0;

    virtual void reset() = 0;

    virtual void set_key(const std::optional<Key>& key) = 0;
    virtual void set_key_on_column(const std::optional<Key>& key, const std::string& column_name) {
        set_key(key); // Default implementation
    }
};



enum class ScanMode {
    INDEX_SCAN,
    FULL_SCAN,
    FULL_SCAN_WITH_PREDICATE
};

class Scan : public Operator {
public:
    Scan(Database& database, const Table& table, const Predicate* pred = nullptr);

    bool next(Output &output) override;
    void reset() override;
    void set_key(const std::optional<Key>& key) override;
    void set_key_on_column(const std::optional<Key>& key, const std::string& column_name) override;

private:
    Database& database_;
    const Table& table_;

    const Predicate* pred_;
    Key index_key_;
    bool set_by_join = false;
    ScanMode mode_;


    

    std::unique_ptr<TreeCursor> cursor_;

    bool in_range(const Key& key, const Predicate& pred);
};



class Filter : public Operator {
public:
    Filter(Database& database, const Table& table, std::unique_ptr<Operator> child);

    void add_predicate(const Predicate* pred);

    bool next(Output &output) override;
    void reset() override;
    void set_key(const std::optional<Key>& key) override;
    void set_key_on_column(const std::optional<Key>& key, const std::string& column_name) override;

private:
    Database& database_;
    const Table& table_;
    std::unique_ptr<Operator> child_;
    

    std::vector<const Predicate*> predicates_;



    bool in_range(Output& to_check);


};


class Join : public Operator {
public:
    Join(Database& database, Table& left_table, Table& right_table,
         std::unique_ptr<Operator> left_child,
         std::unique_ptr<Operator> right_child,
         const Predicate* join_predicate);

    bool next(Output &output) override;
    void reset() override;
    void set_key(const std::optional<Key>& key) override;

private:
    Database& database_;
    Table& left_table_;
    Table& right_table_;
    std::unique_ptr<Operator> left_child_;
    std::unique_ptr<Operator> right_child_;
    const Predicate* join_predicate_;

    bool has_left_record_ = false;
    Output left_output_; 
    const Record* current_left_record_ = nullptr;
    std::string left_value_;
};






inline Key make_index_key(const std::string& literal, Type columnType)
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