#include "../config.h"
#include "../query/plan/planner.hpp"


class Mutation
{

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
