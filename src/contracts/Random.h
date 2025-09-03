using namespace QPI;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
public:
    struct GetCounter_input
    {
    };

    struct GetCounter_output
    {
        uint64 counter;
        id self;
        uint64 name;
    };

    struct IssueBond_input
    {
        uint64 t;
    };

    struct IssueBond_output
    {
    };
    
    
private:
    uint64 _mbond;
    uint64 _counter;
    id _self;
    uint64 _currentName;

    PUBLIC_FUNCTION(GetCounter)
    {
        output.counter = state._counter;
        output.self = state._self;
        output.name = state._currentName;
    }

    PUBLIC_PROCEDURE(IssueBond)
    {
        state._counter = qpi.issueAsset(state._currentName, SELF, (signed char) 0, 1000000LL, 0ULL);
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(IssueBond, 1);
        REGISTER_USER_FUNCTION(GetCounter, 1);
    }

    INITIALIZE()
    {
        state._mbond = 293371593293ULL;
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    struct BEGIN_EPOCH_locals
    {
        sint8 p1;
        sint8 p2;
        sint8 p3;
        uint64 currentName;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        state._mbond = 293371593293ULL;

        locals.currentName = state._mbond;

        locals.p1 = 48 + QPI::mod(QPI::div((uint64)qpi.epoch(), 100ULL), 10ULL);
        locals.p2 = 48 + QPI::mod(QPI::div((uint64)qpi.epoch(), 10ULL), 10ULL);
        locals.p3 = 48 + QPI::mod((uint64)qpi.epoch(), 10ULL);

        locals.currentName |= (uint64)locals.p1 << (5 * 8);
        locals.currentName |= (uint64)locals.p2 << (6 * 8);
        locals.currentName |= (uint64)locals.p3 << (7 * 8);

        state._currentName = locals.currentName;

        //state._counter = locals.currentName;
        state._self = SELF;

        //state._counter = qpi.issueAsset(locals.currentName, SELF, 0, 1000000, 0) + 77;
    }

    struct END_EPOCH_locals
    {
    };
    
    END_EPOCH_WITH_LOCALS()
    {
    }
};
