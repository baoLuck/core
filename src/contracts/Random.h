using namespace QPI;

constexpr uint64 ESCROW_INITIAL_MAX_DEALS = 524288ULL;
constexpr uint64 ESCROW_MAX_DEALS = ESCROW_INITIAL_MAX_DEALS * X_MULTIPLIER;
constexpr uint64 ESCROW_MAX_DEALS_PER_USER = 8;
constexpr uint64 ESCROW_MAX_ASSETS_IN_DEAL = 4;
constexpr uint64 ESCROW_MAX_RESERVED_ASSETS = 2097152ULL;
constexpr uint64 ESCROW_DEAL_EXISTENCE_EPOCH_COUNT = 2;

constexpr uint64 ESCROW_BASE_FEE = 250000ULL;
constexpr uint64 ESCROW_ADDITIONAL_FEE_PERCENT = 200; // 2%
constexpr uint64 ESCROW_FEE_PER_SHARE = 1500000ULL;

constexpr uint64 ESCROW_SHAREHOLDERS_TOKEN_DISTRIBUTION_PERCENT = 9500; // 95%
constexpr uint64 ESCROW_SHAREHOLDERS_QU_DISTRIBUTION_PERCENT = 9300; // 93%
constexpr uint64 ESCROW_BURN_QU_PERCENT = 200; // 2%

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
        uint32 newContractIndex;
    };
    struct TransferShareManagementRights_output
    {
        sint64 transferredShares;
    };

    struct GetDeals_input
    {
        id owner;
        sint64 proposedDealsOffset;
        sint64 publicDealsOffset;
    };
    struct GetDeals_output
    {
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
        Asset asset;
    };
    struct GetFreeAssetAmount_output
    {
        uint64 freeAmount;
    };

