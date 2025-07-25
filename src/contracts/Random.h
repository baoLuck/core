using namespace QPI;

constexpr uint64 ESCROW_INITIAL_MAX_DEALS = 128;
constexpr uint64 ESCROW_MAX_DEALS = ESCROW_INITIAL_MAX_DEALS * X_MULTIPLIER;
constexpr uint64 ESCROW_MAX_DEALS_PER_USER = 8;
constexpr uint64 ESCROW_MAX_ASSETS_IN_DEAL = 4;

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
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> offeredAssets;
        sint8 offeredAssetsNumber;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> requestedAssets;
        sint8 requestedAssetsNubmer;
    };

    struct CreateDeal_input
    {
        sint64 delta;
        id acceptorId;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> offeredAssets;
        sint8 offeredAssetsNumber;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> requestedAssets;
        sint8 requestedAssetsNubmer;
    };

    struct CreateDeal_output
    {
        sint64 currentValue;
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
        struct _DealEntity
        {
            sint64 index;
            Deal deal;
        };
        Array<_DealEntity, ESCROW_MAX_DEALS_PER_USER> ownedDeals;
        Array<_DealEntity, ESCROW_MAX_DEALS_PER_USER> proposedDeals;
    };

    struct AcceptDeal_input
    {
        sint64 index;
    };

    struct AcceptDeal_output
    {
    };

private:
    sint64 _counter;
    struct _DealWithState
    {
        Deal deal;
        bit isActive;
    };
    
    Collection<_DealWithState, ESCROW_MAX_DEALS> _deals;
    Collection<sint64, ESCROW_MAX_DEALS> _proposedDealsIndexes;

    struct _TradeMessage
    {
        unsigned int _contractIndex;
        unsigned int _type;
        
        sint64 numberOfShares;

        char _terminator;
    } _tradeMessage;

    struct CreateDeal_locals
    {
        _DealWithState newDealWithState;
        sint64 tempIndex;
        sint64 counter;
        AssetWithAmount tempAssetWithAmount;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateDeal)
    {
        state._counter += input.delta;

        if (qpi.invocator() == input.acceptorId)
        {
            return;
        }

        for (locals.counter = 0; locals.counter < input.offeredAssetsNumber; locals.counter++)
        {
            if (qpi.numberOfPossessedShares(input.offeredAssets.get(locals.counter).name, input.offeredAssets.get(locals.counter).issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.offeredAssets.get(locals.counter).amount)
            {
                return;
            }
        }

        locals.newDealWithState.deal.acceptorId = input.acceptorId;
        locals.newDealWithState.deal.offeredAssets = input.offeredAssets;
        locals.newDealWithState.deal.offeredAssetsNumber = input.offeredAssetsNumber;
        locals.newDealWithState.deal.requestedAssets = input.requestedAssets;
        locals.newDealWithState.deal.requestedAssetsNubmer = input.requestedAssetsNubmer;
        locals.newDealWithState.isActive = true;
        locals.tempIndex = state._deals.add(qpi.invocator(), locals.newDealWithState, 0);
        state._proposedDealsIndexes.add(input.acceptorId, locals.tempIndex, 0);
    }

    struct GetDeals_locals
    {
        sint64 elementIndex;
        sint64 elementIndex2;
        GetDeals_output::_DealEntity tempDeal;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetDeals)
    {
        output.currentValue = state._counter;
        output.ownedDealsAmount = state._deals.population(input.owner);
        output.proposedDealsAmount = state._proposedDealsIndexes.population(input.owner);

        locals.elementIndex = state._deals.headIndex(input.owner);
        locals.elementIndex2 = 0;
        while (locals.elementIndex != NULL_INDEX
            && locals.elementIndex2 < ESCROW_MAX_DEALS_PER_USER)
        {
            locals.tempDeal.index = locals.elementIndex;
            if (!state._deals.element(locals.elementIndex).isActive)
            {
                locals.elementIndex = state._deals.nextElementIndex(locals.elementIndex);
                continue;
            }
            locals.tempDeal.deal = state._deals.element(locals.elementIndex).deal;
            output.ownedDeals.set(locals.elementIndex2, locals.tempDeal);
            locals.elementIndex = state._deals.nextElementIndex(locals.elementIndex);
            locals.elementIndex2++;
        }

        locals.elementIndex = state._proposedDealsIndexes.headIndex(input.owner);
        locals.elementIndex2 = 0;
        while (locals.elementIndex != NULL_INDEX
            && locals.elementIndex2 < ESCROW_MAX_DEALS_PER_USER)
        {
            locals.tempDeal.index = locals.elementIndex;
            if (!state._deals.element(state._proposedDealsIndexes.element(locals.elementIndex)).isActive)
            {
                locals.elementIndex = state._proposedDealsIndexes.nextElementIndex(locals.elementIndex);
                continue;
            }
            locals.tempDeal.deal = state._deals.element(state._proposedDealsIndexes.element(locals.elementIndex)).deal;
            output.proposedDeals.set(locals.elementIndex2, locals.tempDeal);
            locals.elementIndex = state._proposedDealsIndexes.nextElementIndex(locals.elementIndex);
            locals.elementIndex2++;
        }
    }

    struct AcceptDeal_locals
    {
        _DealWithState tempDeal;
        sint64 counter;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AcceptDeal)
    {
        locals.tempDeal = state._deals.element(input.index);
        if (locals.tempDeal.deal.acceptorId != qpi.invocator() || !locals.tempDeal.isActive)
        {
            state._tradeMessage.numberOfShares = 8;
            LOG_INFO(state._tradeMessage);
            return;
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.deal.requestedAssetsNubmer; locals.counter++)
        {
            if (qpi.numberOfPossessedShares(locals.tempDeal.deal.requestedAssets.get(locals.counter).name, locals.tempDeal.deal.requestedAssets.get(locals.counter).issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < locals.tempDeal.deal.requestedAssets.get(locals.counter).amount)
            {
                return;
            }
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.deal.offeredAssetsNumber; locals.counter++)
        {
            qpi.transferShareOwnershipAndPossession(
                locals.tempDeal.deal.offeredAssets.get(locals.counter).name,
                locals.tempDeal.deal.offeredAssets.get(locals.counter).issuer,
                state._deals.pov(input.index),
                state._deals.pov(input.index),
                locals.tempDeal.deal.offeredAssets.get(locals.counter).amount,
                qpi.invocator());
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.deal.requestedAssetsNubmer; locals.counter++)
        {
            qpi.transferShareOwnershipAndPossession(
                locals.tempDeal.deal.requestedAssets.get(locals.counter).name,
                locals.tempDeal.deal.requestedAssets.get(locals.counter).issuer,
                qpi.invocator(),
                qpi.invocator(),
                locals.tempDeal.deal.requestedAssets.get(locals.counter).amount,
                state._deals.pov(input.index));
        }
        locals.tempDeal.isActive = false;
        state._deals.replace(input.index, locals.tempDeal);
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(CreateDeal, 1);
        REGISTER_USER_FUNCTION(GetDeals, 2);
        REGISTER_USER_PROCEDURE(AcceptDeal, 3);
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
        state._deals.reset();
        state._proposedDealsIndexes.reset();
    }
};
