using namespace QPI;

constexpr uint64 QBOND_MAX_EPOCH_COUNT = 1024ULL;
constexpr uint64 QBOND_BASE_STAKE_AMOUNT = 1000000ULL;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
public:
    struct Stake_input
    {
        uint64 millions;
    };

    struct Stake_output
    {
    };
    
    struct GetCounter_input
    {
    };

    struct GetCounter_output
    {
        uint64 counter;
    };
    
private:
    uint64 _counter;

    HashMap<uint16, uint64, QBOND_MAX_EPOCH_COUNT> _epochNameMap;

    struct Stake_locals
    {
        uint64 mbondNameForEpoch;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(Stake)
    {
        // qpi.numberOfPossessedShares(state._epochNameMap.);
    }

    PUBLIC_FUNCTION(GetCounter)
    {
        output.counter = state._counter;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(Stake, 1);
        REGISTER_USER_FUNCTION(GetCounter, 1);
    }

    INITIALIZE()
    {
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    struct BEGIN_EPOCH_locals
    {
        sint8 chunk;
        uint64 currentName;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        locals.currentName = 1145979469ULL;   // MBND

        locals.chunk = 48 + QPI::mod(QPI::div((uint64)qpi.epoch(), 100ULL), 10ULL);
        locals.currentName |= (uint64)locals.chunk << (4 * 8);

        locals.chunk = 48 + QPI::mod(QPI::div((uint64)qpi.epoch(), 10ULL), 10ULL);
        locals.currentName |= (uint64)locals.chunk << (5 * 8);

        locals.chunk = 48 + QPI::mod((uint64)qpi.epoch(), 10ULL);
        locals.currentName |= (uint64)locals.chunk << (6 * 8);

        qpi.issueAsset(locals.currentName, SELF, 0, 1000000, 0);
        state._epochNameMap.set(qpi.epoch(), locals.currentName);
    }

    struct END_EPOCH_locals
    {
        uint64 mbondNameForEpoch;
        sint64 possesedMbonds;
    };
    
    END_EPOCH_WITH_LOCALS()
    {
        if (state._epochNameMap.get(qpi.epoch(), locals.mbondNameForEpoch))
        {
            locals.possesedMbonds = qpi.numberOfPossessedShares(locals.mbondNameForEpoch, SELF, SELF, SELF, SELF_INDEX, SELF_INDEX);
		    qpi.transferShareOwnershipAndPossession(locals.mbondNameForEpoch, SELF, SELF, SELF, locals.possesedMbonds, NULL_ID);
        }
    }
};
