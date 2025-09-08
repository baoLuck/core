using namespace QPI;

constexpr uint64 ESCROW_INITIAL_MAX_DEALS = 1048576ULL;
constexpr uint64 ESCROW_MAX_DEALS = ESCROW_INITIAL_MAX_DEALS * X_MULTIPLIER;
constexpr uint64 ESCROW_MAX_DEALS_PER_USER = 8;
constexpr uint64 ESCROW_MAX_ASSETS_IN_DEAL = 4;
constexpr uint64 ESCROW_MAX_RESERVED_ASSETS = 4194304ULL;
constexpr uint64 ESCROW_DEAL_EXISTENCE_EPOCH_COUNT = 2;

constexpr uint64 ESCROW_BASE_FEE = 250000ULL;
constexpr uint64 ESCROW_ADDITIONAL_FEE_PERCENT = 200; // 2%
constexpr uint64 ESCROW_FEE_PER_SHARE = 1500000ULL;

constexpr uint64 ESCROW_SHAREHOLDERS_DISTRIBUTION_PERCENT = 9500; // 95%

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
        sint64 index;
        id acceptorId;
        uint64 offeredQU;
        uint64 offeredAssetsNumber;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> offeredAssets;
        uint64 requestedQU;
        uint64 requestedAssetsNumber;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> requestedAssets;
        uint16 creationEpoch;
    };

    struct CreateDeal_input
    {
        id acceptorId;
        uint64 offeredQU;
        uint64 offeredAssetsNumber;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> offeredAssets;
        uint64 requestedQU;
        uint64 requestedAssetsNumber;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> requestedAssets;
    };
    struct CreateDeal_output
    {
    };

    struct AcceptDeal_input
    {
        sint64 index;
    };
    struct AcceptDeal_output
    {
    };

    struct MakeDealPublic_input
    {
        sint64 index;
    };
    struct MakeDealPublic_output
    {
    };

    struct CancelDeal_input
    {
        sint64 index;
    };
    struct CancelDeal_output
    {
    };

    struct TransferShareManagementRights_input
    {
        Asset asset;
        sint64 amount;
    };
    struct TransferShareManagementRights_output
    {
        sint64 transferredShares;
    };

    struct GetDeals_input
    {
        id owner;
    };
    struct GetDeals_output
    {
        sint64 counter;
        uint64 ownedDealsAmount;
        uint64 proposedDealsAmount;
        uint64 publicDealsAmount;
        Array<Deal, ESCROW_MAX_DEALS_PER_USER> ownedDeals;
        Array<Deal, 32> proposedDeals;
        Array<Deal, 64> publicDeals;
    };

    struct GetFreeAssetAmount_input
    {
        id owner;
        id issuer;
        uint64 assetName;
    };
    struct GetFreeAssetAmount_output
    {
        uint64 freeAmount;
    };

    private:
    uint64 _earnedAmount;
    uint64 _distributedAmount;
    sint64 _currentDealIndex;
    HashMap<Asset, uint64, ESCROW_MAX_RESERVED_ASSETS> _earnedTokens;
    HashMap<Asset, uint64, ESCROW_MAX_RESERVED_ASSETS> _distributedTokens;

    Collection<Deal, ESCROW_MAX_DEALS> _deals;
    Collection<Deal, ESCROW_MAX_DEALS> _dealsCopy;
    Collection<AssetWithAmount, ESCROW_MAX_RESERVED_ASSETS> _reservedAssets;

    sint64 _counter;

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
        uint64 counter;
        sint64 elementIndex;
        sint64 offeredQuAndFee;
        AssetWithAmount tempAssetWithAmount;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateDeal)
    {
        if (state._deals.population() >= ESCROW_MAX_DEALS
                || state._deals.population(qpi.invocator()) >= ESCROW_MAX_DEALS_PER_USER
                || (input.offeredAssetsNumber == 0 && input.requestedAssetsNumber == 0))
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        locals.offeredQuAndFee = input.offeredQU + ESCROW_BASE_FEE;

        for (locals.counter = 0; locals.counter < input.offeredAssetsNumber; locals.counter++)
        {
            state._numberOfReservedShares_input.issuer = input.offeredAssets.get(locals.counter).issuer;
            state._numberOfReservedShares_input.assetName = input.offeredAssets.get(locals.counter).name;
            state._numberOfReservedShares_input.owner = qpi.invocator();
            CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
            if (qpi.numberOfPossessedShares(input.offeredAssets.get(locals.counter).name, input.offeredAssets.get(locals.counter).issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedShares_output.amount 
                    < input.offeredAssets.get(locals.counter).amount)
            {
                if (qpi.invocationReward() > 0)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                return;
            }

            if (input.offeredAssets.get(locals.counter).issuer == NULL_ID)
            {
                locals.offeredQuAndFee += (input.offeredAssets.get(locals.counter).amount * ESCROW_FEE_PER_SHARE);
            }
        }

        for (locals.counter = 0; locals.counter < input.requestedAssetsNumber; locals.counter++)
        {
            if (input.requestedAssets.get(locals.counter).issuer == NULL_ID)
            {
                locals.offeredQuAndFee += (input.requestedAssets.get(locals.counter).amount * ESCROW_FEE_PER_SHARE);
            }
        }

        if (qpi.invocationReward() != locals.offeredQuAndFee)
        {
            if (qpi.invocationReward() > locals.offeredQuAndFee)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.offeredQuAndFee);
            }
            else
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }
        }

        locals.newDeal.index = state._currentDealIndex;
        state._currentDealIndex++;
        locals.newDeal.acceptorId = (input.acceptorId == NULL_ID || input.acceptorId == qpi.invocator()) ? SELF : input.acceptorId;
        locals.newDeal.offeredQU = input.offeredQU;
        locals.newDeal.offeredAssets = input.offeredAssets;
        locals.newDeal.offeredAssetsNumber = input.offeredAssetsNumber;
        locals.newDeal.requestedQU = input.requestedQU;
        locals.newDeal.requestedAssets = input.requestedAssets;
        locals.newDeal.requestedAssetsNumber = input.requestedAssetsNumber;
        locals.newDeal.creationEpoch = qpi.epoch();

        state._deals.add(qpi.invocator(), locals.newDeal, 0);
        state._earnedAmount += ESCROW_BASE_FEE;

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
                        break;
                    }

                    locals.elementIndex = state._reservedAssets.nextElementIndex(locals.elementIndex);
                }
            }
        } 
    }

    struct AcceptDeal_locals
    {
        Deal tempDeal;
        sint64 dealIndexInCollection;
        uint64 counter;
        sint64 transferredShares;
        sint64 transferredFeeShares;
        sint64 elementIndex;
        sint64 elementIndex2;
        sint64 requestedQuAndFee;
        AssetWithAmount tempAssetWithAmount;
        Asset tempAsset;
        uint64 tempAmount;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(AcceptDeal)
    {
        for (locals.dealIndexInCollection = 0; locals.dealIndexInCollection < ESCROW_MAX_DEALS; locals.dealIndexInCollection++)
        {
            if (state._deals.element(locals.dealIndexInCollection).index == input.index)
            {
                locals.tempDeal = state._deals.element(locals.dealIndexInCollection);
                break;
            }
        }
        
        if (locals.dealIndexInCollection >= ESCROW_MAX_DEALS || (locals.tempDeal.acceptorId != SELF && locals.tempDeal.acceptorId != qpi.invocator()))
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        locals.requestedQuAndFee = locals.tempDeal.requestedQU + ESCROW_BASE_FEE;

        for (locals.counter = 0; locals.counter < locals.tempDeal.requestedAssetsNumber; locals.counter++)
        {
            state._numberOfReservedShares_input.issuer = locals.tempDeal.requestedAssets.get(locals.counter).issuer;
            state._numberOfReservedShares_input.assetName = locals.tempDeal.requestedAssets.get(locals.counter).name;
            state._numberOfReservedShares_input.owner = qpi.invocator();
            CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
            if (qpi.numberOfPossessedShares(locals.tempDeal.requestedAssets.get(locals.counter).name, locals.tempDeal.requestedAssets.get(locals.counter).issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedShares_output.amount
                    < locals.tempDeal.requestedAssets.get(locals.counter).amount)
            {
                if (qpi.invocationReward() > 0)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                return;
            }

            if (locals.tempDeal.requestedAssets.get(locals.counter).issuer == NULL_ID)
            {
                locals.requestedQuAndFee += (locals.tempDeal.requestedAssets.get(locals.counter).amount * ESCROW_FEE_PER_SHARE);
            }
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.offeredAssetsNumber; locals.counter++)
        {
            if (locals.tempDeal.offeredAssets.get(locals.counter).issuer == NULL_ID)
            {
                locals.requestedQuAndFee += (locals.tempDeal.offeredAssets.get(locals.counter).amount * ESCROW_FEE_PER_SHARE);
            }
        }

        if (qpi.invocationReward() != locals.requestedQuAndFee)
        {
            if (qpi.invocationReward() > locals.requestedQuAndFee)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.requestedQuAndFee);
            }
            else
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
                return;
            }
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.offeredAssetsNumber; locals.counter++)
        {
            if (locals.tempDeal.offeredAssets.get(locals.counter).issuer == NULL_ID)
            {
                locals.transferredFeeShares = 0;
                locals.transferredShares = locals.tempDeal.offeredAssets.get(locals.counter).amount;
                qpi.transferShareOwnershipAndPossession(
                        locals.tempDeal.offeredAssets.get(locals.counter).name,
                        locals.tempDeal.offeredAssets.get(locals.counter).issuer,
                        state._deals.pov(locals.dealIndexInCollection),
                        state._deals.pov(locals.dealIndexInCollection),
                        locals.tempDeal.offeredAssets.get(locals.counter).amount,
                        qpi.invocator());
            }
            else
            {
                locals.transferredShares = locals.tempDeal.offeredAssets.get(locals.counter).amount - QPI::div(locals.tempDeal.offeredAssets.get(locals.counter).amount * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL);
                qpi.transferShareOwnershipAndPossession(
                        locals.tempDeal.offeredAssets.get(locals.counter).name,
                        locals.tempDeal.offeredAssets.get(locals.counter).issuer,
                        state._deals.pov(locals.dealIndexInCollection),
                        state._deals.pov(locals.dealIndexInCollection),
                        locals.transferredShares,
                        qpi.invocator());

                locals.transferredFeeShares = QPI::div(locals.tempDeal.offeredAssets.get(locals.counter).amount * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL);
                qpi.transferShareOwnershipAndPossession(
                        locals.tempDeal.offeredAssets.get(locals.counter).name,
                        locals.tempDeal.offeredAssets.get(locals.counter).issuer,
                        state._deals.pov(locals.dealIndexInCollection),
                        state._deals.pov(locals.dealIndexInCollection),
                        QPI::div(locals.tempDeal.offeredAssets.get(locals.counter).amount * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL),
                        SELF);
            }

            locals.tempAsset.issuer = locals.tempDeal.offeredAssets.get(locals.counter).issuer;
            locals.tempAsset.assetName = locals.tempDeal.offeredAssets.get(locals.counter).name;
            locals.elementIndex2 = state._earnedTokens.getElementIndex(locals.tempAsset);

            if (locals.tempAsset.issuer != NULL_ID)
            {
                if (locals.elementIndex2 == NULL_INDEX)
                {
                    state._earnedTokens.set(locals.tempAsset, QPI::div(locals.tempDeal.offeredAssets.get(locals.counter).amount * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL));
                }
                else
                {
                    locals.tempAmount = state._earnedTokens.value(locals.elementIndex2);
                    locals.tempAmount += QPI::div(locals.tempDeal.offeredAssets.get(locals.counter).amount * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL);
                    state._earnedTokens.replace(locals.tempAsset, locals.tempAmount);
                }
            }

            state._numberOfReservedShares_input.issuer = locals.tempDeal.offeredAssets.get(locals.counter).issuer;
            state._numberOfReservedShares_input.assetName = locals.tempDeal.offeredAssets.get(locals.counter).name;
            state._numberOfReservedShares_input.owner = state._deals.pov(locals.dealIndexInCollection);
            CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
            locals.elementIndex = state._reservedAssets.headIndex(state._deals.pov(locals.dealIndexInCollection));
            while (locals.elementIndex != NULL_INDEX)
            {
                locals.tempAssetWithAmount = state._reservedAssets.element(locals.elementIndex);
                if (locals.tempAssetWithAmount.name == locals.tempDeal.offeredAssets.get(locals.counter).name
                    && locals.tempAssetWithAmount.issuer == locals.tempDeal.offeredAssets.get(locals.counter).issuer)
                {
                    if (state._numberOfReservedShares_output.amount - locals.transferredShares - locals.transferredFeeShares <= 0)
                    {
                        state._reservedAssets.remove(locals.elementIndex);
                        break;
                    }
                    else
                    {
                        locals.tempAssetWithAmount.amount -= locals.transferredShares;
                        locals.tempAssetWithAmount.amount -= locals.transferredFeeShares;
                        state._reservedAssets.replace(locals.elementIndex, locals.tempAssetWithAmount);
                    }
                }
                locals.elementIndex = state._reservedAssets.nextElementIndex(locals.elementIndex);
            }
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.requestedAssetsNumber; locals.counter++)
        {
            if (locals.tempDeal.requestedAssets.get(locals.counter).issuer == NULL_ID)
            {
                qpi.transferShareOwnershipAndPossession(
                    locals.tempDeal.requestedAssets.get(locals.counter).name,
                    locals.tempDeal.requestedAssets.get(locals.counter).issuer,
                    qpi.invocator(),
                    qpi.invocator(),
                    locals.tempDeal.requestedAssets.get(locals.counter).amount,
                    state._deals.pov(locals.dealIndexInCollection));
            }
            else
            {
                qpi.transferShareOwnershipAndPossession(
                    locals.tempDeal.requestedAssets.get(locals.counter).name,
                    locals.tempDeal.requestedAssets.get(locals.counter).issuer,
                    qpi.invocator(),
                    qpi.invocator(),
                    locals.tempDeal.requestedAssets.get(locals.counter).amount - QPI::div(locals.tempDeal.requestedAssets.get(locals.counter).amount * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL),
                    state._deals.pov(locals.dealIndexInCollection));

                qpi.transferShareOwnershipAndPossession(
                    locals.tempDeal.requestedAssets.get(locals.counter).name,
                    locals.tempDeal.requestedAssets.get(locals.counter).issuer,
                    qpi.invocator(),
                    qpi.invocator(),
                    QPI::div(locals.tempDeal.requestedAssets.get(locals.counter).amount * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL),
                    SELF);
            }

            locals.tempAsset.issuer = locals.tempDeal.requestedAssets.get(locals.counter).issuer;
            locals.tempAsset.assetName = locals.tempDeal.requestedAssets.get(locals.counter).name;
            locals.elementIndex2 = state._earnedTokens.getElementIndex(locals.tempAsset);

            if (locals.tempAsset.issuer != NULL_ID)
            {
                if (locals.elementIndex2 == NULL_INDEX)
                {
                    state._earnedTokens.set(locals.tempAsset, QPI::div(locals.tempDeal.requestedAssets.get(locals.counter).amount * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL));
                }
                else
                {
                    locals.tempAmount = state._earnedTokens.value(locals.elementIndex2);
                    locals.tempAmount += QPI::div(locals.tempDeal.requestedAssets.get(locals.counter).amount * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL);
                    state._earnedTokens.replace(locals.tempAsset, locals.tempAmount);
                }
            }
        }

        qpi.transfer(qpi.invocator(), locals.tempDeal.offeredQU - QPI::div(locals.tempDeal.offeredQU * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL));
        qpi.transfer(state._deals.pov(locals.dealIndexInCollection), locals.tempDeal.requestedQU - QPI::div(locals.tempDeal.requestedQU * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL));

        state._deals.remove(locals.dealIndexInCollection);

        state._earnedAmount += ESCROW_BASE_FEE;
        state._earnedAmount += QPI::div(locals.tempDeal.offeredQU * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL);
        state._earnedAmount += QPI::div(locals.tempDeal.requestedQU * ESCROW_ADDITIONAL_FEE_PERCENT, 10000ULL);
        state._earnedAmount += ((locals.requestedQuAndFee - locals.tempDeal.requestedQU - ESCROW_BASE_FEE) * 2);
    }

    struct MakeDealPublic_locals
    {
        Deal tempDeal;
        sint64 dealIndexInCollection;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(MakeDealPublic)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        for (locals.dealIndexInCollection = 0; locals.dealIndexInCollection < ESCROW_MAX_DEALS; locals.dealIndexInCollection++)
        {
            if (state._deals.element(locals.dealIndexInCollection).index == input.index)
            {
                locals.tempDeal = state._deals.element(locals.dealIndexInCollection);
                break;
            }
        }

        if (locals.dealIndexInCollection >= ESCROW_MAX_DEALS || state._deals.pov(locals.dealIndexInCollection) != qpi.invocator() || locals.tempDeal.acceptorId == SELF)
        {
            return;
        }

        locals.tempDeal.acceptorId = SELF;
        state._deals.replace(locals.dealIndexInCollection, locals.tempDeal);
    }

    struct CancelDeal_locals
    {
        Deal tempDeal;
        uint64 counter;
        sint64 dealIndexInCollection;
        sint64 elementIndex;
        AssetWithAmount tempAssetWithAmount;
        uint64 quForReturn;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CancelDeal)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }

        for (locals.dealIndexInCollection = 0; locals.dealIndexInCollection < ESCROW_MAX_DEALS; locals.dealIndexInCollection++)
        {
            if (state._deals.element(locals.dealIndexInCollection).index == input.index)
            {
                locals.tempDeal = state._deals.element(locals.dealIndexInCollection);
                break;
            }
        }

        if (locals.dealIndexInCollection >= ESCROW_MAX_DEALS || state._deals.pov(locals.dealIndexInCollection) != qpi.invocator())
        {
            return;
        }

        locals.quForReturn = locals.tempDeal.offeredQU;

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

            if (locals.tempDeal.offeredAssets.get(locals.counter).issuer == NULL_ID)
            {
                locals.quForReturn += (locals.tempDeal.offeredAssets.get(locals.counter).amount * ESCROW_FEE_PER_SHARE);
            }
        }

        for (locals.counter = 0; locals.counter < locals.tempDeal.requestedAssetsNumber; locals.counter++)
        {
            if (locals.tempDeal.requestedAssets.get(locals.counter).issuer == NULL_ID)
            {
                locals.quForReturn += (locals.tempDeal.requestedAssets.get(locals.counter).amount * ESCROW_FEE_PER_SHARE);
            }
        }

        qpi.transfer(qpi.invocator(), locals.quForReturn);

        state._deals.remove(locals.dealIndexInCollection);
    }

    PUBLIC_PROCEDURE(TransferShareManagementRights)
    {
        state._counter = 1;
        // if (qpi.invocationReward() > 0)
        // {
        //     qpi.transfer(qpi.invocator(), qpi.invocationReward());
        // }

        state._counter = 2;
        state._numberOfReservedShares_input.issuer = input.asset.issuer;
        state._numberOfReservedShares_input.assetName = input.asset.assetName;
        state._numberOfReservedShares_input.owner = qpi.invocator();
        CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
        if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) - state._numberOfReservedShares_output.amount < input.amount)
        {
            state._counter = qpi.numberOfShares(input.asset, {qpi.invocator(), 3, false, false}, {qpi.invocator(), 3, false, false});
            output.transferredShares = 0;
        }
        else
        {
            state._counter = 4;
            if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.amount, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX, 1000000) < 0)
            {
                state._counter = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.amount, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX, 1000000);
                output.transferredShares = 0;
            }
            else
            {
                state._counter = 6;
                output.transferredShares = input.amount;
            }
        }
    }

    struct GetDeals_locals
    {
        sint64 elementIndex;
        sint64 elementIndex2;
        sint64 elementIndex3;
        Deal tempDeal;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetDeals)
    {
        output.counter = state._counter;
        output.ownedDealsAmount = state._deals.population(input.owner);

        locals.elementIndex = state._deals.headIndex(input.owner);
        locals.elementIndex2 = 0;
        while (locals.elementIndex != NULL_INDEX
            && locals.elementIndex2 < ESCROW_MAX_DEALS_PER_USER)
        {
            locals.tempDeal = state._deals.element(locals.elementIndex);
            output.ownedDeals.set(locals.elementIndex2, locals.tempDeal);
            locals.elementIndex = state._deals.nextElementIndex(locals.elementIndex);
            locals.elementIndex2++;
        }

        locals.elementIndex2 = 0;
        locals.elementIndex3 = 0;
        for (locals.elementIndex = 0; locals.elementIndex < ESCROW_MAX_DEALS; locals.elementIndex++)
        {
            locals.tempDeal = state._deals.element(locals.elementIndex);
            if (locals.tempDeal.acceptorId == input.owner && locals.elementIndex2 < 32)
            {
                locals.tempDeal.acceptorId = state._deals.pov(locals.elementIndex);
                output.proposedDeals.set(locals.elementIndex2, locals.tempDeal);
                locals.elementIndex2++;
            }
            if (locals.tempDeal.acceptorId == SELF
                && locals.elementIndex3 < 64
                && state._deals.pov(locals.elementIndex) != input.owner)
            {
                    locals.tempDeal.acceptorId = state._deals.pov(locals.elementIndex);
                    output.publicDeals.set(locals.elementIndex3, locals.tempDeal);
                    locals.elementIndex3++;
            }
        }
        output.proposedDealsAmount = locals.elementIndex2;
        output.publicDealsAmount = locals.elementIndex3;
    }

    struct GetFreeAssetAmount_locals
    {
        _NumberOfReservedShares_input reservedInput;
        _NumberOfReservedShares_output reservedOutput;
    };

    PUBLIC_FUNCTION_WITH_LOCALS(GetFreeAssetAmount)
    {
        // locals.reservedInput.issuer = input.issuer;
        // locals.reservedInput.assetName = input.assetName;
        // locals.reservedInput.owner = input.owner;
        // CALL(_NumberOfReservedShares, locals.reservedInput, locals.reservedOutput);
        output.freeAmount = qpi.numberOfPossessedShares(input.assetName, input.issuer, input.owner, input.owner, SELF_INDEX, SELF_INDEX) + 5;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(CreateDeal, 1);
        REGISTER_USER_PROCEDURE(AcceptDeal, 2);
        REGISTER_USER_PROCEDURE(MakeDealPublic, 3);
        REGISTER_USER_PROCEDURE(CancelDeal, 4);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 5);

        REGISTER_USER_FUNCTION(GetDeals, 1);
        REGISTER_USER_FUNCTION(GetFreeAssetAmount, 2);
    }

    INITIALIZE()
    {
        state._currentDealIndex = 1;
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    struct BEGIN_EPOCH_locals
    {
        Deal tempDeal;
        Deal tempDeal2;
        uint64 counter;
        sint64 dealIndexInCollection;
        sint64 elementIndex;
        AssetWithAmount tempAssetWithAmount;
        uint64 quForReturn;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        state._dealsCopy = state._deals;
        for (locals.dealIndexInCollection = 0; locals.dealIndexInCollection < ESCROW_MAX_DEALS; locals.dealIndexInCollection++)
        {
            if (state._dealsCopy.pov(locals.dealIndexInCollection) == NULL_ID || state._dealsCopy.element(locals.dealIndexInCollection).creationEpoch + ESCROW_DEAL_EXISTENCE_EPOCH_COUNT > qpi.epoch())
            {
                continue;
            }
            
            locals.tempDeal = state._dealsCopy.element(locals.dealIndexInCollection);
            locals.quForReturn = locals.tempDeal.offeredQU;
            for (locals.counter = 0; locals.counter < locals.tempDeal.offeredAssetsNumber; locals.counter++)
            {
                state._numberOfReservedShares_input.issuer = locals.tempDeal.offeredAssets.get(locals.counter).issuer;
                state._numberOfReservedShares_input.assetName = locals.tempDeal.offeredAssets.get(locals.counter).name;
                state._numberOfReservedShares_input.owner = state._dealsCopy.pov(locals.dealIndexInCollection);
                CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
                locals.elementIndex = state._reservedAssets.headIndex(state._dealsCopy.pov(locals.dealIndexInCollection));
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

                if (locals.tempDeal.offeredAssets.get(locals.counter).issuer == NULL_ID)
                {
                    locals.quForReturn += (locals.tempDeal.offeredAssets.get(locals.counter).amount * ESCROW_FEE_PER_SHARE);
                }
            }

            for (locals.counter = 0; locals.counter < locals.tempDeal.requestedAssetsNumber; locals.counter++)
            {
                if (locals.tempDeal.requestedAssets.get(locals.counter).issuer == NULL_ID)
                {
                    locals.quForReturn += (locals.tempDeal.requestedAssets.get(locals.counter).amount * ESCROW_FEE_PER_SHARE);
                }
            }

            if (locals.quForReturn > 0)
            {
                qpi.transfer(state._dealsCopy.pov(locals.dealIndexInCollection), locals.quForReturn);
            }

            locals.elementIndex = state._deals.headIndex(state._dealsCopy.pov(locals.dealIndexInCollection));
            while (locals.elementIndex != NULL_INDEX)
            {
                locals.tempDeal2 = state._deals.element(locals.elementIndex);
                if (locals.tempDeal.index == locals.tempDeal2.index)
                {
                    state._deals.remove(locals.elementIndex);
                    break;
                }
                locals.elementIndex = state._deals.nextElementIndex(locals.elementIndex);
            } 
        }
    }

    struct END_EPOCH_locals
    {
        uint64 amountToDistribute;
        uint64 amountToBurn;
        AssetOwnershipIterator assetIt;
        Asset selfShare;
        sint64 counter;
        Asset tempAsset;
        uint64 tempEarnedAmount;
        uint64 tempDistributedAmount;
        uint64 tokenAmountToDistribute;
        uint64 transferredShares;
    };

    END_EPOCH_WITH_LOCALS()
    {
        locals.amountToDistribute = QPI::div((state._earnedAmount - state._distributedAmount) * ESCROW_SHAREHOLDERS_DISTRIBUTION_PERCENT, 10000ULL);
        locals.amountToBurn = state._earnedAmount - state._distributedAmount - locals.amountToDistribute;

        if ((QPI::div(locals.amountToDistribute, 676ULL) > 0) && (state._earnedAmount > state._distributedAmount))
        {
            if (qpi.distributeDividends(QPI::div(locals.amountToDistribute, 676ULL)))
            {
                state._distributedAmount += QPI::div(locals.amountToDistribute, 676ULL) * NUMBER_OF_COMPUTORS;
                qpi.burn(locals.amountToBurn);
                state._distributedAmount += locals.amountToBurn;
            }
        }

        locals.selfShare.issuer = NULL_ID;
        locals.selfShare.assetName = 85002843734354ULL;

        for (locals.counter = 0; locals.counter < ESCROW_MAX_RESERVED_ASSETS; locals.counter++)
        {
            if (state._earnedTokens.isEmptySlot(locals.counter))
            {
                continue;
            }

            locals.tempAsset = state._earnedTokens.key(locals.counter);
            locals.tempEarnedAmount = state._earnedTokens.value(locals.counter);
            locals.tokenAmountToDistribute = locals.tempEarnedAmount;

            if (state._distributedTokens.get(locals.tempAsset, locals.tempDistributedAmount))
            {
                locals.tokenAmountToDistribute -= locals.tempDistributedAmount;
            }

            if (locals.tokenAmountToDistribute < 676ULL)
            {
                continue;
            }

            locals.transferredShares = 0;
            locals.assetIt.begin(locals.selfShare);
            while (!locals.assetIt.reachedEnd())
            {
                locals.transferredShares += QPI::div(locals.tokenAmountToDistribute, 676ULL) * locals.assetIt.numberOfOwnedShares();
                qpi.transferShareOwnershipAndPossession(
                        locals.tempAsset.assetName,
                        locals.tempAsset.issuer,
                        SELF,
                        SELF,
                        QPI::div(locals.tokenAmountToDistribute, 676ULL) * locals.assetIt.numberOfOwnedShares(),
                        locals.assetIt.owner());
                locals.assetIt.next();
            }

            if (state._distributedTokens.get(locals.tempAsset, locals.tempDistributedAmount))
            {
                locals.tempDistributedAmount += locals.transferredShares;
                state._distributedTokens.replace(locals.tempAsset, locals.tempDistributedAmount);
            }
            else
            {
                state._distributedTokens.set(locals.tempAsset, locals.transferredShares);
            }
        }
    }
};