private:
    uint64 _earnedAmount;
    uint64 _distributedAmount;
    HashMap<Asset, uint64, ESCROW_MAX_RESERVED_ASSETS> _earnedTokens;
    HashMap<Asset, uint64, ESCROW_MAX_RESERVED_ASSETS> _distributedTokens;

    sint64 _currentDealIndex;
    Collection<Deal, ESCROW_MAX_DEALS> _deals;
    struct _EntryForRemove
    {
        id owner;
        sint64 index;
    };
    Array<_EntryForRemove, 524288> _entriesForRemove;
    Collection<AssetWithAmount, ESCROW_MAX_RESERVED_ASSETS> _reservedAssets;

    id _devAddress;

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

        locals.elementIndex = state._reservedAssets.headIndex(input.owner);
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
        Asset tempAsset;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateDeal)
    {
        if (state._deals.population() >= ESCROW_MAX_DEALS
                || state._deals.population(qpi.invocator()) >= ESCROW_MAX_DEALS_PER_USER
                || (input.offeredAssetsNumber == 0 && input.requestedAssetsNumber == 0)
                || input.offeredQU >= MAX_AMOUNT
                || input.requestedQU >= MAX_AMOUNT)
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
            locals.tempAsset.assetName = input.offeredAssets.get(locals.counter).name;
            locals.tempAsset.issuer = input.offeredAssets.get(locals.counter).issuer;
            if (input.offeredAssets.get(locals.counter).amount >= MAX_AMOUNT
                    || qpi.numberOfShares(locals.tempAsset, {qpi.invocator(), SELF_INDEX, false, false}, {qpi.invocator(), SELF_INDEX, false, false}) - state._numberOfReservedShares_output.amount 
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
            if (input.requestedAssets.get(locals.counter).amount >= MAX_AMOUNT)
            {
                if (qpi.invocationReward() > 0)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward());
                }
                return;
            }
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
            locals.tempAsset.assetName = locals.tempDeal.requestedAssets.get(locals.counter).name;
            locals.tempAsset.issuer = locals.tempDeal.requestedAssets.get(locals.counter).issuer;
            if (qpi.numberOfShares(locals.tempAsset, {qpi.invocator(), SELF_INDEX, false, false}, {qpi.invocator(), SELF_INDEX, false, false}) - state._numberOfReservedShares_output.amount
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
            if (state._deals.pov(locals.dealIndexInCollection) != NULL_ID && state._deals.element(locals.dealIndexInCollection).index == input.index)
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

    struct TransferShareManagementRights_locals
    {
        QX::Fees_input feesInput;
        QX::Fees_output feesOutput;
    };

    PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
    {
        output.transferredShares = 0;
        CALL_OTHER_CONTRACT_FUNCTION(QX, Fees, locals.feesInput, locals.feesOutput);

        if (qpi.invocationReward() < locals.feesOutput.transferFee
                || input.amount >= MAX_AMOUNT)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        if (qpi.invocationReward() > locals.feesOutput.transferFee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.feesOutput.transferFee);
        }

        state._numberOfReservedShares_input.issuer = input.asset.issuer;
        state._numberOfReservedShares_input.assetName = input.asset.assetName;
        state._numberOfReservedShares_input.owner = qpi.invocator();
        CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
        if (qpi.numberOfShares(input.asset, {qpi.invocator(), SELF_INDEX, false, false}, {qpi.invocator(), SELF_INDEX, false, false}) - state._numberOfReservedShares_output.amount < input.amount)
        {
            output.transferredShares = 0;
        }
        else
        {
            if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.amount, input.newContractIndex, input.newContractIndex, locals.feesOutput.transferFee) < 0)
            {
                output.transferredShares = 0;
            }
            else
            {
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
                if (input.proposedDealsOffset > 0)
                {
                    input.proposedDealsOffset--;
                }
                else
                {
                    locals.tempDeal.acceptorId = state._deals.pov(locals.elementIndex);
                    output.proposedDeals.set(locals.elementIndex2, locals.tempDeal);
                    locals.elementIndex2++;
                }
            }

            if (locals.tempDeal.acceptorId == SELF
                && locals.elementIndex3 < 64
                && state._deals.pov(locals.elementIndex) != input.owner)
            {
                if (input.publicDealsOffset > 0)
                {
                    input.publicDealsOffset--;
                }
                else
                {
                    locals.tempDeal.acceptorId = state._deals.pov(locals.elementIndex);
                    output.publicDeals.set(locals.elementIndex3, locals.tempDeal);
                    locals.elementIndex3++;
                }   
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
        locals.reservedInput.issuer = input.asset.issuer;
        locals.reservedInput.assetName = input.asset.assetName;
        locals.reservedInput.owner = input.owner;
        CALL(_NumberOfReservedShares, locals.reservedInput, locals.reservedOutput);
        output.freeAmount = qpi.numberOfShares(input.asset, {input.owner, SELF_INDEX, false, false}, {input.owner, SELF_INDEX, false, false}) - locals.reservedOutput.amount;
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
        state._devAddress = ID(_E, _S, _C, _R, _O, _W, _F, _P, _Z, _M, _F, _P, _D, _F, _T, _M, _G, _K, _N, _N, _Z, _L, _N, _B, _U, _J, _L, _C, _W, _G, _B, _U, _L, _K, _S, _N, _W, _L, _S, _D, _R, _G, _T, _Y, _T, _B, _E, _M, _F, _O, _X, _B, _C, _A, _E, _H);
        state._currentDealIndex = 1;
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    struct BEGIN_EPOCH_locals
    {
        Deal tempDeal;
        uint64 counter;
        sint64 elementIndex;
        sint64 elementIndex2;
        AssetWithAmount tempAssetWithAmount;
        uint64 quForReturn;
        _EntryForRemove tempEntry;
        sint64 arrayIndex;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        state._devAddress = ID(_E, _S, _C, _R, _O, _W, _F, _P, _Z, _M, _F, _P, _D, _F, _T, _M, _G, _K, _N, _N, _Z, _L, _N, _B, _U, _J, _L, _C, _W, _G, _B, _U, _L, _K, _S, _N, _W, _L, _S, _D, _R, _G, _T, _Y, _T, _B, _E, _M, _F, _O, _X, _B, _C, _A, _E, _H);
        
        for (locals.elementIndex = 0; locals.elementIndex < ESCROW_MAX_DEALS; locals.elementIndex++)
        {
            if (state._deals.pov(locals.elementIndex) == NULL_ID || state._deals.element(locals.elementIndex).creationEpoch + ESCROW_DEAL_EXISTENCE_EPOCH_COUNT > qpi.epoch())
            {
                continue;
            }
            
            locals.tempDeal = state._deals.element(locals.elementIndex);
            locals.quForReturn = locals.tempDeal.offeredQU;
            for (locals.counter = 0; locals.counter < locals.tempDeal.offeredAssetsNumber; locals.counter++)
            {
                state._numberOfReservedShares_input.issuer = locals.tempDeal.offeredAssets.get(locals.counter).issuer;
                state._numberOfReservedShares_input.assetName = locals.tempDeal.offeredAssets.get(locals.counter).name;
                state._numberOfReservedShares_input.owner = state._deals.pov(locals.elementIndex);
                CALL(_NumberOfReservedShares, state._numberOfReservedShares_input, state._numberOfReservedShares_output);
                locals.elementIndex2 = state._reservedAssets.headIndex(state._deals.pov(locals.elementIndex));
                while (locals.elementIndex2 != NULL_INDEX)
                {
                    locals.tempAssetWithAmount = state._reservedAssets.element(locals.elementIndex2);
                    if (locals.tempAssetWithAmount.name == locals.tempDeal.offeredAssets.get(locals.counter).name
                        && locals.tempAssetWithAmount.issuer == locals.tempDeal.offeredAssets.get(locals.counter).issuer)
                    {
                        if (state._numberOfReservedShares_output.amount - locals.tempDeal.offeredAssets.get(locals.counter).amount <= 0)
                        {
                            state._reservedAssets.remove(locals.elementIndex2);
                            break;
                        }
                        else
                        {
                            locals.tempAssetWithAmount.amount -= locals.tempDeal.offeredAssets.get(locals.counter).amount;
                            state._reservedAssets.replace(locals.elementIndex2, locals.tempAssetWithAmount);
                        }
                    }
                    locals.elementIndex2 = state._reservedAssets.nextElementIndex(locals.elementIndex2);
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
                qpi.transfer(state._deals.pov(locals.elementIndex), locals.quForReturn);
            }

            locals.tempEntry.owner = state._deals.pov(locals.elementIndex);
            locals.tempEntry.index = locals.tempDeal.index;
            state._entriesForRemove.set(locals.arrayIndex, locals.tempEntry);
            locals.arrayIndex++;
        }

        locals.tempEntry.owner = NULL_ID;
        locals.tempEntry.index = -1;

        for (locals.elementIndex = 0; locals.elementIndex < locals.arrayIndex; locals.elementIndex++)
        {
            locals.elementIndex2 = state._deals.headIndex(state._entriesForRemove.get(locals.elementIndex).owner);
            while (locals.elementIndex2 != NULL_INDEX)
            {
                if (state._entriesForRemove.get(locals.elementIndex).index == state._deals.element(locals.elementIndex2).index)
                {
                    state._deals.remove(locals.elementIndex2);
                    break;
                }
                locals.elementIndex2 = state._deals.nextElementIndex(locals.elementIndex2);
            }
            state._entriesForRemove.set(locals.elementIndex, locals.tempEntry);
        }
    }

    struct END_EPOCH_locals
    {
        uint64 amountToDistribute;
        uint64 amountToBurn;
        uint64 amountToDevs;
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
        locals.amountToDistribute = QPI::div((state._earnedAmount - state._distributedAmount) * ESCROW_SHAREHOLDERS_QU_DISTRIBUTION_PERCENT, 10000ULL);
        locals.amountToBurn = QPI::div((state._earnedAmount - state._distributedAmount) * ESCROW_BURN_QU_PERCENT, 10000ULL);
        locals.amountToDevs = state._earnedAmount - state._distributedAmount - locals.amountToDistribute - locals.amountToBurn;

        if ((QPI::div(locals.amountToDistribute, 676ULL) > 0) && (state._earnedAmount > state._distributedAmount))
        {
            if (qpi.distributeDividends(QPI::div(locals.amountToDistribute, 676ULL)))
            {
                qpi.burn(locals.amountToBurn);
                qpi.transfer(state._devAddress, locals.amountToDevs);
                state._distributedAmount += QPI::div(locals.amountToDistribute, 676ULL) * NUMBER_OF_COMPUTORS;
                state._distributedAmount += locals.amountToBurn;
                state._distributedAmount += locals.amountToDevs;
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

            if (state._distributedTokens.get(locals.tempAsset, locals.tempDistributedAmount))
            {
                locals.tempEarnedAmount -= locals.tempDistributedAmount;
            }

            locals.tokenAmountToDistribute = QPI::div(locals.tempEarnedAmount * ESCROW_SHAREHOLDERS_TOKEN_DISTRIBUTION_PERCENT, 10000ULL);

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

            qpi.transferShareOwnershipAndPossession(
                        locals.tempAsset.assetName,
                        locals.tempAsset.issuer,
                        SELF,
                        SELF,
                        locals.tempEarnedAmount - locals.tokenAmountToDistribute,
                        state._devAddress);
            locals.transferredShares += (locals.tempEarnedAmount - locals.tokenAmountToDistribute);

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
