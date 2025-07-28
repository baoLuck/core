using namespace QPI;
 
constexpr uint64 ESCROW_INITIAL_MAX_DEALS = 32;
constexpr uint64 ESCROW_MAX_DEALS = ESCROW_INITIAL_MAX_DEALS * X_MULTIPLIER;
constexpr uint64 ESCROW_MAX_DEALS_PER_USER = 8;
constexpr uint64 ESCROW_MAX_ASSETS_IN_DEAL = 4;
constexpr uint64 ESCROW_MAX_RESERVED_ASSETS = 256;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
public:
    struct AssetWithAmount
    {
        id issuer;
        uint64 name;
        sint64 amount;
    };

    struct Deal
    {
        id acceptorId;
        sint64 offeredQU;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> offeredAssets;
        sint8 offeredAssetsNumber;
        sint64 requestedQU;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> requestedAssets;
        sint8 requestedAssetsNumber;
    };

    struct CreateDeal_input
    {
        sint64 delta;
        id acceptorId;
        sint64 offeredQU;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> offeredAssets;
        sint8 offeredAssetsNumber;
        sint64 requestedQU;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> requestedAssets;
        sint8 requestedAssetsNumber;
    };

    struct CreateDeal_output
    {
    };

    struct GetDeals_input
    {
        id owner;
    };

    struct GetDeals_output
    {
        sint64 currentValue;
        sint64 ownedDealsAmount;
        sint64 proposedDealsAmount;
        sint64 openedDealsAmount;
        struct _DealEntity
        {
            sint64 index;
            Deal deal;
        };
        Array<_DealEntity, ESCROW_MAX_DEALS_PER_USER> ownedDeals;
        Array<_DealEntity, ESCROW_MAX_DEALS_PER_USER> proposedDeals;
        Array<_DealEntity, 32> openedDeals;
    };

    struct AcceptDeal_input
    {
        sint64 index;
    };

    struct AcceptDeal_output
    {
    };

    struct MakeDealOpened_input
    {
        sint64 index;
    };

    struct MakeDealOpened_output
    {
    };

    struct CancelDeal_input
    {
        sint64 index;
    };

    struct CancelDeal_output
    {
    };

