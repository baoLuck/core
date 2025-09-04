using namespace QPI;

constexpr uint64 QBOND_MAX_EPOCH_COUNT = 1024ULL;
constexpr uint64 QBOND_MBOND_PRICE = 1000000ULL;
constexpr uint64 QBOND_MAX_QUEUE_SIZE = 10ULL;
constexpr uint64 QBOND_MIN_MBONDS_TO_STAKE = 10ULL;
constexpr sint64 QBOND_MBONDS_EMISSION = 1000000000LL;
constexpr uint16 QBOND_START_EPOCH = 170;

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

    struct MBondInfo
    {
        uint64 name;
        sint64 stakersAmount;
        sint64 totalStaked;
    };

    struct Stake_input
    {
        sint64 quMillions;
    };

    struct Stake_output
    {
    };

    struct TransferMBondOwnershipAndPossession_input
    {
        id newOwnerAndPossessor;
        sint64 epoch;
        sint64 numberOfMBonds;
    };

    struct TransferMBondOwnershipAndPossession_output
    {
        sint64 transferredMBonds;
    };

    struct GetInfoPerEpoch_input
    {
        uint16 epoch;
    };

    struct GetInfoPerEpoch_output
    {
        uint64 stakersAmount;
        sint64 totalStaked;
        uint64 counter;
    };
    
