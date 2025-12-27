#include "builder.hpp"

//takes a path and turns each prediacte into a relational object

//picks a predicate to use for scan 
Predicate &pick_scan_predicate(Path &path)
{
    //for now just return the first one
    return path.predicates[0];
}

Pipeline::Pipeline(Path &path, Database& database)
{
    Predicate& scan_predicate = pick_scan_predicate(path);

    auto temp = std::make_unique<Scan>(database, database.get_table
        (std::get<ColumnOperand>(scan_predicate.left).table), &scan_predicate);

    root = std::move(temp);

    Output output;

    while (root->next(output))
    {
        std::cout << output.record.str << std::endl;
    }
}   


void Pipeline::Execute()
{
    Output output;

    while (root->next(output))
    {
        std::cout << "Pipline output record: " << output.record.str << std::endl;
    }

}





//helpers
