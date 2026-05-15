#include "executor.hpp"

void Executor::execute_select(AST::SelectQuery* query, Session& session)
{
    Transaction* txn_ptr;
    {
        std::lock_guard<std::mutex> lock(database.txn_mutex);
        txn_ptr = &database.transactions.at(session.current_transaction_id);
    }
    Transaction& txn = *txn_ptr;

    LogicalPlan plan(*query);
    std::string error_msg;
    if (!validateSelectQuery(*query, database, &error_msg))
    {
        session.send_error(error_msg);
        return;
    }
    if (!validatePlan(plan, database, &error_msg))
    {
        session.send_error(error_msg);
        return;
    }

    if (plan.paths.size() == 0)
    {
        Path temp = Path{};
        PhysicalPlan plan_executor(temp, database, &txn);
        plan_executor.run_select(query, &session);
    }
    else
    {
        PhysicalPlan plan_executor(plan.paths[0], database, &txn);
        plan_executor.run_select(query, &session);
    }
}
