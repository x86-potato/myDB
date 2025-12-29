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



class Output
{
public:
    Output() = default;

    Record record;
    Table *table = nullptr;
};


class Operator
{
public:
    virtual ~Operator() = default;

    std::vector<Table*> tables_;
    virtual bool next(Output &output) = 0;

};



enum class ScanMode {
    INDEX_SCAN,
    FULL_SCAN,
    FULL_SCAN_WITH_PREDICATE
};

class Scan : public Operator {
public:
    Scan(Database& database, Table& table, const Predicate* pred = nullptr);

    bool next(Output &output) override;

private:
    Table& table_;

    Database& database_;
    const Predicate* pred_;
    ScanMode mode_;


    

    std::unique_ptr<TreeCursor> cursor_;

    bool in_range(const Key& key, const Predicate& pred);
};



class Filter : public Operator {
public:
    Filter(Database& database, Table& table, std::unique_ptr<Operator> child);

    void add_predicate(const Predicate* pred);

    bool next(Output &output) override;

private:
    std::unique_ptr<Operator> child_;
    Table& table_;
    

    Database& database_;
    std::vector<const Predicate*> predicates_;



    bool in_range(Output& to_check);


};