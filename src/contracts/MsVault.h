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

    struct AddAskOrder_input
    {
        sint64 epoch;
        sint64 price;
        sint64 numberOfMBonds;
    };
    struct AddAskOrder_output
    {
        sint64 addedMBondsAmount;
    };

    struct RemoveAskOrder_input
    {
        sint64 epoch;
        sint64 price;
        sint64 numberOfMBonds;
    };
    struct RemoveAskOrder_output
    {
        sint64 removedMBondsAmount;
    };

    struct GetInfoPerEpoch_input
    {
        sint64 epoch;
    };
    struct GetInfoPerEpoch_output
    {
        uint64 stakersAmount;
        sint64 totalStaked;
    };

    struct GetAskOrders_input
    {
        sint64 epoch;
        sint64 offset;
    };
    struct GetAskOrders_output
    {
        uint64 counter;
        struct Order
        {
            id owner;
            sint64 epoch;
            sint64 numberOfMBonds;
            sint64 price;
        };

        Array<Order, 256> orders;
    };
    
private:
    Array<StakeEntry, 16> _stakeQueue;
    HashMap<uint16, MBondInfo, 1024> _epochMbondInfoMap;
    uint64 _qearnIncomeAmount;
    uint64 _counter;

    struct _Order
    {
        id owner;
        sint64 epoch;
        sint64 numberOfMBonds;
    };
    Collection<_Order, 1048576> _askOrders;
    Collection<_Order, 1048576> _bidOrders;

    struct _NumberOfReservedMBonds_input
    {
        id owner;
        sint64 epoch;
    } _numberOfReservedMBonds_input;

    struct _NumberOfReservedMBonds_output
    {
        sint64 amount;
    } _numberOfReservedMBonds_output;

    struct _NumberOfReservedMBonds_locals
    {
        sint64 elementIndex;
        id mbondIdentity;
        _Order order;
        MBondInfo tempMbondInfo;
    };

    PRIVATE_FUNCTION_WITH_LOCALS(_NumberOfReservedMBonds)
    {
        output.amount = 0;
        if (!state._epochMbondInfoMap.get((uint16)input.epoch, locals.tempMbondInfo))
        {
            return;
        }

        locals.mbondIdentity = SELF;
        locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;

        locals.elementIndex = state._askOrders.headIndex(locals.mbondIdentity, 0);
        while (locals.elementIndex != NULL_INDEX)
        {
            locals.order = state._askOrders.element(locals.elementIndex);
            if (locals.order.epoch == input.epoch && locals.order.owner == input.owner)
            {
                output.amount += locals.order.numberOfMBonds;
            }

            locals.elementIndex = state._askOrders.nextElementIndex(locals.elementIndex);
        }
    }

    struct Stake_locals
    {
        sint64 amountInQueue;
        sint64 index;
        sint64 tempAmount;
        uint64 counter;
        sint64 amountToStake;
        StakeEntry tempStakeEntry;
        MBondInfo tempMbondInfo;
        QEARN::lock_input lock_input;
        QEARN::lock_output lock_output;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(Stake)
    {
        if (input.quMillions <= 0 || (uint64) qpi.invocationReward() < input.quMillions * QBOND_MBOND_PRICE || !state._epochMbondInfoMap.get(qpi.epoch(), locals.tempMbondInfo))
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
            if (qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, state._stakeQueue.get(locals.counter).staker, state._stakeQueue.get(locals.counter).staker, SELF_INDEX, SELF_INDEX) <= 0)
            {
                locals.tempMbondInfo.stakersAmount++;
            }
            qpi.transferShareOwnershipAndPossession(locals.tempMbondInfo.name, SELF, SELF, SELF, state._stakeQueue.get(locals.counter).amount, state._stakeQueue.get(locals.counter).staker);
            locals.amountToStake += state._stakeQueue.get(locals.counter).amount;

            state._stakeQueue.set(locals.counter, locals.tempStakeEntry);
        }

        locals.tempMbondInfo.totalStaked += locals.amountToStake;
        state._epochMbondInfoMap.replace(qpi.epoch(), locals.tempMbondInfo);

        INVOKE_OTHER_CONTRACT_PROCEDURE(QEARN, lock, locals.lock_input, locals.lock_output, locals.amountToStake * QBOND_MBOND_PRICE);
    }

    struct TransferMBondOwnershipAndPossession_locals
    {
        MBondInfo tempMbondInfo;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TransferMBondOwnershipAndPossession)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        state._numberOfReservedMBonds_input.epoch = input.epoch;
        state._numberOfReservedMBonds_input.owner = qpi.invocator();
        CALL(_NumberOfReservedMBonds, state._numberOfReservedMBonds_input, state._numberOfReservedMBonds_output);

        if (state._epochMbondInfoMap.get((uint16)input.epoch, locals.tempMbondInfo)
                && qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedMBonds_output.amount < input.numberOfMBonds)
        {
            output.transferredMBonds = 0;
        }
        else
        {
            if (qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, input.newOwnerAndPossessor, input.newOwnerAndPossessor, SELF_INDEX, SELF_INDEX) <= 0)
            {
                locals.tempMbondInfo.stakersAmount++;
            }
            output.transferredMBonds = qpi.transferShareOwnershipAndPossession(locals.tempMbondInfo.name, SELF, qpi.invocator(), qpi.invocator(), input.numberOfMBonds, input.newOwnerAndPossessor) < 0 ? 0 : input.numberOfMBonds;
            if (qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) <= 0)
            {
                locals.tempMbondInfo.stakersAmount--;
            }
            state._epochMbondInfoMap.replace((uint16)input.epoch, locals.tempMbondInfo);
        }
    }

    struct AddAskOrder_locals
    {
        MBondInfo tempMbondInfo;
        id mbondIdentity;
        sint64 elementIndex;
        _Order order;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AddAskOrder)
    {
        state._counter = 1;
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        if (input.price <= 0 || input.numberOfMBonds <= 0 || !state._epochMbondInfoMap.get(input.epoch, locals.tempMbondInfo))
        {
            state._counter = 2;
            output.addedMBondsAmount = 0;
            return;
        }

        state._counter = 3;
        state._numberOfReservedMBonds_input.epoch = input.epoch;
        state._numberOfReservedMBonds_input.owner = qpi.invocator();
        CALL(_NumberOfReservedMBonds, state._numberOfReservedMBonds_input, state._numberOfReservedMBonds_output);
        if (qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedMBonds_output.amount < input.numberOfMBonds)
        {
            state._counter = 4;
            output.addedMBondsAmount = 0;
            return;
        }

        state._counter = 5;
        output.addedMBondsAmount = input.numberOfMBonds;

        locals.mbondIdentity = SELF;
        locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;

        if (state._askOrders.population(locals.mbondIdentity) == 0)
        {
            state._counter = 22;
            locals.order.epoch = input.epoch;
            locals.order.numberOfMBonds = input.numberOfMBonds;
            locals.order.owner = qpi.invocator();
            state._askOrders.add(locals.mbondIdentity, locals.order, -input.price);
            return;
        }

        state._counter = 6;
        locals.elementIndex = state._askOrders.headIndex(locals.mbondIdentity, 0);
        while (locals.elementIndex != NULL_INDEX)
        {
            state._counter = 7;
            if (input.price < -state._askOrders.priority(locals.elementIndex))
            {
                state._counter = 8;
                locals.order.epoch = input.epoch;
                locals.order.numberOfMBonds = input.numberOfMBonds;
                locals.order.owner = qpi.invocator();
                state._askOrders.add(locals.mbondIdentity, locals.order, -input.price);
                break;
            }
            else if (input.price == -state._askOrders.priority(locals.elementIndex))
            {
                state._counter = 9;
                if (state._askOrders.element(locals.elementIndex).owner == qpi.invocator())
                {
                    state._counter = 10;
                    locals.order = state._askOrders.element(locals.elementIndex);
                    locals.order.numberOfMBonds += input.numberOfMBonds;
                    state._askOrders.replace(locals.elementIndex, locals.order);
                    break;
                }
            }

            state._counter = 11;
            if (state._askOrders.nextElementIndex(locals.elementIndex) == NULL_INDEX)
            {
                state._counter = 12;
                locals.order.epoch = input.epoch;
                locals.order.numberOfMBonds = input.numberOfMBonds;
                locals.order.owner = qpi.invocator();
                state._askOrders.add(locals.mbondIdentity, locals.order, -input.price);
                break;
            }

            state._counter = 13;
            locals.elementIndex = state._askOrders.nextElementIndex(locals.elementIndex);
        }
    }

    struct RemoveAskOrder_locals
    {
        MBondInfo tempMbondInfo;
        id mbondIdentity;
        sint64 elementIndex;
        _Order order;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(RemoveAskOrder)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        output.removedMBondsAmount = 0;

        if (input.price <= 0 || input.numberOfMBonds <= 0 || !state._epochMbondInfoMap.get(input.epoch, locals.tempMbondInfo))
        {
            state._counter = 2;
            return;
        }

        state._counter = 5;

        locals.mbondIdentity = SELF;
        locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;

        state._counter = 6;
        locals.elementIndex = state._askOrders.headIndex(locals.mbondIdentity, 0);
        while (locals.elementIndex != NULL_INDEX)
        {
            if (input.price == -state._askOrders.priority(locals.elementIndex) && state._askOrders.element(locals.elementIndex).owner == qpi.invocator())
            {
                if (state._askOrders.element(locals.elementIndex).numberOfMBonds <= input.numberOfMBonds)
                {
                    state._askOrders.remove(locals.elementIndex);
                    output.removedMBondsAmount = input.numberOfMBonds;
                }
                else
                {
                    locals.order = state._askOrders.element(locals.elementIndex);
                    locals.order.numberOfMBonds -= input.numberOfMBonds;
                    state._askOrders.replace(locals.elementIndex, locals.order);
                }
                break;
            }

            locals.elementIndex = state._askOrders.nextElementIndex(locals.elementIndex);
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

        locals.index = state._epochMbondInfoMap.getElementIndex(input.epoch);

        if (locals.index == NULL_INDEX)
        {
            return;
        }

        output.totalStaked = state._epochMbondInfoMap.value(locals.index).totalStaked;
        output.stakersAmount = state._epochMbondInfoMap.value(locals.index).stakersAmount;
    }

    struct GetAskOrders_locals
    {
        MBondInfo tempMbondInfo;
        id mbondIdentity;
        sint64 elementIndex;
        sint64 arrayElementIndex;
        GetAskOrders_output::Order tempOrder;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetAskOrders)
    {
        if (!state._epochMbondInfoMap.get(input.epoch, locals.tempMbondInfo))
        {
            return;
        }

        locals.arrayElementIndex = 0;
        locals.mbondIdentity = SELF;
        locals.mbondIdentity.u64._3 = locals.tempMbondInfo.name;

        locals.elementIndex = state._askOrders.headIndex(locals.mbondIdentity, 0);
        while (locals.elementIndex != NULL_INDEX)
        {
            locals.tempOrder.owner = state._askOrders.element(locals.elementIndex).owner;
            locals.tempOrder.epoch = state._askOrders.element(locals.elementIndex).epoch;
            locals.tempOrder.numberOfMBonds = state._askOrders.element(locals.elementIndex).numberOfMBonds;
            locals.tempOrder.price = -state._askOrders.priority(locals.elementIndex);
            output.orders.set(locals.arrayElementIndex, locals.tempOrder);
            locals.arrayElementIndex++;
            locals.elementIndex = state._askOrders.nextElementIndex(locals.elementIndex);
        }

        output.counter = state._counter;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(Stake, 1);
        REGISTER_USER_PROCEDURE(TransferMBondOwnershipAndPossession, 2);
        REGISTER_USER_PROCEDURE(AddAskOrder, 3);
        REGISTER_USER_PROCEDURE(RemoveAskOrder, 4);

        REGISTER_USER_FUNCTION(GetInfoPerEpoch, 1);
        REGISTER_USER_FUNCTION(GetAskOrders, 2);
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
        sint64 totalReward;
        sint64 rewardPerMBond;
        Asset tempAsset;
        MBondInfo tempMbondInfo;
        AssetOwnershipIterator assetIt;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        if (state._qearnIncomeAmount > 0 && state._epochMbondInfoMap.get(qpi.epoch() - 53, locals.tempMbondInfo))
        {
            locals.totalReward = state._qearnIncomeAmount - locals.tempMbondInfo.totalStaked * QBOND_MBOND_PRICE;
            locals.rewardPerMBond = QPI::div(locals.totalReward, locals.tempMbondInfo.totalStaked);
            
            locals.tempAsset.assetName = locals.tempMbondInfo.name;
            locals.tempAsset.issuer = SELF;
            locals.assetIt.begin(locals.tempAsset);
            while (!locals.assetIt.reachedEnd())
            {
                qpi.transfer(locals.assetIt.owner(), (QBOND_MBOND_PRICE + locals.rewardPerMBond) * locals.assetIt.numberOfOwnedShares());
                qpi.transferShareOwnershipAndPossession(
                        locals.tempMbondInfo.name,
                        SELF,
                        locals.assetIt.owner(),
                        locals.assetIt.owner(),
                        locals.assetIt.numberOfOwnedShares(),
                        SELF);
                locals.assetIt.next();
            }
            state._qearnIncomeAmount = 0;
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
        locals.tempMbondInfo.name = locals.currentName;
        locals.tempMbondInfo.totalStaked = 0;
        locals.tempMbondInfo.stakersAmount = 0;
        state._epochMbondInfoMap.set(qpi.epoch(), locals.tempMbondInfo);
        state._stakeQueue.setAll(locals.emptyEntry);
    }

    struct POST_INCOMING_TRANSFER_locals
    {
        MBondInfo tempMbondInfo;
    };

    POST_INCOMING_TRANSFER_WITH_LOCALS()
    {
        if (input.sourceId == id(QEARN_CONTRACT_INDEX, 0, 0, 0) && state._epochMbondInfoMap.get(qpi.epoch() - 52, locals.tempMbondInfo))
        {
            state._qearnIncomeAmount = input.amount;
        }
    }

    struct END_EPOCH_locals
    {
        sint64 availableMbonds;
        MBondInfo tempMbondInfo;
    };
    
    END_EPOCH_WITH_LOCALS()
    {
        if (state._epochMbondInfoMap.get(qpi.epoch(), locals.tempMbondInfo))
        {
            locals.availableMbonds = qpi.numberOfPossessedShares(locals.tempMbondInfo.name, SELF, SELF, SELF, SELF_INDEX, SELF_INDEX);
            qpi.transferShareOwnershipAndPossession(locals.tempMbondInfo.name, SELF, SELF, SELF, locals.availableMbonds, NULL_ID);
        }
    }
};