private:
    Array<StakeEntry, 16> _stakeQueue;
    HashMap<uint16, MBondInfo, 1024> _epochMbondInfoMap;
    MBondInfo _tempMbondInfo;
    uint64 _counter;

    struct Stake_locals
    {
        sint64 amountInQueue;
        sint64 index;
        sint64 tempAmount;
        uint64 counter;
        sint64 amountToStake;
        StakeEntry tempStakeEntry;
        QEARN::lock_input lock_input;
        QEARN::lock_output lock_output;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(Stake)
    {
        if (input.quMillions <= 0 || (uint64) qpi.invocationReward() < input.quMillions * QBOND_MBOND_PRICE || !state._epochMbondInfoMap.get(qpi.epoch(), state._tempMbondInfo))
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
            if (qpi.numberOfPossessedShares(state._tempMbondInfo.name, SELF, state._stakeQueue.get(locals.counter).staker, state._stakeQueue.get(locals.counter).staker, SELF_INDEX, SELF_INDEX) <= 0)
            {
                state._tempMbondInfo.stakersAmount++;
            }
            qpi.transferShareOwnershipAndPossession(state._tempMbondInfo.name, SELF, SELF, SELF, state._stakeQueue.get(locals.counter).amount, state._stakeQueue.get(locals.counter).staker);
            locals.amountToStake += state._stakeQueue.get(locals.counter).amount;

            state._stakeQueue.set(locals.counter, locals.tempStakeEntry);
        }

        state._tempMbondInfo.totalStaked += locals.amountToStake;
        state._epochMbondInfoMap.replace(qpi.epoch(), state._tempMbondInfo);

        INVOKE_OTHER_CONTRACT_PROCEDURE(QEARN, lock, locals.lock_input, locals.lock_output, locals.amountToStake * QBOND_MBOND_PRICE);
    }

    PUBLIC_PROCEDURE(TransferMBondOwnershipAndPossession)
    {
        state._counter = 1;

        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        state._counter = 2;

        if (state._epochMbondInfoMap.get(input.epoch, state._tempMbondInfo)
                && qpi.numberOfPossessedShares(state._tempMbondInfo.name, SELF, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfMBonds)
        {
            output.transferredMBonds = 0;
            state._counter = 3;
        }
        else
        {
            output.transferredMBonds = qpi.transferShareOwnershipAndPossession(state._tempMbondInfo.name, SELF, qpi.invocator(), qpi.invocator(), input.numberOfMBonds, input.newOwnerAndPossessor) < 0 ? 0 : input.numberOfMBonds;
            state._counter = 4;
        }
    }

    struct GetInfoPerEpoch_locals
    {
        sint64 index;
    };
    
    PUBLIC_FUNCTION_WITH_LOCALS(GetInfoPerEpoch)
    {
        output.totalStaked = 0;
        output.stakersAmount = 0;
        output.counter = state._counter;

        locals.index = state._epochMbondInfoMap.getElementIndex(input.epoch);

        if (input.epoch < QBOND_START_EPOCH || locals.index == NULL_INDEX)
        {
            return;
        }

        output.totalStaked = state._epochMbondInfoMap.value(locals.index).totalStaked;
        output.stakersAmount = state._epochMbondInfoMap.value(locals.index).stakersAmount;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(Stake, 1);
        REGISTER_USER_PROCEDURE(TransferMBondOwnershipAndPossession, 2);

        REGISTER_USER_FUNCTION(GetInfoPerEpoch, 1);
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
        QEARN::getEndedStatus_input getEndedStatus_input;
        QEARN::getEndedStatus_output getEndedStatus_output;
        uint64 rewardPerMBond;
        AssetOwnershipIterator assetIt;
        Asset tempAsset;
        uint64 transferredShares;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        CALL_OTHER_CONTRACT_FUNCTION(QEARN, getEndedStatus, locals.getEndedStatus_input, locals.getEndedStatus_output);
        if (locals.getEndedStatus_output.fullyUnlockedAmount > 0 && state._epochMbondInfoMap.get(qpi.epoch() - 52, state._tempMbondInfo))
        {
            locals.rewardPerMBond = QPI::div(locals.getEndedStatus_output.fullyRewardedAmount, (uint64) state._tempMbondInfo.totalStaked);
            locals.tempAsset.assetName = state._tempMbondInfo.name;
            locals.tempAsset.issuer = SELF;
            locals.transferredShares = 0;
            locals.assetIt.begin(locals.tempAsset);
            while (!locals.assetIt.reachedEnd())
            {
                locals.transferredShares += locals.assetIt.numberOfOwnedShares();
                qpi.transferShareOwnershipAndPossession(
                        state._tempMbondInfo.name,
                        SELF,
                        locals.assetIt.owner(),
                        locals.assetIt.owner(),
                        locals.assetIt.numberOfOwnedShares(),
                        SELF);
                qpi.transfer(locals.assetIt.owner(), locals.assetIt.numberOfOwnedShares() * QBOND_MBOND_PRICE + locals.rewardPerMBond);
                locals.assetIt.next();
            }
        }

        locals.currentName = 1145979469ULL;   // MBND

        locals.chunk = (sint8) (48 + QPI::mod(QPI::div((uint64)qpi.epoch(), 100ULL), 10ULL));
        locals.currentName |= (uint64)locals.chunk << (4 * 8);

        locals.chunk = (sint8) (48 + QPI::mod(QPI::div((uint64)qpi.epoch(), 10ULL), 10ULL));
        locals.currentName |= (uint64)locals.chunk << (5 * 8);

        locals.chunk = (sint8) (48 + QPI::mod((uint64)qpi.epoch(), 10ULL));
        locals.currentName |= (uint64)locals.chunk << (6 * 8);

        qpi.issueAsset(locals.currentName, SELF, 0, QBOND_MBONDS_EMISSION, 0);

        locals.emptyEntry.staker = SELF;
        locals.emptyEntry.amount = 0;
        state._tempMbondInfo.name = locals.currentName;
        state._tempMbondInfo.totalStaked = 0;
        state._tempMbondInfo.stakersAmount = 0;
        state._epochMbondInfoMap.set(qpi.epoch(), state._tempMbondInfo);
        state._stakeQueue.setAll(locals.emptyEntry);
    }

    struct END_EPOCH_locals
    {
        sint64 availableMbonds;
    };
    
    END_EPOCH_WITH_LOCALS()
    {
        if (state._epochMbondInfoMap.get(qpi.epoch(), state._tempMbondInfo))
        {
            locals.availableMbonds = qpi.numberOfPossessedShares(state._tempMbondInfo.name, SELF, SELF, SELF, SELF_INDEX, SELF_INDEX);
            qpi.transferShareOwnershipAndPossession(state._tempMbondInfo.name, SELF, SELF, SELF, locals.availableMbonds, NULL_ID);
        }
    }
};
