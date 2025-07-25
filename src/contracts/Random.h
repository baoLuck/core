using namespace QPI;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
public:
    struct CreateDeal_input
    {
    };

    struct CreateDeal_output
    {
    };

    struct CreateDeal_locals
    {
    };


    struct GetDeals_input
    {
    };

    struct GetDeals_output
    {
        uint64 dealsAmount;
    };

    struct GetDeals_locals
    {
    };

private:
    Collection<uint8, 16> _deals;
    uint64 _numberOfDeals;

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateDeal)
    {
        state._numberOfDeals++;
        state._deals.add(qpi.invocator(), 5, 0);
        state._numberOfDeals = state._deals.population(qpi.invocator());
        state._numberOfDeals++;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetDeals)
    {
        output.dealsAmount = state._numberOfDeals;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(CreateDeal, 3);
        REGISTER_USER_FUNCTION(GetDeals, 4);
    }

    INITIALIZE()
    {
    }
};