private:
    sint64 _counter;

    Collection<Deal, ESCROW_MAX_DEALS> _deals;
    Collection<AssetWithAmount, ESCROW_MAX_RESERVED_ASSETS> _reservedAssets;

    struct _NumberOfReservedShares_input
    {
        id owner;
        id issuer;
        uint64 assetName;
    } _numberOfReservedShares_input;

    struct _NumberOfReservedShares_output
    {
        sint64 amount;
    } _numberOfReservedShares_output;

    struct _NumberOfReservedShares_locals
    {
        sint64 elementIndex;
        AssetWithAmount assetWithAmount;
    };

    struct _TradeMessage
    {
        unsigned int _contractIndex;
        unsigned int _type;
        
        sint64 marker;
        sint64 numberOfShares;

        char _terminator;
    } _tradeMessage;

    PRIVATE_FUNCTION_WITH_LOCALS(_NumberOfReservedShares)
    {
        output.amount = 0;

        locals.elementIndex = state._reservedAssets.headIndex(input.owner, 0);
        while (locals.elementIndex != NULL_INDEX)
        {
            locals.assetWithAmount = state._reservedAssets.element(locals.elementIndex);
            if (locals.assetWithAmount.name == input.assetName && locals.assetWithAmount.issuer == input.issuer)
            {
                output.amount = locals.assetWithAmount.amount;
            }

            locals.elementIndex = state._reservedAssets.nextElementIndex(locals.elementIndex);
        }
    }

    struct CreateDeal_locals
    {
        Deal newDeal;
        sint64 counter;
        sint64 elementIndex;
        AssetWithAmount tempAssetWithAmount;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateDeal)
    {
        state._counter += input.delta;
        if (state._deals.population() >= ESCROW_MAX_DEALS)
        {
            return;
        }

        if (state._deals.population(qpi.invocator()) >= ESCROW_MAX_DEALS_PER_USER)
        {
            return;
        }

        for (locals.counter = 0; locals.counter < input.offeredAssetsNumber; locals.counter++)
        {
            state._numberOfReservedShares_input.issuer = input.offeredAssets.get(locals.counter).issuer;
            state._numberOfReservedShares_input.assetName = input.offeredAssets.get(locals.counter).name;
            state._numberOfReservedShares_input.owner = qpi.invocator();
            CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
            if (qpi.numberOfPossessedShares(input.offeredAssets.get(locals.counter).name, input.offeredAssets.get(locals.counter).issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedShares_output.amount < input.offeredAssets.get(locals.counter).amount)
            {
                return;
            }
        }

        locals.newDeal.acceptorId = (input.acceptorId == NULL_ID || input.acceptorId == qpi.invocator()) ? SELF : input.acceptorId;
        locals.newDeal.offeredQU = input.offeredQU;
        locals.newDeal.offeredAssets = input.offeredAssets;
        locals.newDeal.offeredAssetsNumber = input.offeredAssetsNumber;
        locals.newDeal.requestedQU = input.requestedQU;
        locals.newDeal.requestedAssets = input.requestedAssets;
        locals.newDeal.requestedAssetsNumber = input.requestedAssetsNumber;
        state._deals.add(qpi.invocator(), locals.newDeal, 0);

        for (locals.counter = 0; locals.counter < input.offeredAssetsNumber; locals.counter++)
        {
            state._numberOfReservedShares_input.issuer = input.offeredAssets.get(locals.counter).issuer;
            state._numberOfReservedShares_input.assetName = input.offeredAssets.get(locals.counter).name;
            state._numberOfReservedShares_input.owner = qpi.invocator();
            CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
            if (state._numberOfReservedShares_output.amount == 0)
            {
                locals.tempAssetWithAmount = input.offeredAssets.get(locals.counter);
                state._reservedAssets.add(qpi.invocator(), locals.tempAssetWithAmount, 0);
            }
            else
            {
                locals.elementIndex = state._reservedAssets.headIndex(qpi.invocator());
                while (locals.elementIndex != NULL_INDEX)
                {
                    locals.tempAssetWithAmount = state._reservedAssets.element(locals.elementIndex);
                    if (locals.tempAssetWithAmount.name == input.offeredAssets.get(locals.counter).name && locals.tempAssetWithAmount.issuer == input.offeredAssets.get(locals.counter).issuer)
                    {
                        locals.tempAssetWithAmount.amount += input.offeredAssets.get(locals.counter).amount;
                        state._reservedAssets.replace(locals.elementIndex, locals.tempAssetWithAmount);
                    }

                    locals.elementIndex = state._reservedAssets.nextElementIndex(locals.elementIndex);
                }
            }
        } 
    }

    struct GetDeals_locals
    {
        sint64 elementIndex;
        sint64 elementIndex2;
        sint64 elementIndex3;
        GetDeals_output::_DealEntity tempDealEntity;
        Deal tempDeal;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetDeals)
    {
        output.currentValue = state._counter;
        output.ownedDealsAmount = state._deals.population(input.owner);

        locals.elementIndex = state._deals.headIndex(input.owner);
        locals.elementIndex2 = 0;
        while (locals.elementIndex != NULL_INDEX
            && locals.elementIndex2 < ESCROW_MAX_DEALS_PER_USER)
        {
            locals.tempDealEntity.index = locals.elementIndex;
            locals.tempDealEntity.deal = state._deals.element(locals.elementIndex);
            output.ownedDeals.set(locals.elementIndex2, locals.tempDealEntity);
            locals.elementIndex = state._deals.nextElementIndex(locals.elementIndex);
            locals.elementIndex2++;
        }

        locals.elementIndex2 = 0;
        locals.elementIndex3 = 0;
        for (locals.elementIndex = 0; locals.elementIndex < ESCROW_MAX_DEALS; locals.elementIndex++)
        {
            locals.tempDeal = state._deals.element(locals.elementIndex);
            if (locals.tempDeal.acceptorId == input.owner && locals.elementIndex2 < ESCROW_MAX_DEALS_PER_USER)
            {
                locals.tempDealEntity.index = locals.elementIndex;
                locals.tempDealEntity.deal = locals.tempDeal;
                locals.tempDealEntity.deal.acceptorId = state._deals.pov(locals.elementIndex);
                output.proposedDeals.set(locals.elementIndex2, locals.tempDealEntity);
                locals.elementIndex2++;
            }
            if (locals.tempDeal.acceptorId == SELF
                && locals.elementIndex3 < ESCROW_MAX_DEALS_PER_USER
                && state._deals.pov(locals.elementIndex) != input.owner)
            {
                    locals.tempDealEntity.index = locals.elementIndex;
                    locals.tempDealEntity.deal = locals.tempDeal;
                    locals.tempDealEntity.deal.acceptorId = state._deals.pov(locals.elementIndex);
                    output.openedDeals.set(locals.elementIndex3, locals.tempDealEntity);
                    locals.elementIndex3++;
            }
        }
        output.proposedDealsAmount = locals.elementIndex2;
        output.openedDealsAmount = locals.elementIndex3;
    }

    struct AcceptDeal_locals
    {
        Deal tempDeal;
        sint64 counter;
        sint64 transferedShares;
        sint64 elementIndex;
        AssetWithAmount tempAssetWithAmount;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AcceptDeal)
    {
        locals.tempDeal = state._deals.element(input.index);
        if (locals.tempDeal.acceptorId != SELF && locals.tempDeal.acceptorId != qpi.invocator())
        {
            return;
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.requestedAssetsNumber; locals.counter++)
        {
            if (qpi.numberOfPossessedShares(locals.tempDeal.requestedAssets.get(locals.counter).name, locals.tempDeal.requestedAssets.get(locals.counter).issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < locals.tempDeal.requestedAssets.get(locals.counter).amount)
            {
                return;
            }
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.offeredAssetsNumber; locals.counter++)
        {
            locals.transferedShares = qpi.transferShareOwnershipAndPossession(
                                                locals.tempDeal.offeredAssets.get(locals.counter).name,
                                                locals.tempDeal.offeredAssets.get(locals.counter).issuer,
                                                state._deals.pov(input.index),
                                                state._deals.pov(input.index),
                                                locals.tempDeal.offeredAssets.get(locals.counter).amount,
                                                qpi.invocator());

            state._numberOfReservedShares_input.issuer = locals.tempDeal.offeredAssets.get(locals.counter).issuer;
            state._numberOfReservedShares_input.assetName = locals.tempDeal.offeredAssets.get(locals.counter).name;
            state._numberOfReservedShares_input.owner = state._deals.pov(input.index);
            CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
            locals.elementIndex = state._reservedAssets.headIndex(state._deals.pov(input.index));
            while (locals.elementIndex != NULL_INDEX)
            {
                locals.tempAssetWithAmount = state._reservedAssets.element(locals.elementIndex);
                if (locals.tempAssetWithAmount.name == locals.tempDeal.offeredAssets.get(locals.counter).name
                    && locals.tempAssetWithAmount.issuer == locals.tempDeal.offeredAssets.get(locals.counter).issuer)
                {
                    if (state._numberOfReservedShares_output.amount - locals.transferedShares <= 0)
                    {
                        state._reservedAssets.remove(locals.elementIndex);
                        break;
                    }
                    else
                    {
                        locals.tempAssetWithAmount.amount -= locals.transferedShares;
                        state._reservedAssets.replace(locals.elementIndex, locals.tempAssetWithAmount);
                    }
                }
                locals.elementIndex = state._reservedAssets.nextElementIndex(locals.elementIndex);
            }
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.requestedAssetsNumber; locals.counter++)
        {
            qpi.transferShareOwnershipAndPossession(
                locals.tempDeal.requestedAssets.get(locals.counter).name,
                locals.tempDeal.requestedAssets.get(locals.counter).issuer,
                qpi.invocator(),
                qpi.invocator(),
                locals.tempDeal.requestedAssets.get(locals.counter).amount,
                state._deals.pov(input.index));
        }

        qpi.transfer(qpi.invocator(), locals.tempDeal.offeredQU);
        qpi.transfer(state._deals.pov(input.index), locals.tempDeal.requestedQU);

        state._deals.remove(input.index);
    }

    struct MakeDealOpened_locals
    {
        Deal tempDeal;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(MakeDealOpened)
    {
        locals.tempDeal = state._deals.element(input.index);
        if (state._deals.pov(input.index) != qpi.invocator() || locals.tempDeal.acceptorId == SELF)
        {
            return;
        }

        locals.tempDeal.acceptorId = SELF;
        state._deals.replace(input.index, locals.tempDeal);
    }

    struct CancelDeal_locals
    {
        Deal tempDeal;
        sint64 counter;
        sint64 elementIndex;
        AssetWithAmount tempAssetWithAmount;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CancelDeal)
    {
        locals.tempDeal = state._deals.element(input.index);
        if (state._deals.pov(input.index) != qpi.invocator())
        {
            return;
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.offeredAssetsNumber; locals.counter++)
        {
            state._numberOfReservedShares_input.issuer = locals.tempDeal.offeredAssets.get(locals.counter).issuer;
            state._numberOfReservedShares_input.assetName = locals.tempDeal.offeredAssets.get(locals.counter).name;
            state._numberOfReservedShares_input.owner = qpi.invocator();
            CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
            locals.elementIndex = state._reservedAssets.headIndex(qpi.invocator());
            while (locals.elementIndex != NULL_INDEX)
            {
                locals.tempAssetWithAmount = state._reservedAssets.element(locals.elementIndex);
                if (locals.tempAssetWithAmount.name == locals.tempDeal.offeredAssets.get(locals.counter).name
                    && locals.tempAssetWithAmount.issuer == locals.tempDeal.offeredAssets.get(locals.counter).issuer)
                {
                    if (state._numberOfReservedShares_output.amount - locals.tempDeal.offeredAssets.get(locals.counter).amount <= 0)
                    {
                        state._reservedAssets.remove(locals.elementIndex);
                        break;
                    }
                    else
                    {
                        locals.tempAssetWithAmount.amount -= locals.tempDeal.offeredAssets.get(locals.counter).amount;
                        state._reservedAssets.replace(locals.elementIndex, locals.tempAssetWithAmount);
                    }
                }
                locals.elementIndex = state._reservedAssets.nextElementIndex(locals.elementIndex);
            }
        }

        state._deals.remove(input.index);
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(CreateDeal, 1);
        REGISTER_USER_FUNCTION(GetDeals, 2);
        REGISTER_USER_PROCEDURE(AcceptDeal, 3);
        REGISTER_USER_PROCEDURE(MakeDealOpened, 4);
        REGISTER_USER_PROCEDURE(CancelDeal, 5);
    }

    INITIALIZE()
    {
        state._counter = 0ULL;
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    END_EPOCH()
    {
    }
};
