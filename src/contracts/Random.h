using namespace QPI;

constexpr uint64 ESCROW_INITIAL_MAX_DEALS = 128;                           // 131072ULL; // 2^17
constexpr uint64 ESCROW_MAX_DEALS = ESCROW_INITIAL_MAX_DEALS * X_MULTIPLIER;
constexpr uint64 ESCROW_MAX_DEALS_PER_USER = 32;                           // 1024ULL * X_MULTIPLIER;
constexpr uint64 ESCROW_MAX_ASSETS_IN_DEAL = 8;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
protected:
    struct _TradeMessage
    {
        uint32 _contractIndex;

        id acceptorId;
        // uint8 s1;
        // id acceptorId;
        // uint8 s2;
        // uint64 s3;

        char _terminator;
    } _tradeMessage;

    struct AssetWithAmount
    {
        Asset asset;
        sint64 assetAmount;
    };

    struct Deal
    {
        id acceptorId;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> offeredAssets;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> requestedAssets;
        bit isActive;
    };

    struct CreateDeal_input
    {
        id acceptorId;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> offeredAssets;
        Array<AssetWithAmount, ESCROW_MAX_ASSETS_IN_DEAL> requestedAssets;
    };

    struct CreateDeal_output
    {
        sint64 index;
    };

    struct CreateDeal_locals
    {
        Deal newDeal;
    };

    struct GetDeals_input
    {
        id owner;
    };

    struct GetDeals_output
    {
        Array<Deal, 32> deals;
        uint64 dealsAmount;
    };

    struct GetDeals_locals
    {
        Deal tempDeal1;
        Deal tempDeal2;
    };

    uint64 _dealsCapacity;
    uint64 _numberOfActiveDeals;
    Collection<uint8, ESCROW_MAX_DEALS> _deals;

    PUBLIC_PROCEDURE_WITH_LOCALS(CreateDeal)
    {
        // locals.newDeal.acceptorId = input.acceptorId;
        // locals.newDeal.offeredAssets = input.offeredAssets;
        // locals.newDeal.requestedAssets = input.requestedAssets;
        // locals.newDeal.isActive = true;
        output.index = state._deals.add(qpi.invocator(), 6, 0);



        // // locals.newDeal.acceptorId = input.acceptorId;
        // locals.newDeal.offeredAssets = input.offeredAssets;
        // locals.newDeal.requestedAssets = input.requestedAssets;
        // locals.newDeal.isActive = true;
        // state._tradeMessage.s1 = 1;
        // state._tradeMessage.s2 = 2;
        // state._tradeMessage.acceptorId = qpi.invocator();
        // state._deals.add(qpi.invocator(), locals.newDeal, 0);
        // state._tradeMessage.s3 = state._deals.capacity();

        // state._tradeMessage.s3 = 3;
        // state._tradeMessage.s4 = 4;
        // state._tradeMessage.s5 = 5;
        // state._tradeMessage.s6 = 6;
        // state._tradeMessage.s7 = 7;
        // state._tradeMessage.s8 = 8;
        // state._tradeMessage.s9 = 9;
        // LOG_INFO(state._tradeMessage);
        // state._numberOfAllDeals++;



        // locals.newDeal.offeredAsset = input.offeredAsset;
        // locals.newDeal.numberOfOfferedAsset = input.numberOfOfferedAsset;
        // locals.newDeal.requestedAsset = input.requestedAsset;
        // locals.newDeal.numberOfRequestedAsset = input.numberOfRequestedAsset;
        // locals.newDeal.isActive = true;
        // locals.newDeal.isOfferedAssetReceivedBySc = false;
        // locals.newDeal.isRequestedAssetReceivedBySc = false;



        // state._deals.add(qpi.invocator(), )
        // // state.slotIndex = state._deals.headIndex(qpi.invocator(), 0);






        // locals.slotIndex = -1;
        // for (locals.ii = 0; locals.ii < ESCROW_MAX_DEALS; locals.ii++)
        // {
        //     locals.tempDeal = state._deals.get(locals.ii);
        //     if (!locals.tempDeal.isActive && !locals.tempDeal.isOfferedAssetReceivedBySc && !locals.tempDeal.isRequestedAssetReceivedBySc)
        //     {
        //         locals.slotIndex = locals.ii;
        //         break;
        //     }
        // }

        // if (locals.slotIndex == -1)
        // {
        //     return;
        // }

        // // state._numberOfDeals = input.delta;

        // locals.newDeal.ownerId = qpi.invocator();
        // locals.newDeal.acceptorId = input.acceptorId;
        // locals.newDeal.offeredAsset = input.offeredAsset;
        // locals.newDeal.numberOfOfferedAsset = input.numberOfOfferedAsset;
        // locals.newDeal.requestedAsset = input.requestedAsset;
        // locals.newDeal.numberOfRequestedAsset = input.numberOfRequestedAsset;
        // locals.newDeal.isActive = true;
        // locals.newDeal.isOfferedAssetReceivedBySc = false;
        // locals.newDeal.isRequestedAssetReceivedBySc = false;

        // locals.transferShareManagementRights_input.asset.assetName = input.offeredAsset.assetName;
        // locals.transferShareManagementRights_input.asset.issuer = input.offeredAsset.issuer;
        // locals.transferShareManagementRights_input.newManagingContractIndex = SELF_INDEX;
        // locals.transferShareManagementRights_input.numberOfShares = input.delta;

        // INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);
        // state._numberOfDeals = qpi.transferShareOwnershipAndPossession(input.offeredAsset.assetName, input.offeredAsset.issuer, SELF, SELF, input.delta, qpi.invocator());

        // // qpi.releaseShares(input.offeredAsset, qpi.invocator(), qpi.invocator(), input.numberOfOfferedAsset, 3, 3, 0);

        // state._deals.set((uint64) locals.slotIndex, locals.newDeal);
        // state._numberOfActiveDeals++;

        // // state._numberOfDeals = locals.transferShareManagementRights_input.numberOfShares;
        // state._counter += input.delta;
        // state._numberOfAllDeals = (uint64) locals.transferShareManagementRights_output.transferredNumberOfShares;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(GetDeals)
    {
        output.dealsAmount = state._deals.population();
        locals.tempDeal1.isActive = true;
        output.deals.set(0, locals.tempDeal1);
        locals.tempDeal2.isActive = true;
        locals.tempDeal2.acceptorId = qpi.invocator();
        output.deals.set(1, locals.tempDeal2);
        

        // locals.mess.s1 = 4;
        // LOG_INFO(locals.mess);

        // locals.count = 0ULL;

        // for (locals.i = 0ULL; locals.i < ESCROW_MAX_DEALS; locals.i++)
        // {
        //     locals.tempDeal = state._deals.get(locals.i);
        //     if (locals.tempDeal.isActive)
        //     {
        //         if (locals.tempDeal.ownerId == locals.tempDeal.acceptorId)
        //         {
        //             //output.dealsIds.set((uint64) locals.count, locals.i);
        //             locals.count++;
        //         }
        //     }
        // }

        // for (locals.i = 0ULL; locals.i < ESCROW_MAX_DEALS; locals.i++)
        // {
        //     locals.tempDeal = state._deals.get(locals.i);
        //     if (locals.tempDeal.isActive)
        //     {
        //         // locals.count++;  || locals.tempDeal.acceptorId == input.publicKey
        //         if (locals.tempDeal.ownerId == qpi.invocator())
        //         {
        //             output.dealsIds.set((uint64) locals.count, locals.i);
        //             locals.count++;
        //         }
        //     }
        // }
        
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(CreateDeal, 3);
        REGISTER_USER_FUNCTION(GetDeals, 4);
    }

    INITIALIZE()
    {
    }

    PRE_RELEASE_SHARES()
    {
    }

    POST_RELEASE_SHARES()
    {
    }

    PRE_ACQUIRE_SHARES()
    {
        output.allowTransfer = true;
    }

    POST_ACQUIRE_SHARES()
    {
    }
};
