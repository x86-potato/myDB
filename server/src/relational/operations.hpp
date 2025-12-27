#pragma once
#include <memory>
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
};

class Operator
{
public:
    virtual ~Operator() = default;

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
    void init();

    Database& database_;
    Table& table_;
    const Predicate* pred_;
    ScanMode mode_;


    

    std::unique_ptr<TreeCursor> cursor_;

};