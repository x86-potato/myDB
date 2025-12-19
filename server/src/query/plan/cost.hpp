#include "planner.hpp"



//given our paths, we will assign a cost to each predicate and then sort them
//based on the decudced cost
//you can think of the cost of a predicate as a filtering power
//aka: low cost -> most rows filtered

//predicates fall into 2 main groups

// selection level lower cost
// and join level higher cost


//selection falls into any where clause which has one table and a literal
//as its predicate.

//join is any equation of two tables, users.id == messages.senderid;

//joins will be done last, always

constexpr int COST_FILTER = 1;
constexpr int COST_JOIN = 10;
