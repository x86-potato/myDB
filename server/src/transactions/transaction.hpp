#include "../config.h"
#include "../query/plan/planner.hpp"


class Mutation
{
    enum Type {
        INSERT,
        MODIFY,
        DELETE
    } type;
    union
    {
        struct {
            std::string table_name;
            StringVec values;
        } insert;
        struct {
            std::string table_name;

            Plan plan;

        } update;
    };

};

class Transaction {
private:
    std::vector<Mutation> Mutations;
public:
    void add_insert();
    void add_modify();
    void add_delete();


    void begin();
    void commit();
    void rollback();
};
