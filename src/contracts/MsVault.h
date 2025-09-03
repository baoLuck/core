using namespace QPI;

constexpr uint64 QBOND_MAX_EPOCH_COUNT = 1024ULL;
constexpr uint64 QBOND_MIN_STAKE_AMOUNT = 1000000ULL;
constexpr uint64 QBOND_MAX_QUEUE_SIZE = 10ULL;
constexpr uint64 QBOND_MIN_MBONDS_TO_STAKE = 10ULL;

struct MSVAULT2
{
};

struct MSVAULT : public ContractBase
{
public:
    struct StakeEntry
    {
        id staker;
        sint64 amount;
    };

    struct Stake_input
    {
        sint64 quMillions;
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
    Array<StakeEntry, 16> _stakeQueue;

    HashMap<uint16, uint64, QBOND_MAX_EPOCH_COUNT> _epochNameMap;

    struct Stake_locals
    {
        uint64 mbondNameForEpoch;
        sint64 availableMbonds;
        sint64 amountInQueue;
        uint64 counter;
        sint64 amountToStake;
        StakeEntry tempStakeEntry;
        QEARN::lock_input lock_input;
        QEARN::lock_output lock_output;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(Stake)
    {
        if (qpi.invocationReward() < input.quMillions * QBOND_MIN_STAKE_AMOUNT || !state._epochNameMap.get(qpi.epoch(), locals.mbondNameForEpoch))
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        locals.amountInQueue = input.quMillions;
        for (locals.counter = 0; locals.counter < QBOND_MAX_QUEUE_SIZE; locals.counter++)
        {
            if (state._stakeQueue.get(locals.counter).staker != SELF)
            {
                locals.amountInQueue += state._stakeQueue.get(locals.counter).amount;
            }
            else 
            {
                locals.tempStakeEntry.staker = qpi.invocator();
                locals.tempStakeEntry.amount = input.quMillions;
                state._stakeQueue.set(locals.counter, locals.tempStakeEntry);
                break;
            }
        }

        if (locals.amountInQueue < QBOND_MIN_MBONDS_TO_STAKE)
        {
            return;
        }

        locals.tempStakeEntry.staker = SELF;
        locals.tempStakeEntry.amount = 0;
        locals.amountToStake = 0;
        for (locals.counter = 0; locals.counter < QBOND_MAX_QUEUE_SIZE; locals.counter++)
        {
            if (state._stakeQueue.get(locals.counter).staker == SELF)
            {
                break;
            }
            qpi.transferShareOwnershipAndPossession(locals.mbondNameForEpoch, SELF, SELF, SELF, state._stakeQueue.get(locals.counter).amount, state._stakeQueue.get(locals.counter).staker);
            locals.amountToStake += state._stakeQueue.get(locals.counter).amount;
        }

        INVOKE_OTHER_CONTRACT_PROCEDURE(QEARN, lock, locals.lock_input, locals.lock_output, locals.amountToStake * QBOND_MIN_STAKE_AMOUNT);
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
        StakeEntry emptyEntry;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        locals.currentName = 1145979469ULL;   // MBND

        locals.chunk = (sint8) (48 + QPI::mod(QPI::div((uint64)qpi.epoch(), 100ULL), 10ULL));
        locals.currentName |= (uint64)locals.chunk << (4 * 8);

        locals.chunk = (sint8) (48 + QPI::mod(QPI::div((uint64)qpi.epoch(), 10ULL), 10ULL));
        locals.currentName |= (uint64)locals.chunk << (5 * 8);

        locals.chunk = (sint8) (48 + QPI::mod((uint64)qpi.epoch(), 10ULL));
        locals.currentName |= (uint64)locals.chunk << (6 * 8);

        qpi.issueAsset(locals.currentName, SELF, 0, 1000000000LL, 0);
        state._epochNameMap.set(qpi.epoch(), locals.currentName);

        locals.emptyEntry.staker = SELF;
        locals.emptyEntry.amount = 0;
        state._stakeQueue.setAll(locals.emptyEntry);
    }

    struct END_EPOCH_locals
    {
        uint64 mbondNameForEpoch;
        sint64 availableMbonds;
    };
    
    END_EPOCH_WITH_LOCALS()
    {
        if (state._epochNameMap.get(qpi.epoch(), locals.mbondNameForEpoch))
        {
            locals.availableMbonds = qpi.numberOfPossessedShares(locals.mbondNameForEpoch, SELF, SELF, SELF, SELF_INDEX, SELF_INDEX);
            qpi.transferShareOwnershipAndPossession(locals.mbondNameForEpoch, SELF, SELF, SELF, locals.availableMbonds, NULL_ID);
        }
    }
};
